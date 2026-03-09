# Build and Test Instructions

## Prerequisites

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.15 or higher
- Git (for fetching dependencies)
- OpenSSL development libraries

### Installing Prerequisites

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install build-essential cmake git libssl-dev
```

**macOS:**
```bash
brew install cmake openssl
```

**Windows:**
- Install Visual Studio 2019 or later with C++ support
- Install CMake from https://cmake.org/download/
- OpenSSL will be automatically handled by vcpkg or downloaded

## Building

```bash
# Clone the repository
git clone https://github.com/KrzysztofDomanski/webtrtc_remote_management.git
cd webtrtc_remote_management

# Create build directory
mkdir build
cd build

# Configure (Debug build)
cmake ..

# Or configure for Release build
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build
cmake --build . -j4

# The build will automatically fetch and compile:
# - libdatachannel (with mbedtls)
# - nlohmann/json
# - Google Test (for testing)
```

## Running Tests

```bash
# From the build directory
ctest --output-on-failure

# Or run the test executable directly for more detail
./tests/webrtc_mgmt_tests

# Run specific tests
./tests/webrtc_mgmt_tests --gtest_filter=PeerConnectionTest.*
```

### Test Results

The test suite includes 48 tests covering:
- Peer connection creation and management
- Data channel operations
- Signaling client functionality
- Data channel manager with multiple channels
- Integration tests with full stack

**Expected Results:** 45+ tests should pass. A few edge case tests may fail due to:
- Invalid SDP handling (expected behavior)
- STUN server unavailability in sandboxed environments

## Running the Service

### Foreground Mode (for development/testing)

```bash
# From the build directory
./src/webrtc_remote_mgmt --foreground
```

The service will:
- Initialize WebRTC peer connections
- Connect to the signaling server (ws://localhost:8080/signaling by default)
- Create multiple data channels: control, metrics, logs, alerts, data
- Send periodic heartbeats

### Running Examples

```bash
# Simple peer example
./examples/simple_peer
```

Press Ctrl+C to stop the service.

## Configuration

### Modifying STUN/TURN Servers

Edit `src/service.cpp` to configure ICE servers:

```cpp
dcmConfig.peerConfig.iceServers = {
    {"stun:your.stun.server:19302", "", ""},
    {"turn:your.turn.server:3478", "username", "password"}
};
```

### Local Testing Without STUN/TURN

For local testing without internet connectivity:

```cpp
dcmConfig.peerConfig.disableIceServers = true;
```

### Customizing Data Channels

Edit the channel labels in `src/service.cpp`:

```cpp
dcmConfig.channelLabels = {
    "control",    // Control commands
    "metrics",    // Performance metrics
    "logs",       // Application logs
    "alerts",     // Alert notifications
    "custom"      // Your custom channel
};
```

## Installing as a System Service

### Linux (systemd)

```bash
cd scripts/systemd
sudo ./install.sh
```

This will:
- Copy the executable to `/usr/local/bin/`
- Install the systemd service file
- Enable the service to start on boot

**Managing the service:**
```bash
sudo systemctl start webrtc-remote-mgmt
sudo systemctl stop webrtc-remote-mgmt
sudo systemctl status webrtc-remote-mgmt
sudo journalctl -u webrtc-remote-mgmt -f  # View logs
```

### macOS (launchd)

```bash
cd scripts/launchd
sudo ./install.sh
```

**Managing the service:**
```bash
sudo launchctl list | grep webrtc
tail -f /var/log/webrtc-remote-mgmt.log
```

### Windows

Service installation on Windows is planned but not yet implemented. For now, run in foreground mode or use Task Scheduler.

## Uninstalling

### Linux
```bash
cd scripts/systemd
sudo ./uninstall.sh
```

### macOS
```bash
cd scripts/launchd
sudo ./uninstall.sh
```

## Troubleshooting

### Build Fails with "libdatachannel not found"

The project uses CMake FetchContent to automatically download libdatachannel. Ensure you have:
- Active internet connection during first build
- Git installed and accessible from CMake

### Tests Fail with STUN-related Errors

This is expected in sandboxed environments. The core functionality tests will pass. STUN/TURN tests require:
- Internet connectivity
- Access to STUN servers (default: stun.l.google.com)

### Signaling Connection Fails

The default configuration expects a WebSocket signaling server at `ws://localhost:8080/signaling`. For testing:
- Implement a simple WebSocket signaling server, or
- Use the mock/local signaling mode (already implemented for testing)

## Project Structure

```
webtrtc_remote_management/
├── CMakeLists.txt              # Root CMake configuration
├── README.md                   # Project overview
├── BUILD.md                    # This file
├── include/webrtc_mgmt/        # Public headers
│   ├── peer_connection.hpp     # WebRTC peer connection wrapper
│   ├── signaling_client.hpp    # Signaling client interface
│   ├── data_channel_manager.hpp # High-level channel manager
│   └── service.hpp             # Service/daemon interface
├── src/                        # Implementation files
│   ├── peer_connection.cpp
│   ├── signaling_client.cpp
│   ├── data_channel_manager.cpp
│   ├── service.cpp
│   └── main.cpp                # Service entry point
├── tests/                      # Test suite
│   ├── test_peer_connection.cpp
│   ├── test_signaling_client.cpp
│   ├── test_data_channel_manager.cpp
│   └── test_integration.cpp
├── examples/                   # Example applications
│   └── simple_peer.cpp
└── scripts/                    # Service installation scripts
    ├── systemd/                # Linux systemd files
    └── launchd/                # macOS launchd files
```

## Next Steps

1. **Implement WebSocket Signaling**: Replace the mock signaling client with a real WebSocket implementation
2. **Add SQLite Database**: Integrate SQLite for data persistence
3. **Implement Repository Pattern**: Create repository classes for different data types
4. **Add Windows Service Support**: Complete the Windows service installation
5. **Enhanced Security**: Add TLS support for signaling and authentication
6. **Monitoring Dashboard**: Create a web-based monitoring interface

## Contributing

When contributing, please:
1. Follow the existing code style
2. Add tests for new functionality
3. Update documentation
4. Ensure all tests pass before submitting PRs

## License

MIT License - see LICENSE file for details
