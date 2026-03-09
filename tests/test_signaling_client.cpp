#include <gtest/gtest.h>
#include "webrtc_mgmt/signaling_client.hpp"
#include <thread>
#include <chrono>
#include <atomic>

using namespace webrtc_mgmt;

class SignalingClientTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.serverUrl = "ws://localhost:8080/signaling";
        config_.peerId = "test-peer";
        config_.reconnectIntervalMs = 1000;
        config_.maxReconnectAttempts = 3;
    }

    SignalingConfig config_;
};

TEST_F(SignalingClientTest, CreateSignalingClient) {
    ASSERT_NO_THROW({
        SignalingClient client(config_);
    });
}

TEST_F(SignalingClientTest, ConnectToSignalingServer) {
    SignalingClient client(config_);

    // This will succeed with our mock implementation
    bool connected = client.connect();
    EXPECT_TRUE(connected);
    EXPECT_TRUE(client.isConnected());
}

TEST_F(SignalingClientTest, DisconnectFromSignalingServer) {
    SignalingClient client(config_);

    client.connect();
    EXPECT_TRUE(client.isConnected());

    client.disconnect();
    EXPECT_FALSE(client.isConnected());
}

TEST_F(SignalingClientTest, SendOffer) {
    SignalingClient client(config_);
    client.connect();

    std::string targetPeer = "remote-peer";
    std::string sdp = "v=0\r\no=- 0 0 IN IP4 127.0.0.1\r\n";

    bool result = client.sendOffer(targetPeer, sdp);
    EXPECT_TRUE(result);
}

TEST_F(SignalingClientTest, SendAnswer) {
    SignalingClient client(config_);
    client.connect();

    std::string targetPeer = "remote-peer";
    std::string sdp = "v=0\r\no=- 0 0 IN IP4 127.0.0.1\r\n";

    bool result = client.sendAnswer(targetPeer, sdp);
    EXPECT_TRUE(result);
}

TEST_F(SignalingClientTest, SendIceCandidate) {
    SignalingClient client(config_);
    client.connect();

    std::string targetPeer = "remote-peer";
    std::string candidate = "candidate:1 1 UDP 2130706431 127.0.0.1 12345 typ host";
    std::string mid = "0";

    bool result = client.sendIceCandidate(targetPeer, candidate, mid);
    EXPECT_TRUE(result);
}

TEST_F(SignalingClientTest, SendWithoutConnection) {
    SignalingClient client(config_);

    // Should fail because not connected
    bool result = client.sendOffer("peer", "sdp");
    EXPECT_FALSE(result);
}

TEST_F(SignalingClientTest, MessageCallback) {
    SignalingClient client(config_);

    std::atomic<bool> callbackCalled{false};
    SignalingMessage receivedMsg;

    client.onMessage([&](const SignalingMessage& msg) {
        receivedMsg = msg;
        callbackCalled.store(true);
    });

    client.connect();

    // The callback is set and will be called when messages arrive
    EXPECT_FALSE(callbackCalled.load());
}

TEST_F(SignalingClientTest, MessageTypes) {
    // Test that all message types can be created
    SignalingMessage offerMsg;
    offerMsg.type = SignalingMessageType::OFFER;
    offerMsg.sdp = "offer-sdp";
    EXPECT_EQ(offerMsg.type, SignalingMessageType::OFFER);

    SignalingMessage answerMsg;
    answerMsg.type = SignalingMessageType::ANSWER;
    answerMsg.sdp = "answer-sdp";
    EXPECT_EQ(answerMsg.type, SignalingMessageType::ANSWER);

    SignalingMessage candidateMsg;
    candidateMsg.type = SignalingMessageType::ICE_CANDIDATE;
    candidateMsg.candidate = "candidate-string";
    EXPECT_EQ(candidateMsg.type, SignalingMessageType::ICE_CANDIDATE);

    SignalingMessage errorMsg;
    errorMsg.type = SignalingMessageType::ERROR;
    errorMsg.error = "error-message";
    EXPECT_EQ(errorMsg.type, SignalingMessageType::ERROR);
}

TEST_F(SignalingClientTest, MultipleConnectDisconnect) {
    SignalingClient client(config_);

    for (int i = 0; i < 3; ++i) {
        EXPECT_TRUE(client.connect());
        EXPECT_TRUE(client.isConnected());

        client.disconnect();
        EXPECT_FALSE(client.isConnected());
    }
}

TEST_F(SignalingClientTest, MoveSemantics) {
    SignalingClient client1(config_);
    client1.connect();
    EXPECT_TRUE(client1.isConnected());

    // Move construction
    SignalingClient client2(std::move(client1));
    EXPECT_TRUE(client2.isConnected());

    // Move assignment
    SignalingClient client3(config_);
    client3 = std::move(client2);
    EXPECT_TRUE(client3.isConnected());
}
