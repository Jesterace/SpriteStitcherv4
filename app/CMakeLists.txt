add_executable(spritestitcher main.cpp resources.qrc)

target_link_libraries(spritestitcher PRIVATE ssui)

include(GNUInstallDirs)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/spritestitcher.desktop.in
    ${CMAKE_CURRENT_BINARY_DIR}/spritestitcher.desktop
    @ONLY
)

install(TARGETS spritestitcher RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/spritestitcher.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES icons/spritestitcher.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps)
