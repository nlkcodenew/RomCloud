#pragma once

#include <string>

namespace RomCloud {

class IssueLogger {
public:
    static IssueLogger& instance();

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

    bool m_enabled = false;
    std::string m_relayUrl;
    int m_issueCount = 0;
};

} // namespace RomCloud
