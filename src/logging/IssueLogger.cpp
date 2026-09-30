#include "IssueLogger.h"

#include "../config/AppConfig.h"
#include "../network/JsonHelper.h"
#include "../ota/UpdateManager.h"
#include "../platform/DeviceIdentity.h"
#include "../platform/PlatformInfo.h"
#include "Logger.h"

#include <curl/curl.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <regex>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace RomCloud {

namespace {

constexpr size_t MAX_LOG_BYTES = 2 * 1024 * 1024;
constexpr size_t MAX_BODY_CHARS = 48000;

size_t captureResponse(void* contents, size_t size, size_t count, void* userData) {
    const size_t bytes = size * count;
    static_cast<std::string*>(userData)->append(static_cast<const char*>(contents), bytes);
    return bytes;
}

} // namespace

IssueLogger& IssueLogger::instance() {
    static IssueLogger instance;
    return instance;
}

IssueLogger::IssueLogger() {
    m_relayUrl = readRelayUrl();
    const bool hasHttpsPrefix = m_relayUrl.rfind("https://", 0) == 0;
    const size_t hostStart = std::string("https://").size();
    const size_t pathStart = hasHttpsPrefix ? m_relayUrl.find('/', hostStart) : std::string::npos;
    const std::string authority = !hasHttpsPrefix ? "" :
        (pathStart == std::string::npos
            ? m_relayUrl.substr(hostStart)
            : m_relayUrl.substr(hostStart, pathStart - hostStart));
    m_enabled = hasHttpsPrefix &&
                !authority.empty() && authority.find('@') == std::string::npos &&
                m_relayUrl.find('?') == std::string::npos &&
                m_relayUrl.find('#') == std::string::npos;
    if (m_enabled) {
        Logger::info("IssueLogger: HTTPS relay enabled; no GitHub token is stored on device");
    } else {
        Logger::warn("IssueLogger: diagnostics relay is not configured");
    }
}

IssueLogger::~IssueLogger() {
    shutdown();
}

void IssueLogger::init() {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    if (m_running) return;
    m_stopRequested = false;
    m_running = true;
    m_worker = std::thread(&IssueLogger::workerLoop, this);
}

void IssueLogger::shutdown() {
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        if (!m_running) return;
        m_stopRequested = true;
    }
    m_queueCv.notify_all();
    if (m_worker.joinable()) m_worker.join();
    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_running = false;
}

void IssueLogger::enqueueError(const std::string& message) {
    if (message.rfind("IssueLogger:", 0) == 0) return;
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        if (std::find(m_errorQueue.begin(), m_errorQueue.end(), message) != m_errorQueue.end()) return;
        if (m_errorQueue.size() >= 100) m_errorQueue.pop_front();
        m_errorQueue.push_back(message);
    }
    m_queueCv.notify_one();
}

void IssueLogger::workerLoop() {
    while (true) {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        m_queueCv.wait(lock, [this]() { return m_stopRequested || !m_errorQueue.empty(); });
        if (m_stopRequested && m_errorQueue.empty()) break;
        m_queueCv.wait_for(lock, std::chrono::seconds(2), [this]() { return m_stopRequested; });

        std::deque<std::string> errors;
        errors.swap(m_errorQueue);
        lock.unlock();

        std::ostringstream details;
        details << "Các lỗi được gom trong cùng một tác vụ:\n";
        for (const auto& error : errors) details << "- " << error << "\n";
        sendReport("runtime_error", "Báo cáo lỗi vận hành tự động từ thiết bị TrimUI.", details.str());
    }
}

bool IssueLogger::isEnabled() const { return m_enabled; }
int IssueLogger::getRecentIssuesCount() const { return m_issueCount; }

std::string IssueLogger::readRelayUrl() const {
    const char* envUrl = getenv("ROMCLOUD_ISSUE_RELAY_URL");
    if (envUrl && std::string(envUrl).rfind("https://", 0) == 0) return envUrl;

    std::ifstream file(AppConfig::instance().getConfigDir() + "/reporting.json");
    if (!file.is_open()) return "";
    std::stringstream buffer;
    buffer << file.rdbuf();
    return JsonHelper::extractString(buffer.str(), "issue_relay_url");
}

