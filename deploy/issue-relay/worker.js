const MAX_REQUEST_BYTES = 8_000_000;
const MAX_TITLE_CHARS = 240;
const MAX_BODY_CHARS = 60000;
const MAX_LOG_CHARS = 2_100_000;
const LOG_COMMENT_CHARS = 50000;
const DEDUPE_SECONDS = 30 * 24 * 60 * 60;
const RATE_LIMIT_MAX = 10;
const RATE_LIMIT_WINDOW_SECONDS = 10 * 60;

const TOKEN_PATTERN = /\b(?:ghp|github_pat|gho|ghu|ghs|ghr)_[^\s,}\]["']+/g;
const SECRET_LINE_PATTERN = /^([^\n]*?(?:token|password|passwd|secret|refresh_token|access_token|client_secret|serial[-_ ]?(?:number|no)|sunxi_chipid|chip[-_ ]?id|machine[-_ ]?id)[^:=\n]*[:=]\s*)[^\s,}\]]+/gim;
const PRIVATE_IP_PATTERN = /\b(?:10(?:\.\d{1,3}){3}|192\.168(?:\.\d{1,3}){2}|172\.(?:1[6-9]|2\d|3[01])(?:\.\d{1,3}){2})\b/g;
const MAC_PATTERN = /\b(?:[0-9a-f]{2}[:-]){5}[0-9a-f]{2}\b/gi;

function jsonResponse(payload, status) {
  return new Response(JSON.stringify(payload), {
    status,
    headers: {
      "Content-Type": "application/json; charset=utf-8",
      "Cache-Control": "no-store",
      "X-Content-Type-Options": "nosniff",
    },
  });
}

function sanitize(value) {
  return String(value)
    .replace(SECRET_LINE_PATTERN, "$1[REDACTED_SECRET]")
    .replace(TOKEN_PATTERN, "[REDACTED_TOKEN]")
    .replace(PRIVATE_IP_PATTERN, "[PRIVATE_IP]")
    .replace(MAC_PATTERN, "[MAC_ADDRESS]")
    .replaceAll("\u0000", "");
}

function validPayload(payload) {
  return payload &&
    payload.schema === 1 &&
    payload.app === "romcloud" &&
    typeof payload.version === "string" &&
    /^[0-9]+\.[0-9]+\.[0-9]+(?:[-A-Za-z0-9.]+)?$/.test(payload.version) &&
    typeof payload.fingerprint === "string" &&
    /^[a-f0-9]{64}$/.test(payload.fingerprint) &&
    typeof payload.title === "string" &&
    payload.title.startsWith("[device-log]") &&
    payload.title.length <= MAX_TITLE_CHARS &&
    typeof payload.body === "string" &&
    payload.body.length <= MAX_BODY_CHARS &&
    (payload.log === undefined ||
      (typeof payload.log === "string" && payload.log.length <= MAX_LOG_CHARS));
}

function githubHeaders(env) {
  return {
    "Accept": "application/vnd.github+json",
    "Authorization": `Bearer ${env.GITHUB_TOKEN}`,
    "Content-Type": "application/json",
    "User-Agent": "romcloud-issue-relay",
    "X-GitHub-Api-Version": "2022-11-28",
  };
}

function splitLog(log) {
  const chunks = [];
  for (let offset = 0; offset < log.length; offset += LOG_COMMENT_CHARS) {
    chunks.push(log.slice(offset, offset + LOG_COMMENT_CHARS));
  }
  return chunks;
}

async function rateLimitKey(request) {
  const address = request.headers.get("cf-connecting-ip") || "unknown";
  const digest = await crypto.subtle.digest(
    "SHA-256",
    new TextEncoder().encode(`romcloud-relay-rate-v1\u0000${address}`),
  );
  return Array.from(new Uint8Array(digest))
    .slice(0, 12)
    .map((byte) => byte.toString(16).padStart(2, "0"))
    .join("");
}

async function enforceRateLimit(request, env) {
  if (!env.REPORTS) return false;
  const now = Math.floor(Date.now() / 1000);
  const window = Math.floor(now / RATE_LIMIT_WINDOW_SECONDS);
  const key = `rate:${await rateLimitKey(request)}:${window}`;
  const count = Number(await env.REPORTS.get(key) || 0);
  if (count >= RATE_LIMIT_MAX) return true;
  await env.REPORTS.put(key, String(count + 1), {
    expirationTtl: RATE_LIMIT_WINDOW_SECONDS * 2,
  });
  return false;
}

