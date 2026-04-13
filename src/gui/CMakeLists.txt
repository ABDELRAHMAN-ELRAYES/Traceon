# Create the GUI library
add_library(gui_lib STATIC 
    src/gui_placeholder.cpp
)

# Tell the rest of the project where the gui public headers are
target_include_directories(gui_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
