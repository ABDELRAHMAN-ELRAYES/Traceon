# Trigger Src sub-directories CMake files
add_subdirectory(analyzer-engine)
add_subdirectory(gui)
add_subdirectory(cli-parser)

# Create the main app(Entry Point)
add_executable(Traceon main.cpp)


target_link_libraries(Traceon PRIVATE 
    analyzer_engine_lib 
    gui_lib 
    cli_parser_lib
)
