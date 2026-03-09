#pragma once

#include <memory>
#include <string>
#include <functional>
#include <map>

namespace webrtc_mgmt {

/**
 * @brief Signaling message types
 */
enum class SignalingMessageType {
    OFFER,
    ANSWER,
    ICE_CANDIDATE,
    ERROR,
    UNKNOWN
};

/**
 * @brief Signaling message
 */
struct SignalingMessage {
    SignalingMessageType type;
    std::string peerId;
    std::string sdp;
    std::string sdpType;
    std::string candidate;
    std::string candidateMid;
    std::string error;
};

/**
 * @brief Callback for received signaling messages
 */
using OnSignalingMessageCallback = std::function<void(const SignalingMessage& message)>;

/**
 * @brief Signaling client configuration
 */
struct SignalingConfig {
    std::string serverUrl;        // WebSocket URL of signaling server
    std::string peerId;           // Unique identifier for this peer
    int reconnectIntervalMs = 5000;
    int maxReconnectAttempts = 10;
};

/**
 * @brief Forward declaration
 */
class SignalingClientImpl;

/**
 * @brief Signaling client for SDP exchange
 *
 * This client communicates with a signaling server to exchange
 * SDP offers/answers and ICE candidates between peers.
 */
class SignalingClient {
public:
    explicit SignalingClient(const SignalingConfig& config);
    ~SignalingClient();

    // Non-copyable, movable
    SignalingClient(const SignalingClient&) = delete;
    SignalingClient& operator=(const SignalingClient&) = delete;
    SignalingClient(SignalingClient&&) noexcept;
    SignalingClient& operator=(SignalingClient&&) noexcept;

    // Connect to signaling server
    bool connect();

    // Disconnect from signaling server
    void disconnect();

    // Check if connected
    bool isConnected() const;

    // Send offer
    bool sendOffer(const std::string& targetPeerId, const std::string& sdp);

    // Send answer
    bool sendAnswer(const std::string& targetPeerId, const std::string& sdp);

    // Send ICE candidate
    bool sendIceCandidate(const std::string& targetPeerId,
                          const std::string& candidate,
                          const std::string& mid);

    // Set callback for received messages
    void onMessage(OnSignalingMessageCallback callback);

private:
    std::shared_ptr<SignalingClientImpl> impl_;
};

} // namespace webrtc_mgmt
