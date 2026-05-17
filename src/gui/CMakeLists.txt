find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Create the GUI library, including the third-party pugixml source
add_library(gui_lib STATIC 
    src/parser/report_parser.cpp
    src/utils/utils.cpp
    ${CMAKE_SOURCE_DIR}/third-party/libs/pugixml/pugixml.cpp
)

target_link_libraries(gui_lib PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Tell the rest of the project where the gui public headers are
target_include_directories(gui_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(gui_lib PUBLIC ${CMAKE_SOURCE_DIR}/third-party/libs)

# Create the test GUI application
add_executable(traceon_gui main.cpp)
target_link_libraries(traceon_gui PRIVATE gui_lib Qt6::Core Qt6::Gui Qt6::Widgets)
