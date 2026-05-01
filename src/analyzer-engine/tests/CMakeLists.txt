include(FetchContent)

# Fetch GTest
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(
  analyzer_engine_unit_tests
  unit/test_input_layer.cpp
  unit/test_packet_decoder.cpp
  unit/test_protocol_validator.cpp
  unit/test_report_builder.cpp
)

target_link_libraries(
  analyzer_engine_unit_tests
  PRIVATE
  analyzer_engine_lib
  GTest::gtest_main
)

add_executable(
  analyzer_engine_system_tests
  system/test_analyzer_engine.cpp
)

target_link_libraries(
  analyzer_engine_system_tests
  PRIVATE
  analyzer_engine_lib
  GTest::gtest_main
)

# GTest Discover
include(GoogleTest)
gtest_discover_tests(analyzer_engine_unit_tests)
gtest_discover_tests(analyzer_engine_system_tests)
