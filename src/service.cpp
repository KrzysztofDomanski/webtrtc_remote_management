#include "webrtc_mgmt/service.hpp"
#include "webrtc_mgmt/data_channel_manager.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

namespace webrtc_mgmt {

static std::atomic<bool> g_running{false};
static std::atomic<bool> g_shutdownRequested{false};

void signalHandler(int signal) {
    std::cout << "Received signal " << signal << ", shutting down..." << std::endl;
    g_shutdownRequested.store(true);
}

class RemoteManagementServiceImpl {
public:
    explicit RemoteManagementServiceImpl(const ServiceConfig& config)
        : config_(config)
        , status_(ServiceStatus::STOPPED) {}

    ~RemoteManagementServiceImpl() {
        stop();
    }

    bool start() {
        if (status_ == ServiceStatus::RUNNING) {
            return true;
        }

        status_ = ServiceStatus::STARTING;
        std::cout << "Starting " << config_.displayName << "..." << std::endl;

        // Set up signal handlers
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);

        // Configure data channel manager
        DataChannelManagerConfig dcmConfig;

        // Configure peer connection with default STUN servers
        dcmConfig.peerConfig.iceServers = {
            {"stun:stun.l.google.com:19302", "", ""},
            {"stun:stun1.l.google.com:19302", "", ""}
        };

        // For local testing, can disable ICE servers
        // dcmConfig.peerConfig.disableIceServers = true;

        // Configure signaling
        dcmConfig.signalingConfig.serverUrl = "ws://localhost:8080/signaling";
        dcmConfig.signalingConfig.peerId = config_.serviceName;

        // Configure channels for different data types
        dcmConfig.channelLabels = {
            "control",    // Control commands
            "metrics",    // Performance metrics
            "logs",       // Application logs
            "alerts",     // Alert notifications
            "data"        // General data repository
        };

        // Create data channel manager
        dataChannelManager_ = std::make_unique<DataChannelManager>(dcmConfig);

        // Set up message handlers
        setupMessageHandlers();

        // Start the manager
        if (!dataChannelManager_->start()) {
            std::cerr << "Failed to start data channel manager" << std::endl;
            status_ = ServiceStatus::ERROR;
            return false;
        }

        g_running.store(true);
        status_ = ServiceStatus::RUNNING;
        std::cout << config_.displayName << " started successfully" << std::endl;

        return true;
    }

    void stop() {
        if (status_ == ServiceStatus::STOPPED) {
            return;
        }

        status_ = ServiceStatus::STOPPING;
        std::cout << "Stopping " << config_.displayName << "..." << std::endl;

        g_running.store(false);

        if (dataChannelManager_) {
            dataChannelManager_->stop();
            dataChannelManager_.reset();
        }

        status_ = ServiceStatus::STOPPED;
        std::cout << config_.displayName << " stopped" << std::endl;
    }

    ServiceStatus getStatus() const {
        return status_;
    }

    int runForeground() {
        if (!start()) {
            return 1;
        }

        std::cout << "Service running in foreground. Press Ctrl+C to stop." << std::endl;

        // Main service loop
        while (g_running.load() && !g_shutdownRequested.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // Send periodic heartbeat on control channel
            if (dataChannelManager_->isChannelOpen("control")) {
                static auto lastHeartbeat = std::chrono::steady_clock::now();
                auto now = std::chrono::steady_clock::now();
                if (std::chrono::duration_cast<std::chrono::seconds>(now - lastHeartbeat).count() >= 30) {
                    std::string heartbeat = "heartbeat:" + std::to_string(
                        std::chrono::system_clock::now().time_since_epoch().count()
                    );
                    dataChannelManager_->send("control", heartbeat);
                    lastHeartbeat = now;
                }
            }
        }

        stop();
        return 0;
    }

private:
    void setupMessageHandlers() {
        // Control channel handler
        dataChannelManager_->onChannelMessage("control", [](const std::vector<uint8_t>& data) {
            std::string message(data.begin(), data.end());
            std::cout << "[Control] Received: " << message << std::endl;
            // TODO: Handle control commands
        });

        // Metrics channel handler
        dataChannelManager_->onChannelMessage("metrics", [](const std::vector<uint8_t>& data) {
            std::string message(data.begin(), data.end());
            std::cout << "[Metrics] Received: " << message << std::endl;
            // TODO: Process metrics data
        });

        // Logs channel handler
        dataChannelManager_->onChannelMessage("logs", [](const std::vector<uint8_t>& data) {
            std::string message(data.begin(), data.end());
            std::cout << "[Logs] Received: " << message << std::endl;
            // TODO: Store logs
        });

        // Alerts channel handler
        dataChannelManager_->onChannelMessage("alerts", [](const std::vector<uint8_t>& data) {
            std::string message(data.begin(), data.end());
            std::cout << "[Alerts] Received: " << message << std::endl;
            // TODO: Handle alerts
        });

        // Data channel handler
        dataChannelManager_->onChannelMessage("data", [](const std::vector<uint8_t>& data) {
            std::string message(data.begin(), data.end());
            std::cout << "[Data] Received: " << message << std::endl;
            // TODO: Store data in repository
        });
    }

    ServiceConfig config_;
    ServiceStatus status_;
    std::unique_ptr<DataChannelManager> dataChannelManager_;
};

// RemoteManagementService implementation
RemoteManagementService::RemoteManagementService(const ServiceConfig& config)
    : impl_(std::make_unique<RemoteManagementServiceImpl>(config)) {}

RemoteManagementService::~RemoteManagementService() = default;

bool RemoteManagementService::start() {
    return impl_->start();
}

void RemoteManagementService::stop() {
    impl_->stop();
}

ServiceStatus RemoteManagementService::getStatus() const {
    return impl_->getStatus();
}

int RemoteManagementService::runForeground() {
    return impl_->runForeground();
}

bool RemoteManagementService::install(const ServiceConfig& config) {
    std::cout << "Installing service: " << config.displayName << std::endl;

#ifdef _WIN32
    // TODO: Implement Windows service installation
    std::cerr << "Windows service installation not yet implemented" << std::endl;
    return false;
#elif __APPLE__
    // TODO: Implement macOS launchd installation
    std::cerr << "macOS launchd installation not yet implemented" << std::endl;
    return false;
#else
    // TODO: Implement Linux systemd service installation
    std::cerr << "Linux systemd installation not yet implemented" << std::endl;
    return false;
#endif
}

bool RemoteManagementService::uninstall(const ServiceConfig& config) {
    std::cout << "Uninstalling service: " << config.displayName << std::endl;

#ifdef _WIN32
    // TODO: Implement Windows service uninstallation
    std::cerr << "Windows service uninstallation not yet implemented" << std::endl;
    return false;
#elif __APPLE__
    // TODO: Implement macOS launchd uninstallation
    std::cerr << "macOS launchd uninstallation not yet implemented" << std::endl;
    return false;
#else
    // TODO: Implement Linux systemd service uninstallation
    std::cerr << "Linux systemd uninstallation not yet implemented" << std::endl;
    return false;
#endif
}

} // namespace webrtc_mgmt
