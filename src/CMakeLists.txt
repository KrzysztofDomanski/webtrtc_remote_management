# Core library
add_library(webrtc_mgmt_core
    peer_connection.cpp
    signaling_client.cpp
    data_channel_manager.cpp
    service.cpp
)

target_include_directories(webrtc_mgmt_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(webrtc_mgmt_core
    PUBLIC
        datachannel
        nlohmann_json::nlohmann_json
        Threads::Threads
)

# Main executable
add_executable(webrtc_remote_mgmt
    main.cpp
)

target_link_libraries(webrtc_remote_mgmt
    PRIVATE
        webrtc_mgmt_core
)

# Install targets
install(TARGETS webrtc_mgmt_core webrtc_remote_mgmt
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)
