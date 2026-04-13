# Create the backend library
add_library(backend_lib STATIC 
    src/core/packet/packet.cpp
    src/core/tlp/tlp.cpp
    src/decoder/packet_decoder.cpp
    src/utils/utils.cpp
)

# Tell the rest of the project where the backend public headers are
target_include_directories(backend_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
