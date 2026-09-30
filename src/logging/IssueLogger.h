#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <thread>

namespace RomCloud {

class IssueLogger {
public:
    static IssueLogger& instance();

    void init();
    void shutdown();
    void enqueueError(const std::string& message);

    bool logError(const std::string& errorType,
                  const std::string& errorMessage,
                  const std::string& stackTrace = "",
                  const std::string& context = "");
    bool logCrash(const std::string& crashInfo,
                  const std::string& stackTrace = "",
                  const std::string& deviceInfo = "");
    bool uploadPending(const std::string& reason = "startup");
    bool isEnabled() const;
    int getRecentIssuesCount() const;

private:
    IssueLogger();
    ~IssueLogger();
    IssueLogger(const IssueLogger&) = delete;
    IssueLogger& operator=(const IssueLogger&) = delete;

    bool sendReport(const std::string& reason,
                    const std::string& summary,
                    const std::string& details);
    std::string readRelayUrl() const;
    std::string readLogTail() const;
    std::string sanitize(const std::string& text) const;
    std::string jsonEscape(const std::string& text) const;
    std::string cleanReason(const std::string& reason) const;
    void rememberPending(const std::string& reason,
                         const std::string& summary,
                         const std::string& details) const;
    void workerLoop();

    bool m_enabled = false;
    std::string m_relayUrl;
    int m_issueCount = 0;
    std::mutex m_queueMutex;
    std::condition_variable m_queueCv;
    std::deque<std::string> m_errorQueue;
    std::thread m_worker;
    bool m_running = false;
    bool m_stopRequested = false;
};

} // namespace RomCloud
