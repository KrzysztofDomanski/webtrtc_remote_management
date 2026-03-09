#include "webrtc_mgmt/data_channel_manager.hpp"
#include <iostream>
#include <algorithm>

namespace webrtc_mgmt {

DataChannelManager::DataChannelManager(const DataChannelManagerConfig& config)
    : config_(config)
    , running_(false) {}

DataChannelManager::~DataChannelManager() {
    stop();
}

bool DataChannelManager::start() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (running_) {
        return true;
    }

    std::cout << "Starting DataChannelManager..." << std::endl;

    // Create peer connection
    setupPeerConnection();

    // Create signaling client
    setupSignaling();

    // Connect to signaling server
    if (!signalingClient_->connect()) {
        std::cerr << "Failed to connect to signaling server" << std::endl;
        return false;
    }

    // Create data channels
    createDataChannels();

    running_ = true;
    std::cout << "DataChannelManager started successfully" << std::endl;
    return true;
}

void DataChannelManager::stop() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!running_) {
        return;
    }

    std::cout << "Stopping DataChannelManager..." << std::endl;

    running_ = false;

    if (signalingClient_) {
        signalingClient_->disconnect();
    }

    if (peerConnection_) {
        peerConnection_->close();
    }

    dataChannels_.clear();
    messageCallbacks_.clear();

    std::cout << "DataChannelManager stopped" << std::endl;
}

bool DataChannelManager::isRunning() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return running_;
}

bool DataChannelManager::send(const std::string& channelLabel, const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = dataChannels_.find(channelLabel);
    if (it == dataChannels_.end()) {
        std::cerr << "Channel not found: " << channelLabel << std::endl;
        return false;
    }

    return it->second.send(data);
}

bool DataChannelManager::send(const std::string& channelLabel, const std::string& data) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = dataChannels_.find(channelLabel);
    if (it == dataChannels_.end()) {
        std::cerr << "Channel not found: " << channelLabel << std::endl;
        return false;
    }

    return it->second.send(data);
}

void DataChannelManager::onChannelMessage(const std::string& channelLabel, OnMessageCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    messageCallbacks_[channelLabel] = std::move(callback);
}

void DataChannelManager::onConnectionStateChange(OnConnectionStateChangeCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (peerConnection_) {
        peerConnection_->onConnectionStateChange(std::move(callback));
    }
}

std::vector<std::string> DataChannelManager::getChannelLabels() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_.channelLabels;
}

bool DataChannelManager::isChannelOpen(const std::string& channelLabel) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = dataChannels_.find(channelLabel);
    if (it == dataChannels_.end()) {
        return false;
    }

    return it->second.isOpen();
}

void DataChannelManager::setupPeerConnection() {
    peerConnection_ = std::make_unique<PeerConnection>(config_.peerConfig);

    // Set up local description callback
    peerConnection_->onLocalDescription([this](const std::string& sdp, const std::string& type) {
        std::cout << "Local description (" << type << "): " << sdp.substr(0, 50) << "..." << std::endl;

        // Send to signaling server
        if (signalingClient_ && signalingClient_->isConnected()) {
            if (type == "offer") {
                signalingClient_->sendOffer("remote-peer", sdp);
            } else if (type == "answer") {
                signalingClient_->sendAnswer("remote-peer", sdp);
            }
        }
    });

    // Set up ICE candidate callback
    peerConnection_->onIceCandidate([this](const std::string& candidate, const std::string& mid) {
        std::cout << "ICE candidate: " << candidate.substr(0, 50) << "..." << std::endl;

        // Send to signaling server
        if (signalingClient_ && signalingClient_->isConnected()) {
            signalingClient_->sendIceCandidate("remote-peer", candidate, mid);
        }
    });

    // Set up data channel callback (for incoming channels)
    peerConnection_->onDataChannel([this](DataChannel channel) {
        std::string label = channel.getLabel();
        std::cout << "Received data channel: " << label << std::endl;

        // Set up message callback if configured
        auto it = messageCallbacks_.find(label);
        if (it != messageCallbacks_.end()) {
            channel.onMessage(it->second);
        }

        channel.onOpen([label]() {
            std::cout << "Channel opened: " << label << std::endl;
        });

        channel.onClosed([label]() {
            std::cout << "Channel closed: " << label << std::endl;
        });

        channel.onError([label](const std::string& error) {
            std::cerr << "Channel error [" << label << "]: " << error << std::endl;
        });

        dataChannels_[label] = std::move(channel);
    });

    // Set up connection state callback
    peerConnection_->onConnectionStateChange([](const std::string& state) {
        std::cout << "Connection state: " << state << std::endl;
    });
}

void DataChannelManager::setupSignaling() {
    signalingClient_ = std::make_unique<SignalingClient>(config_.signalingConfig);

    // Set up message callback
    signalingClient_->onMessage([this](const SignalingMessage& message) {
        handleSignalingMessage(message);
    });
}

void DataChannelManager::handleSignalingMessage(const SignalingMessage& message) {
    std::cout << "Received signaling message from peer: " << message.peerId << std::endl;

    switch (message.type) {
        case SignalingMessageType::OFFER:
            std::cout << "Received offer" << std::endl;
            if (peerConnection_) {
                peerConnection_->setRemoteDescription(message.sdp, message.sdpType);
                peerConnection_->createAnswer();
            }
            break;

        case SignalingMessageType::ANSWER:
            std::cout << "Received answer" << std::endl;
            if (peerConnection_) {
                peerConnection_->setRemoteDescription(message.sdp, message.sdpType);
            }
            break;

        case SignalingMessageType::ICE_CANDIDATE:
            std::cout << "Received ICE candidate" << std::endl;
            if (peerConnection_) {
                peerConnection_->addRemoteCandidate(message.candidate, message.candidateMid);
            }
            break;

        case SignalingMessageType::ERROR:
            std::cerr << "Signaling error: " << message.error << std::endl;
            break;

        default:
            std::cerr << "Unknown signaling message type" << std::endl;
            break;
    }
}

void DataChannelManager::createDataChannels() {
    for (const auto& label : config_.channelLabels) {
        DataChannelConfig dcConfig;
        dcConfig.label = label;
        dcConfig.ordered = true;
        dcConfig.reliable = true;

        auto channel = peerConnection_->createDataChannel(dcConfig);

        // Set up callbacks
        channel.onOpen([label]() {
            std::cout << "Channel opened: " << label << std::endl;
        });

        channel.onClosed([label]() {
            std::cout << "Channel closed: " << label << std::endl;
        });

        channel.onError([label](const std::string& error) {
            std::cerr << "Channel error [" << label << "]: " << error << std::endl;
        });

        // Set up message callback if configured
        auto it = messageCallbacks_.find(label);
        if (it != messageCallbacks_.end()) {
            channel.onMessage(it->second);
        }

        dataChannels_[label] = std::move(channel);
        std::cout << "Created data channel: " << label << std::endl;
    }
}

} // namespace webrtc_mgmt
