# WebRTC Remote Management

A cross-platform C++17 remote monitoring and management service using WebRTC data channels.

## Features

- WebRTC peer-to-peer communication using libdatachannel
- Signaling server support for SDP exchange
- STUN/TURN server support for NAT traversal
- Local operation without STUN/TURN for testing
- Multiple data channels for different data types
- Cross-platform support (Windows, Linux, macOS)
- Service/daemon mode support
- Comprehensive test coverage

## Dependencies

- C++17 compiler
- CMake 3.15 or higher
- libdatachannel (automatically fetched)
- mbedtls (included with libdatachannel)
- nlohmann/json (automatically fetched)
- Google Test (automatically fetched for tests)

## Building

```bash
# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build
cmake --build .

# Run tests
ctest --output-on-failure
```

### Build Options

- `BUILD_TESTS` (default: ON) - Build test suite
- `BUILD_EXAMPLES` (default: ON) - Build example applications

## Architecture

The project follows a layered architecture:

1. **WebRTC Layer**: Manages peer connections and data channels
2. **Signaling Layer**: Handles SDP exchange with signaling server
3. **Service Layer**: Business logic and data management
4. **Repository Layer**: Data persistence (future SQLite integration)

## Running as a Service

### Linux (systemd)
```bash
sudo systemctl enable webrtc-remote-mgmt
sudo systemctl start webrtc-remote-mgmt
```

### Windows
```bash
sc create WebRTCRemoteMgmt binPath= "path\to\service.exe"
sc start WebRTCRemoteMgmt
```

### macOS (launchd)
```bash
sudo launchctl load /Library/LaunchDaemons/com.webrtc.remotemgmt.plist
```

## License

MIT
