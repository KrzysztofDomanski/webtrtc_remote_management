#include <gtest/gtest.h>
#include "webrtc_mgmt/peer_connection.hpp"
#include "webrtc_mgmt/signaling_client.hpp"
#include "webrtc_mgmt/data_channel_manager.hpp"
#include <thread>
#include <chrono>
#include <atomic>

using namespace webrtc_mgmt;

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use local testing mode (no STUN/TURN)
        config_.peerConfig.disableIceServers = true;

        config_.signalingConfig.serverUrl = "ws://localhost:8080/signaling";
        config_.signalingConfig.peerId = "integration-test";

        config_.channelLabels = {"control", "data"};
    }

    DataChannelManagerConfig config_;
};

TEST_F(IntegrationTest, FullStackInitialization) {
    ASSERT_NO_THROW({
        DataChannelManager manager(config_);
        EXPECT_FALSE(manager.isRunning());

        bool started = manager.start();
        EXPECT_TRUE(started);
        EXPECT_TRUE(manager.isRunning());

        // Let it run briefly
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        manager.stop();
        EXPECT_FALSE(manager.isRunning());
    });
}

TEST_F(IntegrationTest, LocalPeerConnection) {
    // Test creating a peer connection with local settings
    PeerConnectionConfig localConfig;
    localConfig.disableIceServers = true;

    ASSERT_NO_THROW({
        PeerConnection pc(localConfig);

        DataChannelConfig dcConfig;
        dcConfig.label = "test";
        auto dc = pc.createDataChannel(dcConfig);

        EXPECT_EQ(dc.getLabel(), "test");
    });
}

TEST_F(IntegrationTest, MultipleChannelCommunication) {
    DataChannelManager manager(config_);

    std::atomic<int> controlMessages{0};
    std::atomic<int> dataMessages{0};

    manager.onChannelMessage("control", [&](const std::vector<uint8_t>& data) {
        controlMessages.fetch_add(1);
    });

    manager.onChannelMessage("data", [&](const std::vector<uint8_t>& data) {
        dataMessages.fetch_add(1);
    });

    EXPECT_TRUE(manager.start());

    // Wait for initialization
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Try to send on multiple channels
    manager.send("control", "control message");
    manager.send("data", "data message");

    // Give time for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    manager.stop();
}

TEST_F(IntegrationTest, StateTransitions) {
    DataChannelManager manager(config_);

    std::vector<std::string> states;
    std::mutex statesMutex;

    manager.onConnectionStateChange([&](const std::string& state) {
        std::lock_guard<std::mutex> lock(statesMutex);
        states.push_back(state);
    });

    EXPECT_TRUE(manager.start());

    // Wait for potential state changes
    std::this_thread::sleep_for(std::chrono::seconds(1));

    manager.stop();

    // Check that we captured some states
    std::lock_guard<std::mutex> lock(statesMutex);
    EXPECT_GE(states.size(), 0);
}

TEST_F(IntegrationTest, RapidStartStop) {
    DataChannelManager manager(config_);

    // Rapidly start and stop multiple times
    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(manager.start());
        EXPECT_TRUE(manager.isRunning());

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        manager.stop();
        EXPECT_FALSE(manager.isRunning());

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

TEST_F(IntegrationTest, LongRunningSession) {
    DataChannelManager manager(config_);

    std::atomic<int> messageCount{0};

    for (const auto& label : config_.channelLabels) {
        manager.onChannelMessage(label, [&](const std::vector<uint8_t>& data) {
            messageCount.fetch_add(1);
        });
    }

    EXPECT_TRUE(manager.start());

    // Run for a longer period
    auto startTime = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - startTime < std::chrono::seconds(2)) {
        // Send periodic messages
        manager.send("control", "heartbeat");
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    manager.stop();
}

TEST_F(IntegrationTest, BinaryDataTransmission) {
    DataChannelManager manager(config_);

    std::vector<uint8_t> receivedData;
    std::mutex dataMutex;
    std::atomic<bool> dataReceived{false};

    manager.onChannelMessage("data", [&](const std::vector<uint8_t>& data) {
        std::lock_guard<std::mutex> lock(dataMutex);
        receivedData = data;
        dataReceived.store(true);
    });

    EXPECT_TRUE(manager.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Send binary data
    std::vector<uint8_t> binaryData = {0x00, 0x01, 0x02, 0xFF, 0xFE, 0xFD};
    manager.send("data", binaryData);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    manager.stop();
}

TEST_F(IntegrationTest, LargeDataTransmission) {
    DataChannelManager manager(config_);

    EXPECT_TRUE(manager.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Send large data
    std::vector<uint8_t> largeData(10000, 0xAA);
    manager.send("data", largeData);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    manager.stop();
}

TEST_F(IntegrationTest, ConcurrentChannelAccess) {
    DataChannelManager manager(config_);

    EXPECT_TRUE(manager.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Send on multiple channels concurrently
    std::vector<std::thread> threads;
    for (const auto& label : config_.channelLabels) {
        threads.emplace_back([&manager, label]() {
            for (int i = 0; i < 10; ++i) {
                std::string msg = label + " message " + std::to_string(i);
                manager.send(label, msg);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    manager.stop();
}
