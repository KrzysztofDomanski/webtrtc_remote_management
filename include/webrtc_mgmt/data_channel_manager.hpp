#pragma once

#include "peer_connection.hpp"
#include "signaling_client.hpp"
#include <memory>
#include <string>
#include <map>
#include <mutex>

namespace webrtc_mgmt {

/**
 * @brief Configuration for data channel manager
 */
struct DataChannelManagerConfig {
    PeerConnectionConfig peerConfig;
    SignalingConfig signalingConfig;

    // Channel labels for different data types
    std::vector<std::string> channelLabels = {
        "control",
        "metrics",
        "logs",
        "alerts"
    };
};

/**
 * @brief Manages multiple data channels for different data types
 *
 * This class provides a high-level interface for managing WebRTC
 * connections with multiple data channels for different purposes
 * (e.g., control commands, metrics, logs, alerts).
 */
class DataChannelManager {
public:
    explicit DataChannelManager(const DataChannelManagerConfig& config);
    ~DataChannelManager();

    // Non-copyable, non-movable (contains mutexes)
    DataChannelManager(const DataChannelManager&) = delete;
    DataChannelManager& operator=(const DataChannelManager&) = delete;
    DataChannelManager(DataChannelManager&&) = delete;
    DataChannelManager& operator=(DataChannelManager&&) = delete;

    // Start the manager (connect signaling, setup peer connection)
    bool start();

    // Stop the manager
    void stop();

    // Check if running
    bool isRunning() const;

    // Send data on a specific channel
    bool send(const std::string& channelLabel, const std::vector<uint8_t>& data);
    bool send(const std::string& channelLabel, const std::string& data);

    // Set callback for receiving data on a channel
    void onChannelMessage(const std::string& channelLabel, OnMessageCallback callback);

    // Set callback for connection state changes
    void onConnectionStateChange(OnConnectionStateChangeCallback callback);

    // Get list of available channel labels
    std::vector<std::string> getChannelLabels() const;

    // Check if a channel is open
    bool isChannelOpen(const std::string& channelLabel) const;

private:
    void setupPeerConnection();
    void setupSignaling();
    void handleSignalingMessage(const SignalingMessage& message);
    void createDataChannels();

    DataChannelManagerConfig config_;
    std::unique_ptr<PeerConnection> peerConnection_;
    std::unique_ptr<SignalingClient> signalingClient_;
    std::map<std::string, DataChannel> dataChannels_;
    std::map<std::string, OnMessageCallback> messageCallbacks_;
    mutable std::mutex mutex_;
    bool running_;
};

} // namespace webrtc_mgmt
