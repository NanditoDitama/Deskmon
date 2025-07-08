cmake_minimum_required(VERSION 3.16)

project(Deskmon VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Mencari komponen Qt6 yang diperlukan
find_package(Qt6 REQUIRED COMPONENTS
    Quick
    Gui
    Qml
    Core
    Sql
    Network
    Widgets
    QuickControls2



)

qt_standard_project_setup(REQUIRES 6.8)

# Menambahkan sumber daya (resources) dan file QML
set(SOURCES
    main.cpp
    logger.cpp
    idlechecker.cpp
)

set(HEADERS
    logger.h
    idlechecker.h
)

set(QML_FILES
    Main.qml
)

# Menambahkan executable
qt_add_executable(Deskmon
    ${SOURCES}
    ${HEADERS}
)

# Menambahkan modul QML
qt_add_qml_module(Deskmon
    URI window_logger
    VERSION 1.0
    QML_FILES ${QML_FILES}
    SOURCES
    SOURCES
)

# Menambahkan resource file jika diperlukan
qt_add_resources(Deskmon "resources"
    PREFIX "/"
    FILES
        icon.ico
        icon.png
        play_icon_app.png
        pause_icon_app.png
        profilImage.png
        icons/arrow_back.svg
        icons/camera.svg
        icons/edit.svg
        icons/visibility.svg
        icons/visibility_off.svg
        icons/dark_mode.svg
        icons/light_mode.svg
        icons/chevron-left.svg
        icons/chevron-right.svg
        icons/refresh.svg
        icons/search.svg
        icons/check.svg
        icons/more-horiz.svg
        icons/review.svg
        icons/close.svg

)

# Link library yang diperlukan
target_link_libraries(Deskmon
    PRIVATE
        Qt6::Quick
        Qt6::Gui
        Qt6::Qml
        Qt6::Core
        Qt6::Sql
        Qt6::Network
        Qt6::Widgets
        Qt6::QuickControls2



)

# Link library spesifik platform
if(WIN32)
    target_link_libraries(Deskmon PRIVATE user32 psapi)
elseif(APPLE)
    # Framework khusus macOS
    find_library(COCOA_LIBRARY Cocoa)
    target_link_libraries(Deskmon PRIVATE ${COCOA_LIBRARY})
elseif(UNIX AND NOT APPLE)
    # Linux
    find_package(X11 REQUIRED)
    target_link_libraries(Deskmon PRIVATE ${X11_LIBRARIES})
endif()

# Untuk macOS, tambahkan pengaturan bundle
if(APPLE)
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.yourcompany.deskmon")
    set(MACOSX_BUNDLE_BUNDLE_NAME "Deskmon")
    set(MACOSX_BUNDLE_ICON_FILE "icon.icns")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}")
    set(MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION}")
    set(MACOSX_BUNDLE_INFO_STRING "Deskmon Activity Monitor")
    set(MACOSX_BUNDLE_COPYRIGHT "Copyright © 2025 Your Company")
endif()

# Set properti target
set_target_properties(Deskmon PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# Install rules
include(GNUInstallDirs)
install(TARGETS Deskmon
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
