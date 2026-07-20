find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_library(ssui STATIC
    GridView.cpp
    GridView.h
    SpritePreviewPanel.cpp
    SpritePreviewPanel.h
    ToolPanel.cpp
    ToolPanel.h
    MainWindow.cpp
    MainWindow.h
    dialogs/ImportOptionsDialog.cpp
    dialogs/ImportOptionsDialog.h
)

target_include_directories(ssui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ssui PUBLIC
    sscore
    Qt6::Widgets
)

target_compile_features(ssui PUBLIC cxx_std_17)
