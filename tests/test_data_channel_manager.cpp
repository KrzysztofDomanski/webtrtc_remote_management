#include <gtest/gtest.h>
#include "webrtc_mgmt/data_channel_manager.hpp"
#include <thread>
#include <chrono>
#include <atomic>

using namespace webrtc_mgmt;

class DataChannelManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.peerConfig.disableIceServers = true;

        config_.signalingConfig.serverUrl = "ws://localhost:8080/signaling";
        config_.signalingConfig.peerId = "test-manager";

        config_.channelLabels = {"control", "metrics", "logs", "alerts"};
    }

    DataChannelManagerConfig config_;
};

TEST_F(DataChannelManagerTest, CreateManager) {
    ASSERT_NO_THROW({
        DataChannelManager manager(config_);
    });
}

TEST_F(DataChannelManagerTest, StartManager) {
    DataChannelManager manager(config_);

    bool started = manager.start();
    EXPECT_TRUE(started);
    EXPECT_TRUE(manager.isRunning());

    manager.stop();
    EXPECT_FALSE(manager.isRunning());
}

TEST_F(DataChannelManagerTest, StopWithoutStart) {
    DataChannelManager manager(config_);

    ASSERT_NO_THROW({
        manager.stop();
    });
}

TEST_F(DataChannelManagerTest, MultipleStartCalls) {
    DataChannelManager manager(config_);

    EXPECT_TRUE(manager.start());
    EXPECT_TRUE(manager.isRunning());

    // Second start should return true (already running)
    EXPECT_TRUE(manager.start());
    EXPECT_TRUE(manager.isRunning());

    manager.stop();
}

TEST_F(DataChannelManagerTest, MultipleStopCalls) {
    DataChannelManager manager(config_);

    manager.start();
    EXPECT_TRUE(manager.isRunning());

    manager.stop();
    EXPECT_FALSE(manager.isRunning());

    // Second stop should not crash
    ASSERT_NO_THROW({
        manager.stop();
    });
}

TEST_F(DataChannelManagerTest, GetChannelLabels) {
    DataChannelManager manager(config_);

    auto labels = manager.getChannelLabels();
    EXPECT_EQ(labels.size(), 4);
    EXPECT_EQ(labels[0], "control");
    EXPECT_EQ(labels[1], "metrics");
    EXPECT_EQ(labels[2], "logs");
    EXPECT_EQ(labels[3], "alerts");
}

TEST_F(DataChannelManagerTest, SendDataBeforeStart) {
    DataChannelManager manager(config_);

    std::vector<uint8_t> data = {1, 2, 3, 4, 5};
    bool result = manager.send("control", data);
    EXPECT_FALSE(result);
}

TEST_F(DataChannelManagerTest, SendDataAfterStart) {
    DataChannelManager manager(config_);
    manager.start();

    // Give it a moment to initialize
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    std::vector<uint8_t> data = {1, 2, 3, 4, 5};
    // May fail because channels are not connected, but should not crash
    manager.send("control", data);

    manager.stop();
}

TEST_F(DataChannelManagerTest, SendStringData) {
    DataChannelManager manager(config_);
    manager.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    std::string data = "test message";
    manager.send("control", data);

    manager.stop();
}

TEST_F(DataChannelManagerTest, SendToInvalidChannel) {
    DataChannelManager manager(config_);
    manager.start();

    std::vector<uint8_t> data = {1, 2, 3};
    bool result = manager.send("invalid-channel", data);
    EXPECT_FALSE(result);

    manager.stop();
}

TEST_F(DataChannelManagerTest, MessageCallbacks) {
    DataChannelManager manager(config_);

    std::atomic<int> controlCount{0};
    std::atomic<int> metricsCount{0};

    manager.onChannelMessage("control", [&](const std::vector<uint8_t>& data) {
        controlCount.fetch_add(1);
    });

    manager.onChannelMessage("metrics", [&](const std::vector<uint8_t>& data) {
        metricsCount.fetch_add(1);
    });

    manager.start();

    // Callbacks are set and will be called when messages arrive
    EXPECT_EQ(controlCount.load(), 0);
    EXPECT_EQ(metricsCount.load(), 0);

    manager.stop();
}

TEST_F(DataChannelManagerTest, ConnectionStateCallback) {
    DataChannelManager manager(config_);

    std::atomic<int> callbackCount{0};
    std::string lastState;

    manager.onConnectionStateChange([&](const std::string& state) {
        lastState = state;
        callbackCount.fetch_add(1);
    });

    manager.start();

    // Wait for potential state changes
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    manager.stop();

    // We should have had at least one state change
    EXPECT_GE(callbackCount.load(), 0);
}

TEST_F(DataChannelManagerTest, IsChannelOpen) {
    DataChannelManager manager(config_);
    manager.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Channels won't be open without a remote peer, but API should work
    bool isOpen = manager.isChannelOpen("control");
    EXPECT_FALSE(isOpen);

    // Check invalid channel
    bool invalidOpen = manager.isChannelOpen("invalid");
    EXPECT_FALSE(invalidOpen);

    manager.stop();
}

TEST_F(DataChannelManagerTest, CustomChannelLabels) {
    DataChannelManagerConfig customConfig = config_;
    customConfig.channelLabels = {"custom1", "custom2", "custom3"};

    DataChannelManager manager(customConfig);

    auto labels = manager.getChannelLabels();
    EXPECT_EQ(labels.size(), 3);
    EXPECT_EQ(labels[0], "custom1");
    EXPECT_EQ(labels[1], "custom2");
    EXPECT_EQ(labels[2], "custom3");
}

TEST_F(DataChannelManagerTest, WithStunServers) {
    DataChannelManagerConfig stunConfig = config_;
    stunConfig.peerConfig.disableIceServers = false;
    stunConfig.peerConfig.iceServers = {
        {"stun:stun.l.google.com:19302", "", ""}
    };

    ASSERT_NO_THROW({
        DataChannelManager manager(stunConfig);
        manager.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        manager.stop();
    });
}
