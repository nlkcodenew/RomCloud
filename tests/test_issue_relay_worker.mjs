import assert from "node:assert/strict";
import { webcrypto } from "node:crypto";
import { readFile } from "node:fs/promises";

globalThis.crypto ??= webcrypto;

const source = await readFile(new URL("../deploy/issue-relay/worker.js", import.meta.url), "utf8");
const moduleUrl = `data:text/javascript;base64,${Buffer.from(source).toString("base64")}`;
const { default: worker } = await import(moduleUrl);

class MemoryKv {
  constructor() {
    this.values = new Map();
  }

  async get(key, type) {
    const value = this.values.get(key);
    if (type === "json" && value) return JSON.parse(value);
    return value ?? null;
  }

  async put(key, value) {
    this.values.set(key, value);
  }
}

const env = {
  GITHUB_TOKEN: "test-token-not-sent-to-client",
  GITHUB_REPO: "nlkcodenew/RomCloud-diagnostics",
  REPORTS: new MemoryKv(),
};

let githubCalls = 0;
let githubPayload;
const githubComments = [];
globalThis.fetch = async (url, options) => {
  githubCalls += 1;
  const payload = JSON.parse(options.body);
  if (String(url).endsWith("/comments")) {
    githubComments.push(payload.body);
    return new Response(JSON.stringify({ id: githubComments.length }), { status: 201 });
  }
  githubPayload = payload;
  return new Response(JSON.stringify({ html_url: "https://github.com/example/issues/1", number: 1 }), {
    status: 201,
    headers: { "Content-Type": "application/json" },
  });
};

const fingerprint = "a".repeat(64);
const report = {
  schema: 1,
  app: "romcloud",
  version: "2.1.2",
  fingerprint,
  title: "[device-log][HW-ABCDEF123456] v2.1.2 crash aaaaaaaa",
  body: "token=github_pat_secret 192.168.1.2 aa:bb:cc:dd:ee:ff",
  log: `${"diagnostic line\n".repeat(4000)}token=github_pat_log_secret`,
};

const request = () => new Request("https://relay.example/report", {
  method: "POST",
  headers: { "Content-Type": "application/json", "cf-connecting-ip": "203.0.113.10" },
  body: JSON.stringify(report),
});

const firstResponse = await worker.fetch(request(), env);
assert.equal(firstResponse.status, 201);
assert.equal(githubCalls, 3);
assert.match(githubPayload.body, /\[REDACTED_(?:TOKEN|SECRET)\]/);
assert.match(githubPayload.body, /\[PRIVATE_IP\]/);
assert.match(githubPayload.body, /\[MAC_ADDRESS\]/);
assert.doesNotMatch(githubPayload.body, /github_pat_secret|192\.168\.1\.2|aa:bb:cc:dd:ee:ff/);
assert.equal(githubComments.length, 2);
assert.match(githubComments.join("\n"), /debug\.log \(1\/2\)/);
assert.doesNotMatch(githubComments.join("\n"), /github_pat_log_secret/);

const duplicateResponse = await worker.fetch(request(), env);
assert.equal(duplicateResponse.status, 200);
assert.equal(githubCalls, 3);

const invalidResponse = await worker.fetch(new Request("https://relay.example/report", {
  method: "POST",
  headers: { "Content-Type": "application/json" },
  body: JSON.stringify({ ...report, app: "unexpected-app" }),
}), env);
assert.equal(invalidResponse.status, 400);

console.log("issue relay worker tests passed");
