#include "webrtc_mgmt/peer_connection.hpp"
#include <rtc/rtc.hpp>
#include <memory>
#include <stdexcept>
#include <iostream>

namespace webrtc_mgmt {

// Implementation classes using libdatachannel
class DataChannelImpl {
public:
    explicit DataChannelImpl(std::shared_ptr<rtc::DataChannel> dc)
        : channel_(std::move(dc)) {
        if (!channel_) {
            throw std::runtime_error("Invalid data channel");
        }

        // Set up callbacks
        channel_->onOpen([this]() {
            if (onOpenCb_) onOpenCb_();
        });

        channel_->onClosed([this]() {
            if (onClosedCb_) onClosedCb_();
        });

        channel_->onError([this](const std::string& error) {
            if (onErrorCb_) onErrorCb_(error);
        });

        channel_->onMessage([this](auto data) {
            if (onMessageCb_) {
                if (std::holds_alternative<std::string>(data)) {
                    auto& str = std::get<std::string>(data);
                    std::vector<uint8_t> vec(str.begin(), str.end());
                    onMessageCb_(vec);
                } else {
                    onMessageCb_(std::get<std::vector<std::byte>>(data) |
                        [](auto b) { return static_cast<uint8_t>(b); });
                }
            }
        });
    }

    bool send(const std::vector<uint8_t>& data) {
        if (!channel_ || !channel_->isOpen()) {
            return false;
        }
        try {
            std::vector<std::byte> byteData;
            byteData.reserve(data.size());
            for (auto b : data) {
                byteData.push_back(static_cast<std::byte>(b));
            }
            channel_->send(byteData);
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Failed to send data: " << e.what() << std::endl;
            return false;
        }
    }

    bool send(const std::string& data) {
        if (!channel_ || !channel_->isOpen()) {
            return false;
        }
        try {
            channel_->send(data);
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Failed to send data: " << e.what() << std::endl;
            return false;
        }
    }

    bool isOpen() const {
        return channel_ && channel_->isOpen();
    }

    std::string getLabel() const {
        return channel_ ? channel_->label() : "";
    }

    void setOnOpen(OnOpenCallback callback) { onOpenCb_ = std::move(callback); }
    void setOnClosed(OnClosedCallback callback) { onClosedCb_ = std::move(callback); }
    void setOnError(OnErrorCallback callback) { onErrorCb_ = std::move(callback); }
    void setOnMessage(OnMessageCallback callback) { onMessageCb_ = std::move(callback); }

private:
    std::shared_ptr<rtc::DataChannel> channel_;
    OnOpenCallback onOpenCb_;
    OnClosedCallback onClosedCb_;
    OnErrorCallback onErrorCb_;
    OnMessageCallback onMessageCb_;
};

class PeerConnectionImpl {
public:
    explicit PeerConnectionImpl(const PeerConnectionConfig& config) {
        rtc::Configuration rtcConfig;

        // Configure ICE servers
        if (!config.disableIceServers) {
            for (const auto& server : config.iceServers) {
                rtc::IceServer iceServer;
                iceServer.hostname = server.url;
                if (!server.username.empty()) {
                    iceServer.username = server.username;
                }
                if (!server.credential.empty()) {
                    iceServer.password = server.credential;
                }
                rtcConfig.iceServers.push_back(iceServer);
            }
        }

        // Port range configuration
        if (config.portRangeBegin > 0 && config.portRangeEnd > 0) {
            rtcConfig.portRangeBegin = config.portRangeBegin;
            rtcConfig.portRangeEnd = config.portRangeEnd;
        }

        // Disable mDNS for better compatibility
        rtcConfig.disableAutoNegotiation = false;

        pc_ = std::make_shared<rtc::PeerConnection>(rtcConfig);

        // Set up callbacks
        pc_->onLocalDescription([this](rtc::Description desc) {
            if (onLocalDescCb_) {
                std::string sdp = std::string(desc);
                std::string type = desc.typeString();
                onLocalDescCb_(sdp, type);
            }
        });

        pc_->onLocalCandidate([this](rtc::Candidate cand) {
            if (onIceCandCb_) {
                std::string candidate = std::string(cand);
                std::string mid = cand.mid();
                onIceCandCb_(candidate, mid);
            }
        });

        pc_->onDataChannel([this](std::shared_ptr<rtc::DataChannel> dc) {
            if (onDataChannelCb_) {
                auto impl = std::make_shared<DataChannelImpl>(dc);
                DataChannel channel(impl);
                onDataChannelCb_(std::move(channel));
            }
        });

        pc_->onStateChange([this](rtc::PeerConnection::State state) {
            if (onStateCb_) {
                std::string stateStr;
                switch (state) {
                    case rtc::PeerConnection::State::New:
                        stateStr = "new";
                        break;
                    case rtc::PeerConnection::State::Connecting:
                        stateStr = "connecting";
                        break;
                    case rtc::PeerConnection::State::Connected:
                        stateStr = "connected";
                        break;
                    case rtc::PeerConnection::State::Disconnected:
                        stateStr = "disconnected";
                        break;
                    case rtc::PeerConnection::State::Failed:
                        stateStr = "failed";
                        break;
                    case rtc::PeerConnection::State::Closed:
                        stateStr = "closed";
                        break;
                }
                onStateCb_(stateStr);
            }
        });
    }

