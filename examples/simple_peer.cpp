#include "webrtc_mgmt/data_channel_manager.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>

std::atomic<bool> g_running{true};

void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    g_running.store(false);
}

int main(int argc, char* argv[]) {
    std::cout << "Simple WebRTC Peer Example\n" << std::endl;

    // Set up signal handlers
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // Configure data channel manager
    webrtc_mgmt::DataChannelManagerConfig config;

    // For local testing without STUN/TURN
    config.peerConfig.disableIceServers = true;

    // For testing with STUN servers, uncomment:
    // config.peerConfig.disableIceServers = false;
    // config.peerConfig.iceServers = {
    //     {"stun:stun.l.google.com:19302", "", ""},
    //     {"stun:stun1.l.google.com:19302", "", ""}
    // };

    // Configure signaling
    config.signalingConfig.serverUrl = "ws://localhost:8080/signaling";
    config.signalingConfig.peerId = "simple-peer";

    // Configure channels
    config.channelLabels = {"control", "data"};

    // Create manager
    webrtc_mgmt::DataChannelManager manager(config);

    // Set up message handlers
    manager.onChannelMessage("control", [](const std::vector<uint8_t>& data) {
        std::string message(data.begin(), data.end());
        std::cout << "[Control] Received: " << message << std::endl;
    });

    manager.onChannelMessage("data", [](const std::vector<uint8_t>& data) {
        std::string message(data.begin(), data.end());
        std::cout << "[Data] Received: " << message << std::endl;
    });

    // Set up connection state handler
    manager.onConnectionStateChange([](const std::string& state) {
        std::cout << "Connection state: " << state << std::endl;
    });

    // Start the manager
    if (!manager.start()) {
        std::cerr << "Failed to start data channel manager" << std::endl;
        return 1;
    }

    std::cout << "Manager started. Press Ctrl+C to stop.\n" << std::endl;

    // Main loop
    int messageCounter = 0;
    while (g_running.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(5));

        // Send periodic messages on both channels
        if (manager.isChannelOpen("control")) {
            std::string controlMsg = "Control message #" + std::to_string(messageCounter);
            manager.send("control", controlMsg);
            std::cout << "Sent control message: " << controlMsg << std::endl;
        }

        if (manager.isChannelOpen("data")) {
            std::string dataMsg = "Data message #" + std::to_string(messageCounter);
            manager.send("data", dataMsg);
            std::cout << "Sent data message: " << dataMsg << std::endl;
        }

        messageCounter++;
    }

    // Stop the manager
    std::cout << "Stopping manager..." << std::endl;
    manager.stop();

    std::cout << "Manager stopped. Goodbye!" << std::endl;
    return 0;
}
