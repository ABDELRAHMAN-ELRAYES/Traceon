# Create the CLI parser library
add_library(cli_parser_lib STATIC 
    src/cli_parser.cpp
)

# Tell the rest of the project where the CLI parser public headers are
target_include_directories(cli_parser_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# CLI Parser depends on ReportFormat enum in ReportBuilder
target_link_libraries(cli_parser_lib PUBLIC analyzer_engine_lib)