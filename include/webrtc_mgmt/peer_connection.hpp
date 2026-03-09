#pragma once

#include <memory>
#include <string>
#include <functional>
#include <vector>

namespace webrtc_mgmt {

// Forward declarations
class PeerConnectionImpl;
class DataChannelImpl;

/**
 * @brief Configuration for ICE servers (STUN/TURN)
 */
struct IceServer {
    std::string url;              // e.g., "stun:stun.l.google.com:19302"
    std::string username;         // For TURN servers
    std::string credential;       // For TURN servers
};

/**
 * @brief Configuration for peer connection
 */
struct PeerConnectionConfig {
    std::vector<IceServer> iceServers;
    bool disableIceServers = false;  // For local testing without STUN/TURN
    int portRangeBegin = 0;
    int portRangeEnd = 0;
};

/**
 * @brief Data channel configuration
 */
struct DataChannelConfig {
    std::string label;
    bool ordered = true;
    bool reliable = true;
};

/**
 * @brief Callback types for data channel events
 */
using OnOpenCallback = std::function<void()>;
using OnClosedCallback = std::function<void()>;
using OnErrorCallback = std::function<void(const std::string& error)>;
using OnMessageCallback = std::function<void(const std::vector<uint8_t>& data)>;

/**
 * @brief Data channel wrapper
 */
class DataChannel {
public:
    DataChannel();
    ~DataChannel();

    // Non-copyable, movable
    DataChannel(const DataChannel&) = delete;
    DataChannel& operator=(const DataChannel&) = delete;
    DataChannel(DataChannel&&) noexcept;
    DataChannel& operator=(DataChannel&&) noexcept;

    // Send data
    bool send(const std::vector<uint8_t>& data);
    bool send(const std::string& data);

    // Check if channel is open
    bool isOpen() const;

    // Get channel label
    std::string getLabel() const;

    // Set callbacks
    void onOpen(OnOpenCallback callback);
    void onClosed(OnClosedCallback callback);
    void onError(OnErrorCallback callback);
    void onMessage(OnMessageCallback callback);

private:
    friend class PeerConnection;
    friend class PeerConnectionImpl;
    explicit DataChannel(std::shared_ptr<DataChannelImpl> impl);
    std::shared_ptr<DataChannelImpl> impl_;
};

/**
 * @brief Callback types for peer connection events
 */
using OnLocalDescriptionCallback = std::function<void(const std::string& sdp, const std::string& type)>;
using OnIceCandidateCallback = std::function<void(const std::string& candidate, const std::string& mid)>;
using OnDataChannelCallback = std::function<void(DataChannel channel)>;
using OnConnectionStateChangeCallback = std::function<void(const std::string& state)>;

/**
 * @brief WebRTC peer connection wrapper
 */
class PeerConnection {
public:
    explicit PeerConnection(const PeerConnectionConfig& config);
    ~PeerConnection();

    // Non-copyable, movable
    PeerConnection(const PeerConnection&) = delete;
    PeerConnection& operator=(const PeerConnection&) = delete;
    PeerConnection(PeerConnection&&) noexcept;
    PeerConnection& operator=(PeerConnection&&) noexcept;

    // Create data channel
    DataChannel createDataChannel(const DataChannelConfig& config);

    // Set remote description (SDP offer/answer)
    bool setRemoteDescription(const std::string& sdp, const std::string& type);

    // Add remote ICE candidate
    bool addRemoteCandidate(const std::string& candidate, const std::string& mid);

    // Create offer
    void createOffer();

    // Create answer
    void createAnswer();

    // Get connection state
    std::string getState() const;

    // Close connection
    void close();

    // Set callbacks
    void onLocalDescription(OnLocalDescriptionCallback callback);
    void onIceCandidate(OnIceCandidateCallback callback);
    void onDataChannel(OnDataChannelCallback callback);
    void onConnectionStateChange(OnConnectionStateChangeCallback callback);

private:
    std::shared_ptr<PeerConnectionImpl> impl_;
};

} // namespace webrtc_mgmt
