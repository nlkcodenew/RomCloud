#include "logging/Logger.h"

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <sys/stat.h>

using RomCloud::Logger;

static bool exists(const std::string& path) {
    struct stat info;
    return stat(path.c_str(), &info) == 0;
}

int main() {
    const std::string path = "/tmp/romcloud-logger-clear.log";
    std::remove(path.c_str());
    std::remove((path + ".old").c_str());

    {
        std::ofstream old(path + ".old");
        old << "historical log";
    }

    Logger::instance().init(path);
    Logger::info("before reset");
    if (!Logger::instance().clear()) return 1;
    if (exists(path + ".old")) return 2;

    std::ifstream resetLog(path);
    std::string contents((std::istreambuf_iterator<char>(resetLog)), {});
    if (contents.find("LOG RESET") == std::string::npos) return 3;
    if (contents.find("before reset") != std::string::npos) return 4;

    Logger::info("after reset");
    Logger::instance().flush();
    std::ifstream activeLog(path);
    contents.assign((std::istreambuf_iterator<char>(activeLog)), {});
    return contents.find("after reset") == std::string::npos ? 5 : 0;
}
