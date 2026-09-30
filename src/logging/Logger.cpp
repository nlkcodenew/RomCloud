#include "Logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <sys/stat.h>
#include <cerrno>
#include <cstdio>

namespace RomCloud {

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

Logger::~Logger() {
    flush();
    if (m_logFile.is_open()) m_logFile.close();
}

void Logger::init(const std::string& logFilePath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_filePath = logFilePath;
    if (m_logFile.is_open()) m_logFile.close();

    // Keep one previous log plus a detailed current log for remote diagnostics.
    struct stat st;
    if (stat(logFilePath.c_str(), &st) == 0 && st.st_size > 2 * 1024 * 1024) {
        std::string oldPath = logFilePath + ".old";
        remove(oldPath.c_str());
        rename(logFilePath.c_str(), oldPath.c_str());
    }

    m_logFile.open(logFilePath, std::ios::out | std::ios::app);
    m_initialized = m_logFile.is_open();
}

void Logger::header(const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::cout << message << std::endl;
    if (m_initialized && m_logFile.is_open()) {
        m_logFile << message << std::endl;
        m_logFile.flush();
    }
}

void Logger::log(LogLevel level, const std::string& message) {
    std::function<void(const std::string&)> errorCallback;
    std::unique_lock<std::mutex> lock(m_mutex);
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");

    const char* levelStr = "INFO";
    switch (level) {
        case LogLevel::DEBUG:     levelStr = "DEBUG"; break;
        case LogLevel::INFO:      levelStr = "INFO";  break;
        case LogLevel::WARNING:   levelStr = "WARN";  break;
        case LogLevel::LOG_ERROR: levelStr = "ERROR"; break;
    }

    std::string line = "[" + ss.str() + "] [" + levelStr + "] " + message;
    std::cout << line << std::endl;

    if (m_initialized && m_logFile.is_open()) {
        m_logFile << line << std::endl;
        m_logFile.flush();
    }

    if (level == LogLevel::LOG_ERROR) errorCallback = m_errorCallback;
    lock.unlock();
    if (errorCallback) errorCallback(message);
}

void Logger::setErrorCallback(std::function<void(const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_errorCallback = std::move(callback);
}

void Logger::flush() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_initialized && m_logFile.is_open()) m_logFile.flush();
}

bool Logger::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_filePath.empty()) return false;

    if (m_logFile.is_open()) m_logFile.close();
    const std::string oldPath = m_filePath + ".old";
    errno = 0;
    const bool currentRemoved = std::remove(m_filePath.c_str()) == 0 || errno == ENOENT;
    errno = 0;
    const bool oldRemoved = std::remove(oldPath.c_str()) == 0 || errno == ENOENT;

    m_logFile.clear();
    m_logFile.open(m_filePath, std::ios::out | std::ios::trunc);
    m_initialized = m_logFile.is_open();
    if (m_initialized) {
        m_logFile << "[LOG RESET] Previous RomCloud logs were cleared by the user." << std::endl;
        m_logFile.flush();
    }
    return currentRemoved && oldRemoved && m_initialized;
}

void Logger::debug(const std::string& msg) { instance().log(LogLevel::DEBUG, msg); }
void Logger::info(const std::string& msg)  { instance().log(LogLevel::INFO, msg); }
void Logger::warn(const std::string& msg)  { instance().log(LogLevel::WARNING, msg); }
void Logger::error(const std::string& msg) { instance().log(LogLevel::LOG_ERROR, msg); }

} // namespace RomCloud