async function handleReport(request, env) {
  if (!env.GITHUB_TOKEN || !/^[A-Za-z0-9_.-]+\/[A-Za-z0-9_.-]+$/.test(env.GITHUB_REPO || "")) {
    return jsonResponse({ error: "relay_not_configured" }, 503);
  }
  if (!request.headers.get("content-type")?.toLowerCase().startsWith("application/json")) {
    return jsonResponse({ error: "unsupported_media_type" }, 415);
  }
  if (await enforceRateLimit(request, env)) {
    return jsonResponse({ error: "rate_limited" }, 429);
  }
  const contentLength = Number(request.headers.get("content-length") || 0);
  if (contentLength > MAX_REQUEST_BYTES) {
    return jsonResponse({ error: "payload_too_large" }, 413);
  }
  const rawBody = await request.arrayBuffer();
  if (rawBody.byteLength > MAX_REQUEST_BYTES) {
    return jsonResponse({ error: "payload_too_large" }, 413);
  }

  let payload;
  try {
    payload = JSON.parse(new TextDecoder().decode(rawBody));
  } catch {
    return jsonResponse({ error: "invalid_json" }, 400);
  }
  if (!validPayload(payload)) return jsonResponse({ error: "invalid_report" }, 400);

  const dedupeKey = `report:${payload.fingerprint}`;
  const existing = env.REPORTS ? await env.REPORTS.get(dedupeKey, "json") : null;
  if (existing?.complete && existing?.issue_url) {
    return jsonResponse({ accepted: true, duplicate: true, issue_url: existing.issue_url }, 200);
  }

  let issueUrl = typeof existing?.issue_url === "string" ? existing.issue_url : "";
  let issueNumber = Number(existing?.issue_number || 0);
  let uploadedParts = Number(existing?.uploaded_parts || 0);
  if (!issueNumber) {
    const githubResponse = await fetch(`https://api.github.com/repos/${env.GITHUB_REPO}/issues`, {
      method: "POST",
      headers: githubHeaders(env),
      body: JSON.stringify({
        title: sanitize(payload.title).slice(0, MAX_TITLE_CHARS),
        body: sanitize(payload.body).slice(0, MAX_BODY_CHARS),
      }),
    });
    if (!githubResponse.ok) {
      console.error("GitHub issue creation failed", githubResponse.status);
      return jsonResponse({ error: "github_rejected_report" }, 502);
    }
    const issue = await githubResponse.json();
    issueUrl = typeof issue.html_url === "string" ? issue.html_url : "";
    issueNumber = Number(issue.number || 0);
    if (!issueNumber) return jsonResponse({ error: "github_invalid_response" }, 502);
    if (env.REPORTS) {
      await env.REPORTS.put(dedupeKey, JSON.stringify({
        issue_url: issueUrl,
        issue_number: issueNumber,
        uploaded_parts: 0,
        complete: false,
      }), { expirationTtl: DEDUPE_SECONDS });
    }
  }

  const sanitizedLog = sanitize(payload.log || "");
  const logChunks = splitLog(sanitizedLog);
  for (let index = uploadedParts; index < logChunks.length; index += 1) {
    const commentResponse = await fetch(
      `https://api.github.com/repos/${env.GITHUB_REPO}/issues/${issueNumber}/comments`,
      {
        method: "POST",
        headers: githubHeaders(env),
        body: JSON.stringify({
          body: `### debug.log (${index + 1}/${logChunks.length})\n\n\`\`\`text\n${logChunks[index]}\n\`\`\``,
        }),
      },
    );
    if (!commentResponse.ok) {
      console.error("GitHub log comment failed", commentResponse.status, index + 1);
      return jsonResponse({ error: "github_rejected_log", issue_url: issueUrl }, 502);
    }
    uploadedParts = index + 1;
    if (env.REPORTS) {
      await env.REPORTS.put(dedupeKey, JSON.stringify({
        issue_url: issueUrl,
        issue_number: issueNumber,
        uploaded_parts: uploadedParts,
        complete: false,
      }), { expirationTtl: DEDUPE_SECONDS });
    }
  }
  if (env.REPORTS && issueUrl) {
    await env.REPORTS.put(dedupeKey, JSON.stringify({
      issue_url: issueUrl,
      issue_number: issueNumber,
      uploaded_parts: logChunks.length,
      complete: true,
    }), {
      expirationTtl: DEDUPE_SECONDS,
    });
  }
  return jsonResponse({ accepted: true, issue_url: issueUrl, log_parts: logChunks.length }, 201);
}

export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    if (request.method === "GET" && url.pathname === "/health") {
      return jsonResponse({ ok: true }, 200);
    }
    if (request.method !== "POST" || url.pathname !== "/report") {
      return jsonResponse({ error: "not_found" }, 404);
    }
    try {
      return await handleReport(request, env);
    } catch (error) {
      console.error("Report relay failed", error?.name || "Error");
      return jsonResponse({ error: "relay_error" }, 500);
    }
  },
};
