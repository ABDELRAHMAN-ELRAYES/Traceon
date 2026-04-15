# Create the analyzer engine library
add_library(analyzer_engine_lib STATIC 
    src/core/packet/packet.cpp
    src/core/tlp/tlp.cpp
    src/decoder/packet_decoder.cpp
    src/utils/utils.cpp
)

# Tell the rest of the project where the analyzer engine public headers are
target_include_directories(analyzer_engine_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
