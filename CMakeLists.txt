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

# For macOS, set logger.cpp to be compiled as Objective-C++
if(APPLE)
    set_source_files_properties(logger.cpp PROPERTIES COMPILE_FLAGS "-x objective-c++")
    set_source_files_properties(idlechecker.cpp PROPERTIES COMPILE_FLAGS "-x objective-c++")
endif()

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

# Konfigurasi Windows Resource file untuk icon
if(WIN32)
    # Buat file resource Windows
    set(WIN_RESOURCE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/app.rc)

    # Pastikan file icon.ico ada
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/icon.ico)
        # Buat konten app.rc dengan format yang benar
        file(WRITE ${WIN_RESOURCE_FILE}
            "// Microsoft Visual C++ generated resource script.\n"
            "#include \"resource.h\"\n"
            "\n"
            "#define APSTUDIO_READONLY_SYMBOLS\n"
            "#include \"winres.h\"\n"
            "#undef APSTUDIO_READONLY_SYMBOLS\n"
            "\n"
            "// Icon\n"
            "IDI_ICON1 ICON \"icon.ico\"\n"
            "\n"
            "// Version Information\n"
            "VS_VERSION_INFO VERSIONINFO\n"
            " FILEVERSION 0,1,0,0\n"
            " PRODUCTVERSION 0,1,0,0\n"
            " FILEFLAGSMASK 0x3fL\n"
            " FILEFLAGS 0x0L\n"
            " FILEOS 0x40004L\n"
            " FILETYPE 0x1L\n"
            " FILESUBTYPE 0x0L\n"
            "BEGIN\n"
            "    BLOCK \"StringFileInfo\"\n"
            "    BEGIN\n"
            "        BLOCK \"040904b0\"\n"
            "        BEGIN\n"
            "            VALUE \"CompanyName\", \"Your Company\"\n"
            "            VALUE \"FileDescription\", \"Deskmon Activity Monitor\"\n"
            "            VALUE \"FileVersion\", \"0.1.0.0\"\n"
            "            VALUE \"InternalName\", \"Deskmon\"\n"
            "            VALUE \"LegalCopyright\", \"Copyright (C) 2025\"\n"
            "            VALUE \"OriginalFilename\", \"Deskmon.exe\"\n"
            "            VALUE \"ProductName\", \"Deskmon\"\n"
            "            VALUE \"ProductVersion\", \"0.1.0.0\"\n"
            "        END\n"
            "    END\n"
            "    BLOCK \"VarFileInfo\"\n"
            "    BEGIN\n"
            "        VALUE \"Translation\", 0x409, 1200\n"
            "    END\n"
            "END\n"
        )

        # Buat file resource.h
        set(RESOURCE_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/resource.h)
        file(WRITE ${RESOURCE_HEADER}
            "//{{NO_DEPENDENCIES}}\n"
            "// Microsoft Visual C++ generated include file.\n"
            "#define IDI_ICON1                       101\n"
            "\n"
            "// Next default values for new objects\n"
            "// \n"
            "#ifdef APSTUDIO_INVOKED\n"
            "#ifndef APSTUDIO_READONLY_SYMBOLS\n"
            "#define _APS_NEXT_RESOURCE_VALUE        102\n"
            "#define _APS_NEXT_COMMAND_VALUE         40001\n"
            "#define _APS_NEXT_CONTROL_VALUE         1001\n"
            "#define _APS_NEXT_SYMED_VALUE           101\n"
            "#endif\n"
            "#endif\n"
        )

        # Tambahkan resource file ke sources
        set(SOURCES ${SOURCES} ${WIN_RESOURCE_FILE})

        message(STATUS "Windows resource file created: ${WIN_RESOURCE_FILE}")
    else()
        message(WARNING "icon.ico not found! Icon will not be embedded.")
    endif()
endif()

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
    QML_FILES Pop_up_waktuhabis.qml
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
        icons/danger.svg
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

    # Konfigurasi icon untuk macOS
    set_target_properties(Deskmon PROPERTIES
        MACOSX_BUNDLE_ICON_FILE "icon.icns"
    )

    # Copy icon ke bundle (jika ada icon.icns)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/icon.icns)
        target_sources(Deskmon PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/icon.icns)
        set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/icon.icns PROPERTIES
            MACOSX_PACKAGE_LOCATION Resources
        )
    endif()
elseif(UNIX AND NOT APPLE)
    # Linux
    find_package(X11 REQUIRED)
    target_link_libraries(Deskmon PRIVATE ${X11_LIBRARIES})
endif()

# Untuk macOS, tambahkan pengaturan bundle
if(APPLE)
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.yourcompany.deskmon")
    set(MACOSX_BUNDLE_BUNDLE_NAME "Deskmon")
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