    std::shared_ptr<DataChannelImpl> createDataChannel(const DataChannelConfig& config) {
        rtc::DataChannelInit init;
        init.reliability.unordered = !config.ordered;

        if (!config.reliable) {
            init.reliability.type = rtc::Reliability::Type::Rexmit;
            init.reliability.rexmit = 0;
        }

        auto dc = pc_->createDataChannel(config.label, init);
        return std::make_shared<DataChannelImpl>(dc);
    }

    bool setRemoteDescription(const std::string& sdp, const std::string& type) {
        try {
            rtc::Description desc(sdp, type);
            pc_->setRemoteDescription(desc);
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Failed to set remote description: " << e.what() << std::endl;
            return false;
        }
    }

    bool addRemoteCandidate(const std::string& candidate, const std::string& mid) {
        try {
            rtc::Candidate cand(candidate, mid);
            pc_->addRemoteCandidate(cand);
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Failed to add remote candidate: " << e.what() << std::endl;
            return false;
        }
    }

    void createOffer() {
        // Offer will be generated automatically by libdatachannel
    }

    void createAnswer() {
        // Answer will be generated automatically when remote description is set
    }

    std::string getState() const {
        if (!pc_) return "closed";

        switch (pc_->state()) {
            case rtc::PeerConnection::State::New:
                return "new";
            case rtc::PeerConnection::State::Connecting:
                return "connecting";
            case rtc::PeerConnection::State::Connected:
                return "connected";
            case rtc::PeerConnection::State::Disconnected:
                return "disconnected";
            case rtc::PeerConnection::State::Failed:
                return "failed";
            case rtc::PeerConnection::State::Closed:
                return "closed";
            default:
                return "unknown";
        }
    }

    void close() {
        if (pc_) {
            pc_->close();
        }
    }

    void setOnLocalDescription(OnLocalDescriptionCallback callback) {
        onLocalDescCb_ = std::move(callback);
    }

    void setOnIceCandidate(OnIceCandidateCallback callback) {
        onIceCandCb_ = std::move(callback);
    }

    void setOnDataChannel(OnDataChannelCallback callback) {
        onDataChannelCb_ = std::move(callback);
    }

    void setOnConnectionStateChange(OnConnectionStateChangeCallback callback) {
        onStateCb_ = std::move(callback);
    }

private:
    std::shared_ptr<rtc::PeerConnection> pc_;
    OnLocalDescriptionCallback onLocalDescCb_;
    OnIceCandidateCallback onIceCandCb_;
    OnDataChannelCallback onDataChannelCb_;
    OnConnectionStateChangeCallback onStateCb_;
};

// DataChannel implementation
DataChannel::DataChannel() = default;

DataChannel::DataChannel(std::shared_ptr<DataChannelImpl> impl)
    : impl_(std::move(impl)) {}

DataChannel::~DataChannel() = default;

DataChannel::DataChannel(DataChannel&&) noexcept = default;
DataChannel& DataChannel::operator=(DataChannel&&) noexcept = default;

bool DataChannel::send(const std::vector<uint8_t>& data) {
    return impl_ && impl_->send(data);
}

bool DataChannel::send(const std::string& data) {
    return impl_ && impl_->send(data);
}

bool DataChannel::isOpen() const {
    return impl_ && impl_->isOpen();
}

std::string DataChannel::getLabel() const {
    return impl_ ? impl_->getLabel() : "";
}

void DataChannel::onOpen(OnOpenCallback callback) {
    if (impl_) impl_->setOnOpen(std::move(callback));
}

void DataChannel::onClosed(OnClosedCallback callback) {
    if (impl_) impl_->setOnClosed(std::move(callback));
}

void DataChannel::onError(OnErrorCallback callback) {
    if (impl_) impl_->setOnError(std::move(callback));
}

void DataChannel::onMessage(OnMessageCallback callback) {
    if (impl_) impl_->setOnMessage(std::move(callback));
}

// PeerConnection implementation
PeerConnection::PeerConnection(const PeerConnectionConfig& config)
    : impl_(std::make_shared<PeerConnectionImpl>(config)) {}

PeerConnection::~PeerConnection() = default;

PeerConnection::PeerConnection(PeerConnection&&) noexcept = default;
PeerConnection& PeerConnection::operator=(PeerConnection&&) noexcept = default;

DataChannel PeerConnection::createDataChannel(const DataChannelConfig& config) {
    auto impl = impl_->createDataChannel(config);
    return DataChannel(impl);
}

bool PeerConnection::setRemoteDescription(const std::string& sdp, const std::string& type) {
    return impl_->setRemoteDescription(sdp, type);
}

bool PeerConnection::addRemoteCandidate(const std::string& candidate, const std::string& mid) {
    return impl_->addRemoteCandidate(candidate, mid);
}

void PeerConnection::createOffer() {
    impl_->createOffer();
}

void PeerConnection::createAnswer() {
    impl_->createAnswer();
}

std::string PeerConnection::getState() const {
    return impl_->getState();
}

void PeerConnection::close() {
    impl_->close();
}

void PeerConnection::onLocalDescription(OnLocalDescriptionCallback callback) {
    impl_->setOnLocalDescription(std::move(callback));
}

void PeerConnection::onIceCandidate(OnIceCandidateCallback callback) {
    impl_->setOnIceCandidate(std::move(callback));
}

void PeerConnection::onDataChannel(OnDataChannelCallback callback) {
    impl_->setOnDataChannel(std::move(callback));
}

void PeerConnection::onConnectionStateChange(OnConnectionStateChangeCallback callback) {
    impl_->setOnConnectionStateChange(std::move(callback));
}

} // namespace webrtc_mgmt
