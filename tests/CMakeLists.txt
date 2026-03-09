# Test executable
add_executable(webrtc_mgmt_tests
    test_peer_connection.cpp
    test_signaling_client.cpp
    test_data_channel_manager.cpp
    test_integration.cpp
    test_main.cpp
)

target_link_libraries(webrtc_mgmt_tests
    PRIVATE
        webrtc_mgmt_core
        GTest::gtest
        GTest::gtest_main
)

# Discover and register tests with CTest
gtest_discover_tests(webrtc_mgmt_tests)