std::string IssueLogger::readLogTail() const {
    const std::string currentPath = Logger::instance().getLogFilePath();
    const std::string appRoot = AppConfig::instance().getAppRoot();
    std::string result;
    const std::vector<std::pair<std::string, std::string>> logFiles = {
        {"DEBUG_PREVIOUS", currentPath + ".old"},
        {"DEBUG_CURRENT", currentPath},
        {"YOUTUBE_RESOLVER", "/tmp/romcloud_youtube_error.log"},
        {"YOUTUBE_MPV", appRoot + "/youtube_mpv.log"},
        {"TIKTOK_MPV", appRoot + "/tiktok_mpv.log"}
    };
    for (const auto& entry : logFiles) {
        const std::string& label = entry.first;
        const std::string& path = entry.second;
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) continue;
        file.seekg(0, std::ios::end);
        const std::streamoff size = file.tellg();
        file.seekg(std::max<std::streamoff>(0, size - static_cast<std::streamoff>(MAX_LOG_BYTES)));
        std::stringstream buffer;
        buffer << file.rdbuf();
        if (!result.empty()) result += "\n";
        result += "--- BEGIN " + label + " [" + path + "] ---\n";
        result += buffer.str();
        result += "\n--- END " + label + " ---\n";
        if (result.size() > MAX_LOG_BYTES) result.erase(0, result.size() - MAX_LOG_BYTES);
    }
    return result;
}

std::string IssueLogger::sanitize(const std::string& input) const {
    std::string value = input;
    value = std::regex_replace(value,
        std::regex("(token|password|passwd|secret|refresh_token|access_token|client_secret|serial[-_ ]?(number|no)|sunxi_chipid|chip[-_ ]?id|machine[-_ ]?id)([^:=\\n]*[:=]\\s*)[^\\s,}\\]]+", std::regex::icase),
        "$1$3[REDACTED_SECRET]");
    value = std::regex_replace(value,
        std::regex("\\b(ghp|github_pat|gho|ghu|ghs|ghr)_[^\\s,}\\]\\[\\\"']+", std::regex::icase),
        "[REDACTED_TOKEN]");
    value = std::regex_replace(value,
        std::regex("\\b(10(\\.\\d{1,3}){3}|192\\.168(\\.\\d{1,3}){2}|172\\.(1[6-9]|2\\d|3[01])(\\.\\d{1,3}){2})\\b"),
        "[PRIVATE_IP]");
    value = std::regex_replace(value,
        std::regex("\\b([0-9a-f]{2}[:-]){5}[0-9a-f]{2}\\b", std::regex::icase),
        "[MAC_ADDRESS]");
    value.erase(std::remove(value.begin(), value.end(), '\0'), value.end());
    size_t fence = 0;
    while ((fence = value.find("```", fence)) != std::string::npos) {
        value.replace(fence, 3, "` ` `");
        fence += 5;
    }
    return value;
}

std::string IssueLogger::jsonEscape(const std::string& input) const {
    std::ostringstream output;
    for (unsigned char ch : input) {
        switch (ch) {
            case '\\': output << "\\\\"; break;
            case '"': output << "\\\""; break;
            case '\n': output << "\\n"; break;
            case '\r': output << "\\r"; break;
            case '\t': output << "\\t"; break;
            default:
                if (ch < 0x20) {
                    char escaped[7];
                    std::snprintf(escaped, sizeof(escaped), "\\u%04x", ch);
                    output << escaped;
                } else {
                    output << static_cast<char>(ch);
                }
        }
    }
    return output.str();
}

std::string IssueLogger::cleanReason(const std::string& reason) const {
    std::string value;
    for (unsigned char ch : reason) {
        if (std::isalnum(ch) || ch == '_' || ch == '.' || ch == '-') value += ch;
        else value += '_';
        if (value.size() >= 80) break;
    }
    return value.empty() ? "error" : value;
}

void IssueLogger::rememberPending(const std::string& reason,
                                  const std::string& summary,
                                  const std::string& details) const {
    std::ofstream pending(AppConfig::instance().getDataDir() + "/pending_issue_report",
                          std::ios::trunc);
    if (!pending.is_open()) return;
    pending << "reason=" << cleanReason(reason) << "\n";
    pending << sanitize(summary) << "\n";
    pending << sanitize(details) << "\n";
}

