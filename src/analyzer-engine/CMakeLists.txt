# Create the analyzer engine library
add_library(analyzer_engine_lib STATIC 
    src/core/packet/packet.cpp
    src/core/tlp/tlp.cpp
    src/decoder/packet_decoder.cpp
    src/utils/utils.cpp
    src/input-layer/input_layer.cpp
    src/input-layer/input_exception.cpp
    src/validator/protocol_validator.cpp
    src/report-builder/report_builder.cpp
    src/analyzer_engine.cpp
)

# Tell the rest of the project where the analyzer engine public headers are
target_include_directories(analyzer_engine_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Add third-party to the include path
target_include_directories(analyzer_engine_lib PUBLIC ${CMAKE_SOURCE_DIR}/third-party/libs)
