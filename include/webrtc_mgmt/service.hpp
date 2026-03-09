#pragma once

#include <memory>
#include <string>
#include <functional>

namespace webrtc_mgmt {

/**
 * @brief Service status
 */
enum class ServiceStatus {
    STOPPED,
    STARTING,
    RUNNING,
    STOPPING,
    ERROR
};

/**
 * @brief Service configuration
 */
struct ServiceConfig {
    std::string serviceName = "WebRTCRemoteMgmt";
    std::string displayName = "WebRTC Remote Management Service";
    std::string description = "Remote monitoring and management service using WebRTC";

    // Platform-specific paths
    std::string configPath;
    std::string logPath;
    std::string pidPath;
};

/**
 * @brief Forward declaration
 */
class RemoteManagementServiceImpl;

/**
 * @brief Cross-platform service/daemon for remote management
 *
 * This class provides a cross-platform interface for running the
 * WebRTC remote management as a system service/daemon.
 */
class RemoteManagementService {
public:
    explicit RemoteManagementService(const ServiceConfig& config);
    ~RemoteManagementService();

    // Non-copyable, non-movable
    RemoteManagementService(const RemoteManagementService&) = delete;
    RemoteManagementService& operator=(const RemoteManagementService&) = delete;
    RemoteManagementService(RemoteManagementService&&) = delete;
    RemoteManagementService& operator=(RemoteManagementService&&) = delete;

    // Start the service
    bool start();

    // Stop the service
    void stop();

    // Get current status
    ServiceStatus getStatus() const;

    // Run as foreground application (for testing)
    int runForeground();

    // Install service (platform-specific)
    static bool install(const ServiceConfig& config);

    // Uninstall service (platform-specific)
    static bool uninstall(const ServiceConfig& config);

private:
    std::unique_ptr<RemoteManagementServiceImpl> impl_;
};

} // namespace webrtc_mgmt
