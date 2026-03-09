#include "webrtc_mgmt/signaling_client.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <queue>
#include <condition_variable>

using json = nlohmann::json;

namespace webrtc_mgmt {

/**
 * @brief Simple mock/local signaling implementation
 *
 * In a real implementation, this would use WebSockets to connect to a signaling server.
 * For now, we provide a simple local implementation that can be used for testing.
 * A production implementation would use a library like libwebsockets or similar.
 */
class SignalingClientImpl {
public:
    explicit SignalingClientImpl(const SignalingConfig& config)
        : config_(config)
        , connected_(false)
        , running_(false) {}

    ~SignalingClientImpl() {
        disconnect();
    }

    bool connect() {
        if (connected_.load()) {
            return true;
        }

        std::cout << "Signaling client connecting to: " << config_.serverUrl << std::endl;

        // TODO: Implement actual WebSocket connection
        // For now, simulate connection
        connected_.store(true);
        running_.store(true);

        // Start message processing thread
        messageThread_ = std::thread([this]() {
            processMessages();
        });

        return true;
    }

    void disconnect() {
        if (!connected_.load()) {
            return;
        }

        running_.store(false);
        connected_.store(false);

        // Wake up message thread
        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            queueCv_.notify_all();
        }

        if (messageThread_.joinable()) {
            messageThread_.join();
        }
    }

    bool isConnected() const {
        return connected_.load();
    }

    bool sendOffer(const std::string& targetPeerId, const std::string& sdp) {
        if (!connected_.load()) {
            return false;
        }

        json message;
        message["type"] = "offer";
        message["from"] = config_.peerId;
        message["to"] = targetPeerId;
        message["sdp"] = sdp;

        return sendMessage(message);
    }

    bool sendAnswer(const std::string& targetPeerId, const std::string& sdp) {
        if (!connected_.load()) {
            return false;
        }

        json message;
        message["type"] = "answer";
        message["from"] = config_.peerId;
        message["to"] = targetPeerId;
        message["sdp"] = sdp;

        return sendMessage(message);
    }

    bool sendIceCandidate(const std::string& targetPeerId,
                          const std::string& candidate,
                          const std::string& mid) {
        if (!connected_.load()) {
            return false;
        }

        json message;
        message["type"] = "candidate";
        message["from"] = config_.peerId;
        message["to"] = targetPeerId;
        message["candidate"] = candidate;
        message["candidateMid"] = mid;

        return sendMessage(message);
    }

    void setOnMessage(OnSignalingMessageCallback callback) {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        onMessageCb_ = std::move(callback);
    }

    // For testing: simulate receiving a message
    void simulateReceiveMessage(const json& message) {
        std::lock_guard<std::mutex> lock(queueMutex_);
        receivedMessages_.push(message);
        queueCv_.notify_one();
    }

private:
    bool sendMessage(const json& message) {
        try {
            std::string messageStr = message.dump();
            std::cout << "Signaling: Sending message: " << messageStr << std::endl;
            // TODO: Actually send via WebSocket
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Failed to send signaling message: " << e.what() << std::endl;
            return false;
        }
    }

    void processMessages() {
        while (running_.load()) {
            json message;
            {
                std::unique_lock<std::mutex> lock(queueMutex_);
                queueCv_.wait_for(lock, std::chrono::milliseconds(100), [this]() {
                    return !receivedMessages_.empty() || !running_.load();
                });

                if (!running_.load()) {
                    break;
                }

                if (receivedMessages_.empty()) {
                    continue;
                }

                message = receivedMessages_.front();
                receivedMessages_.pop();
            }

            handleMessage(message);
        }
    }

    void handleMessage(const json& message) {
        try {
            SignalingMessage msg;

            std::string typeStr = message.value("type", "");
            if (typeStr == "offer") {
                msg.type = SignalingMessageType::OFFER;
                msg.sdp = message.value("sdp", "");
                msg.sdpType = "offer";
            } else if (typeStr == "answer") {
                msg.type = SignalingMessageType::ANSWER;
                msg.sdp = message.value("sdp", "");
                msg.sdpType = "answer";
            } else if (typeStr == "candidate") {
                msg.type = SignalingMessageType::ICE_CANDIDATE;
                msg.candidate = message.value("candidate", "");
                msg.candidateMid = message.value("candidateMid", "");
            } else if (typeStr == "error") {
                msg.type = SignalingMessageType::ERROR;
                msg.error = message.value("error", "");
            } else {
                msg.type = SignalingMessageType::UNKNOWN;
            }

            msg.peerId = message.value("from", "");

            std::lock_guard<std::mutex> lock(callbackMutex_);
            if (onMessageCb_) {
                onMessageCb_(msg);
            }
        } catch (const std::exception& e) {
            std::cerr << "Failed to handle signaling message: " << e.what() << std::endl;
        }
    }

    SignalingConfig config_;
    std::atomic<bool> connected_;
    std::atomic<bool> running_;
    std::thread messageThread_;
    std::queue<json> receivedMessages_;
    std::mutex queueMutex_;
    std::condition_variable queueCv_;
    std::mutex callbackMutex_;
    OnSignalingMessageCallback onMessageCb_;
};

// SignalingClient implementation
SignalingClient::SignalingClient(const SignalingConfig& config)
    : impl_(std::make_shared<SignalingClientImpl>(config)) {}

SignalingClient::~SignalingClient() = default;

SignalingClient::SignalingClient(SignalingClient&&) noexcept = default;
SignalingClient& SignalingClient::operator=(SignalingClient&&) noexcept = default;

bool SignalingClient::connect() {
    return impl_->connect();
}

void SignalingClient::disconnect() {
    impl_->disconnect();
}

bool SignalingClient::isConnected() const {
    return impl_->isConnected();
}

bool SignalingClient::sendOffer(const std::string& targetPeerId, const std::string& sdp) {
    return impl_->sendOffer(targetPeerId, sdp);
}

bool SignalingClient::sendAnswer(const std::string& targetPeerId, const std::string& sdp) {
    return impl_->sendAnswer(targetPeerId, sdp);
}

bool SignalingClient::sendIceCandidate(const std::string& targetPeerId,
                                        const std::string& candidate,
                                        const std::string& mid) {
    return impl_->sendIceCandidate(targetPeerId, candidate, mid);
}

void SignalingClient::onMessage(OnSignalingMessageCallback callback) {
    impl_->setOnMessage(std::move(callback));
}

} // namespace webrtc_mgmt
