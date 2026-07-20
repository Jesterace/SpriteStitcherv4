find_package(Qt6 REQUIRED COMPONENTS Core Gui)

# libharu has neither a CMake package config nor a pkg-config file on most
# distros, so find it the old-fashioned way.
find_library(HPDF_LIBRARY NAMES hpdf)
find_path(HPDF_INCLUDE_DIR NAMES hpdf.h)
if(NOT HPDF_LIBRARY OR NOT HPDF_INCLUDE_DIR)
    message(FATAL_ERROR "libharu (hpdf) not found. Install libharu development files "
                         "(e.g. 'pacman -S libharu' or 'apt install libhpdf-dev').")
endif()

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
    src/pdf/HpdfDocument.cpp
    src/pdf/ChartPageLayout.cpp
    src/pdf/PdfExporter.cpp
    # Q_OBJECT headers must be listed explicitly: they live under include/,
    # outside src/, so AUTOMOC's same-directory header scan won't find them
    # otherwise.
    include/ss/core/pattern/PatternModel.h
)

target_include_directories(sscore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(sscore PRIVATE
    ${HPDF_INCLUDE_DIR}
)

target_link_libraries(sscore PUBLIC
    Qt6::Core
    Qt6::Gui
    ${HPDF_LIBRARY}
)

target_compile_features(sscore PUBLIC cxx_std_17)

add_subdirectory(tests)
