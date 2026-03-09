# Example applications
add_executable(simple_peer
    simple_peer.cpp
)

target_link_libraries(simple_peer
    PRIVATE
        webrtc_mgmt_core
)
