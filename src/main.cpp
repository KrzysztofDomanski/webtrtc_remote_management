#include "webrtc_mgmt/service.hpp"
#include <iostream>
#include <string>
#include <cstring>

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options]\n"
              << "Options:\n"
              << "  -h, --help           Show this help message\n"
              << "  -f, --foreground     Run in foreground (default)\n"
              << "  -i, --install        Install as system service\n"
              << "  -u, --uninstall      Uninstall system service\n"
              << "  -v, --version        Show version information\n"
              << std::endl;
}

void printVersion() {
    std::cout << "WebRTC Remote Management Service v0.1.0\n"
              << "Built with C++17, libdatachannel, and mbedtls\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    webrtc_mgmt::ServiceConfig config;
    config.serviceName = "WebRTCRemoteMgmt";
    config.displayName = "WebRTC Remote Management Service";
    config.description = "Remote monitoring and management service using WebRTC";

    // Parse command line arguments
    bool foreground = true;
    bool install = false;
    bool uninstall = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            printVersion();
            return 0;
        } else if (arg == "-f" || arg == "--foreground") {
            foreground = true;
        } else if (arg == "-i" || arg == "--install") {
            install = true;
            foreground = false;
        } else if (arg == "-u" || arg == "--uninstall") {
            uninstall = true;
            foreground = false;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }

    try {
        if (install) {
            if (webrtc_mgmt::RemoteManagementService::install(config)) {
                std::cout << "Service installed successfully" << std::endl;
                return 0;
            } else {
                std::cerr << "Failed to install service" << std::endl;
                return 1;
            }
        }

        if (uninstall) {
            if (webrtc_mgmt::RemoteManagementService::uninstall(config)) {
                std::cout << "Service uninstalled successfully" << std::endl;
                return 0;
            } else {
                std::cerr << "Failed to uninstall service" << std::endl;
                return 1;
            }
        }

        if (foreground) {
            std::cout << "Starting WebRTC Remote Management Service..." << std::endl;
            webrtc_mgmt::RemoteManagementService service(config);
            return service.runForeground();
        }

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
