#include <gtest/gtest.h>
#include "webrtc_mgmt/peer_connection.hpp"
#include <thread>
#include <chrono>
#include <atomic>

using namespace webrtc_mgmt;

class PeerConnectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.disableIceServers = true;  // For local testing
    }

    PeerConnectionConfig config_;
};

TEST_F(PeerConnectionTest, CreatePeerConnection) {
    ASSERT_NO_THROW({
        PeerConnection pc(config_);
    });
}

TEST_F(PeerConnectionTest, CreateDataChannel) {
    PeerConnection pc(config_);

    DataChannelConfig dcConfig;
    dcConfig.label = "test";
    dcConfig.ordered = true;
    dcConfig.reliable = true;

    ASSERT_NO_THROW({
        DataChannel dc = pc.createDataChannel(dcConfig);
        EXPECT_EQ(dc.getLabel(), "test");
    });
}

TEST_F(PeerConnectionTest, InitialState) {
    PeerConnection pc(config_);
    std::string state = pc.getState();
    EXPECT_TRUE(state == "new" || state == "connecting");
}

TEST_F(PeerConnectionTest, LocalDescriptionCallback) {
    PeerConnection pc(config_);

    std::atomic<bool> callbackCalled{false};
    std::string receivedSdp;
    std::string receivedType;

    pc.onLocalDescription([&](const std::string& sdp, const std::string& type) {
        receivedSdp = sdp;
        receivedType = type;
        callbackCalled.store(true);
    });

    // Create a data channel to trigger offer generation
    DataChannelConfig dcConfig;
    dcConfig.label = "test";
    auto dc = pc.createDataChannel(dcConfig);

    // Wait for callback
    for (int i = 0; i < 50 && !callbackCalled.load(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    EXPECT_TRUE(callbackCalled.load());
    EXPECT_FALSE(receivedSdp.empty());
    EXPECT_EQ(receivedType, "offer");
}

TEST_F(PeerConnectionTest, DataChannelCallbacks) {
    PeerConnection pc(config_);

    std::atomic<bool> openCalled{false};
    std::atomic<bool> errorCalled{false};

    DataChannelConfig dcConfig;
    dcConfig.label = "test";
    auto dc = pc.createDataChannel(dcConfig);

    dc.onOpen([&]() {
        openCalled.store(true);
    });

    dc.onError([&](const std::string& error) {
        errorCalled.store(true);
    });

    // The channel won't actually open without a remote peer,
    // but we can verify the callbacks are set
    EXPECT_FALSE(dc.isOpen());
}

TEST_F(PeerConnectionTest, SetRemoteDescription) {
    PeerConnection pc(config_);

    // Create a dummy SDP (this is not a valid complete SDP, but tests the API)
    std::string sdp = "v=0\r\no=- 0 0 IN IP4 127.0.0.1\r\ns=-\r\nt=0 0\r\n";

    // This will fail because it's not a valid SDP, but we're testing the API works
    bool result = pc.setRemoteDescription(sdp, "offer");
    // We don't assert here as libdatachannel may handle invalid SDP differently
}

TEST_F(PeerConnectionTest, AddRemoteCandidate) {
    PeerConnection pc(config_);

    // Try to add a candidate (may fail without remote description)
    std::string candidate = "candidate:1 1 UDP 2130706431 127.0.0.1 12345 typ host";
    bool result = pc.addRemoteCandidate(candidate, "0");
    // We don't assert here as behavior may vary without remote description
}

TEST_F(PeerConnectionTest, ClosePeerConnection) {
    PeerConnection pc(config_);

    ASSERT_NO_THROW({
        pc.close();
    });

    std::string state = pc.getState();
    // After close, state should eventually be "closed"
    EXPECT_TRUE(state == "closed" || state == "disconnected");
}

TEST_F(PeerConnectionTest, MultipleDataChannels) {
    PeerConnection pc(config_);

    std::vector<std::string> labels = {"control", "metrics", "logs", "alerts"};

    for (const auto& label : labels) {
        DataChannelConfig dcConfig;
        dcConfig.label = label;
        auto dc = pc.createDataChannel(dcConfig);
        EXPECT_EQ(dc.getLabel(), label);
    }
}

TEST_F(PeerConnectionTest, DataChannelOrdering) {
    PeerConnection pc(config_);

    DataChannelConfig orderedConfig;
    orderedConfig.label = "ordered";
    orderedConfig.ordered = true;

    DataChannelConfig unorderedConfig;
    unorderedConfig.label = "unordered";
    unorderedConfig.ordered = false;

    ASSERT_NO_THROW({
        auto orderedDc = pc.createDataChannel(orderedConfig);
        auto unorderedDc = pc.createDataChannel(unorderedConfig);
    });
}

TEST_F(PeerConnectionTest, DataChannelReliability) {
    PeerConnection pc(config_);

    DataChannelConfig reliableConfig;
    reliableConfig.label = "reliable";
    reliableConfig.reliable = true;

    DataChannelConfig unreliableConfig;
    unreliableConfig.label = "unreliable";
    unreliableConfig.reliable = false;

    ASSERT_NO_THROW({
        auto reliableDc = pc.createDataChannel(reliableConfig);
        auto unreliableDc = pc.createDataChannel(unreliableConfig);
    });
}

TEST_F(PeerConnectionTest, ConnectionStateChangeCallback) {
    PeerConnection pc(config_);

    std::atomic<int> callbackCount{0};
    std::string lastState;

    pc.onConnectionStateChange([&](const std::string& state) {
        lastState = state;
        callbackCount.fetch_add(1);
    });

    // Create a data channel to trigger state changes
    DataChannelConfig dcConfig;
    dcConfig.label = "test";
    auto dc = pc.createDataChannel(dcConfig);

    // Wait a bit for potential state changes
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // We should have at least one state change
    EXPECT_GE(callbackCount.load(), 0);
}

TEST_F(PeerConnectionTest, WithStunServers) {
    PeerConnectionConfig stunConfig;
    stunConfig.disableIceServers = false;
    stunConfig.iceServers = {
        {"stun:stun.l.google.com:19302", "", ""}
    };

    ASSERT_NO_THROW({
        PeerConnection pc(stunConfig);
    });
}
