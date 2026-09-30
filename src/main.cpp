#include "app/Application.h"
#include "logging/IssueLogger.h"
#include "logging/Logger.h"
#include <exception>
#include <iostream>

int main(int argc, char* argv[]) {
    auto& app = RomCloud::Application::instance();
    try {
        if (!app.init(argc, argv)) {
            std::cerr << "[FATAL] Failed to initialize RomCloud application." << std::endl;
            RomCloud::IssueLogger::instance().logCrash("application_init_failed");
            return 1;
        }
        app.run();
        app.shutdown();
        return app.getExitCode();
    } catch (const std::exception& error) {
        RomCloud::Logger::error(std::string("Unhandled exception: ") + error.what());
        RomCloud::IssueLogger::instance().logCrash("unhandled_exception", error.what());
    } catch (...) {
        RomCloud::Logger::error("Unhandled non-standard exception");
        RomCloud::IssueLogger::instance().logCrash("unhandled_unknown_exception");
    }
    app.shutdown();
    return 1;
}
