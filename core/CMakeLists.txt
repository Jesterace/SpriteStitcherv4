find_package(Qt6 REQUIRED COMPONENTS Core Gui)

add_library(sscore STATIC
    src/color/Lab.cpp
    src/color/ColorDistance.cpp
    src/dmc/DmcTable.cpp
    src/dmc/DmcMatcher.cpp
    src/dmc/DmcTableData.cpp
    src/image/ImageLoader.cpp
    src/quantize/ColorReducer.cpp
    src/pattern/SymbolAssigner.cpp
    src/pattern/PatternModel.cpp
    # Q_OBJECT headers must be listed explicitly: they live under include/,
    # outside src/, so AUTOMOC's same-directory header scan won't find them
    # otherwise.
    include/ss/core/pattern/PatternModel.h
)

target_include_directories(sscore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(sscore PUBLIC
    Qt6::Core
    Qt6::Gui
)

target_compile_features(sscore PUBLIC cxx_std_17)

add_subdirectory(tests)