bool IssueLogger::sendReport(const std::string& rawReason,
                             const std::string& rawSummary,
                             const std::string& rawDetails) {
    if (!m_enabled) {
        rememberPending(rawReason, rawSummary, rawDetails);
        return false;
    }

    const std::string reason = cleanReason(rawReason);
    const std::string hardwareId = DeviceIdentity::hardwareId();
    const std::string model = DeviceIdentity::deviceModel();
    const std::string fullLog = sanitize(readLogTail());
    std::string details = sanitize(rawDetails);
    if (details.size() > 16000) details = details.substr(details.size() - 16000);
    const std::string fingerprint = DeviceIdentity::sha256Hex(
        hardwareId + "\n" + reason + "\n" + details + "\n" + fullLog);

    const std::string statePath = AppConfig::instance().getDataDir() + "/issue_report_state";
    std::ifstream stateIn(statePath);
    std::string previousFingerprint;
    std::getline(stateIn, previousFingerprint);
    if (previousFingerprint == fingerprint) {
        Logger::info("IssueLogger: duplicate report skipped");
        return true;
    }

    auto diag = PlatformInfo::instance().getDiagnostics();
    std::ostringstream body;
    body << sanitize(rawSummary) << "\n\n";
    body << "| Trường | Giá trị |\n|---|---|\n";
    body << "| App | RomCloud v" << APP_VERSION << " |\n";
    body << "| Lý do | `" << reason << "` |\n";
    body << "| Model | `" << model << "` |\n";
    body << "| Mã thiết bị băm | `" << hardwareId << "` |\n";
    body << "| Hệ điều hành | `" << sanitize(diag.osName + " " + diag.kernelRelease) << "` |\n";
    body << "| Màn hình | `" << diag.displayResolution << "` |\n";
    body << "| Fingerprint | `" << fingerprint.substr(0, 16) << "` |\n\n";
    body << "> Token, mật khẩu, IP nội bộ, MAC, serial, chip ID và machine-id thô đã được lọc.\n";
    if (!details.empty()) body << "\n### Chi tiết\n```text\n" << details << "\n```\n";
    std::string bodyText = body.str().substr(0, MAX_BODY_CHARS);

    const std::string title = "[device-log][" + hardwareId + "] v" + APP_VERSION +
                              " " + reason + " " + fingerprint.substr(0, 8);
    const std::string payload = "{\"schema\":1,\"app\":\"romcloud\",\"version\":\"" +
        std::string(APP_VERSION) + "\",\"fingerprint\":\"" + fingerprint +
        "\",\"title\":\"" + jsonEscape(title) + "\",\"body\":\"" +
        jsonEscape(bodyText) + "\",\"log\":\"" + jsonEscape(fullLog) + "\"}";

    CURL* curl = curl_easy_init();
    if (!curl) return false;
    curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, ("User-Agent: RomCloud/" + std::string(APP_VERSION)).c_str());
    curl_easy_setopt(curl, CURLOPT_URL, m_relayUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(payload.size()));
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    std::string responseBody;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, captureResponse);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);
    if (access("/etc/ssl/certs/ca-certificates.crt", F_OK) == 0) {
        curl_easy_setopt(curl, CURLOPT_CAINFO, "/etc/ssl/certs/ca-certificates.crt");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    } else {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    }
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 8L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 90L);

    const CURLcode result = curl_easy_perform(curl);
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (result == CURLE_OK && (httpCode == 200 || httpCode == 201)) {
        std::ofstream stateOut(statePath, std::ios::trunc);
        stateOut << fingerprint << "\n";
        ++m_issueCount;
        Logger::info("IssueLogger: diagnostic report accepted for " + hardwareId +
                     (responseBody.empty() ? "" : " response=" + responseBody));
        return true;
    }
    Logger::warn("IssueLogger: relay upload failed (curl=" + std::string(curl_easy_strerror(result)) +
                 ", HTTP " + std::to_string(httpCode) + ", response=" + responseBody + ")");
    rememberPending(reason, rawSummary, rawDetails);
    return false;
}

bool IssueLogger::logError(const std::string& errorType,
                           const std::string& errorMessage,
                           const std::string& stackTrace,
                           const std::string& context) {
    return sendReport(errorType, "Báo cáo lỗi vận hành được gửi tự động từ thiết bị TrimUI.",
                      errorMessage + "\n" + stackTrace + "\n" + context);
}

bool IssueLogger::logCrash(const std::string& crashInfo,
                           const std::string& stackTrace,
                           const std::string& deviceInfo) {
    return sendReport("crash", "Báo cáo crash được gửi tự động từ thiết bị TrimUI.",
                      crashInfo + "\n" + stackTrace + "\n" + deviceInfo);
}

bool IssueLogger::uploadPending(const std::string& reason) {
    const std::string pendingPath = AppConfig::instance().getDataDir() + "/pending_issue_report";
    std::ifstream pending(pendingPath);
    if (!pending.is_open()) return false;
    std::stringstream details;
    details << pending.rdbuf();
    if (!sendReport(reason, "Báo cáo lỗi đang chờ được gửi lại từ thiết bị TrimUI.", details.str())) {
        return false;
    }
    std::remove(pendingPath.c_str());
    return true;
}

} // namespace RomCloud
