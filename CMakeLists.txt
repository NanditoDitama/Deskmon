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
    Svg
)

# For macOS, set deskmon.cpp to be compiled as Objective-C++
if(APPLE)
    set_source_files_properties(src/deskmon.cpp PROPERTIES COMPILE_FLAGS "-x objective-c++")
    set_source_files_properties(src/idlechecker.cpp PROPERTIES COMPILE_FLAGS "-x objective-c++")
endif()

qt_standard_project_setup(REQUIRES 6.8)

# Menambahkan sumber daya (resources) dan file QML
set(SOURCES
    src/main.cpp
    src/deskmon.cpp
    src/deskmon_auth.cpp
    src/deskmon_worktime.cpp
    src/deskmon_log.cpp
    src/deskmon_window.cpp
    src/deskmon_profile.cpp
    src/deskmon_system.cpp
    src/deskmon_task.cpp
    src/deskmon_productivity.cpp
    src/datamanager.cpp
    src/windowmanager.cpp
    src/apimanager.cpp
    src/sessionmanager.cpp
    src/taskcontroller.cpp
    src/idlechecker.cpp
)

set(HEADERS
    src/deskmon.h
    src/datamanager.h
    src/windowmanager.h
    src/apimanager.h
    src/sessionmanager.h
    src/taskcontroller.h
    src/idlechecker.h
)

# Konfigurasi Windows Resource file untuk icon
if(WIN32)
    # Buat file resource Windows
    set(WIN_RESOURCE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/src/app.rc)

    # Pastikan file icon.ico ada
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.ico)
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
            "IDI_ICON1 ICON \"../assets/icon.ico\"\n"
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
        set(RESOURCE_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/src/resource.h)
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
        message(WARNING "assets/icon.ico not found! Icon will not be embedded.")
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
    QML_FILES
        qml/Main.qml
        qml/Pop_up_waktuhabis.qml
        qml/Login_page.qml
        qml/Profil_page.qml
        qml/Productivty.qml
        qml/SaveTombolCropImage.qml
        qml/Current_Task.qml
        qml/Activity_Monitor.qml
        qml/Monitored_Applications.qml
        qml/AddAplicatioRequest.qml
        qml/RequstAppPending.qml
        qml/DateRange.qml
        qml/EarlyLeaveDialog.qml
        qml/TaskDetailsDialog.qml
        qml/FailureDialog.qml
        qml/NeedReviewDialog.qml
        qml/AddAppDialog.qml
        qml/ApplicationsDialog.qml
        qml/ConfirmFinishDialog.qml
        qml/ConfirmSwitchDialog.qml
        qml/DateRangeDialog.qml
        qml/EarlyLogoutDialog.qml
        qml/MainDialog.qml
        qml/RequestDialog.qml
        qml/TaskDetailPopup.qml
        qml/Dialog.qml
)

# Menambahkan resource file jika diperlukan
qt_add_resources(Deskmon "resources"
    PREFIX "/"
    FILES
        assets/icon.ico
        assets/icon.png
        assets/play_icon_app.png
        assets/pause_icon_app.png
        assets/profilImage.png
        assets/icons/dm.png
        assets/icons/arrow_back.svg
        assets/icons/camera.svg
        assets/icons/edit.svg
        assets/icons/visibility.svg
        assets/icons/visibility_off.svg
        assets/icons/dark_mode.svg
        assets/icons/light_mode.svg
        assets/icons/chevron-left.svg
        assets/icons/chevron-right.svg
        assets/icons/refresh.svg
        assets/icons/search.svg
        assets/icons/check.svg
        assets/icons/more-horiz.svg
        assets/icons/review.svg
        assets/icons/close.svg
        assets/icons/danger.svg
        assets/icons/website.svg
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
        Qt6::Svg
)

# Link library spesifik platform
if(WIN32)
    target_link_libraries(Deskmon PRIVATE user32 psapi ole32 uuid)
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
    WIN32_EXECUTABLE FALSE
)

# Install rules
include(GNUInstallDirs)
install(TARGETS Deskmon
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
