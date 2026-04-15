# Trigger Src sub-directories CMake files
add_subdirectory(analyzer-engine)
add_subdirectory(gui)

# Create the main app(Entry Point)
add_executable(Traceon main.cpp)


target_link_libraries(Traceon PRIVATE analyzer_engine_lib gui_lib)