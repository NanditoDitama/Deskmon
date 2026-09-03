cmake_minimum_required(VERSION 3.16)

project(Deskmon VERSION 1.0.3.4 LANGUAGES CXX)

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

# For macOS, set WindowInfoProvider_Mac.cpp and IdleChecker.cpp to be compiled as Objective-C++
if(APPLE)
    set_source_files_properties(src/core/platform/WindowInfoProvider_Mac.cpp PROPERTIES COMPILE_FLAGS "-x objective-c++")
    set_source_files_properties(src/features/tracking/IdleChecker.cpp PROPERTIES COMPILE_FLAGS "-x objective-c++")
endif()

qt_standard_project_setup(REQUIRES 6.8)

# Menambahkan sumber daya (resources) dan file QML
set(SOURCES
    src/app/main.cpp
    src/app/AppController.cpp
    src/core/network/ApiClient.cpp
    src/core/database/DatabaseManager.cpp
    src/core/database/UserRepository.cpp
    src/core/database/TaskRepository.cpp
    src/core/database/WorkTimeRepository.cpp
    src/core/database/WorkLogRepository.cpp
    src/core/database/ProductivityAppRepository.cpp
    src/features/auth/AuthManager.cpp
    src/features/profile/UserProfileManager.cpp
    src/features/worktime/WorkTimeTracker.cpp
    src/features/tracking/ActivityTracker.cpp
    src/features/tracking/IdleChecker.cpp
    src/features/tasks/TaskManager.cpp
    src/features/productivity/ProductivityStatsService.cpp
)

if(WIN32)
    list(APPEND SOURCES src/core/platform/WindowInfoProvider_Win.cpp)
elseif(APPLE)
    list(APPEND SOURCES src/core/platform/WindowInfoProvider_Mac.cpp)
else()
    list(APPEND SOURCES src/core/platform/WindowInfoProvider_Linux.cpp)
endif()

set(HEADERS
    src/app/AppController.h
    src/core/platform/WindowInfoProvider.h
    src/core/platform/ThemedIconProvider.h
    src/core/network/ApiClient.h
    src/core/database/DatabaseManager.h
    src/core/database/UserRepository.h
    src/core/database/TaskRepository.h
    src/core/database/WorkTimeRepository.h
    src/core/database/WorkLogRepository.h
    src/core/database/ProductivityAppRepository.h
    src/features/auth/AuthManager.h
    src/features/profile/UserProfileManager.h
    src/features/worktime/WorkTimeTracker.h
    src/features/tracking/ActivityTracker.h
    src/features/tracking/IdleChecker.h
    src/features/tasks/TaskManager.h
    src/features/productivity/ProductivityStatsService.h
)

set(QML_FILES
    qml/Main.qml
    qml/theme/Theme.qml
    qml/theme/Lang.qml
    qml/components/ToastNotification.qml
    qml/components/CustomButton.qml
    qml/components/StatusBadge.qml
    qml/components/WorkTimerCard.qml
    qml/login/LoginPage.qml
    qml/profile/ProfilePage.qml
    qml/profile/CropImageDialog.qml
    qml/dashboard/DashboardPage.qml
    qml/dashboard/ProductivityView.qml
    qml/dashboard/CurrentTaskView.qml
    qml/dashboard/ActivityMonitorView.qml
    qml/dashboard/DateRangeDialog.qml
    qml/application_submission/MonitoredApplicationsDialog.qml
    qml/application_submission/AddApplicationRequestDialog.qml
    qml/application_submission/PendingAppRequestsDialog.qml
    qml/dialogs/TimeUpPopup.qml
    qml/dialogs/TaskDetailsDialog.qml
    qml/dialogs/EarlyLeaveDialog.qml
    qml/dialogs/FailureDialog.qml
    qml/dialogs/NeedReviewDialog.qml
    qml/dialogs/IdleNotificationWindow.qml
    qml/dialogs/AuthErrorWindow.qml
    qml/dialogs/PingErrorDialog.qml
    qml/dialogs/TaskDetailPopup.qml
    qml/dialogs/ConfirmSwitchDialog.qml
    qml/dialogs/UpdateDialog.qml
    qml/layouts/HeaderLayout.qml
    qml/layouts/BodyLayout.qml
)

# Konfigurasi Windows Resource file untuk icon
if(WIN32)
    set(VER_TWEAK 0)
    if(PROJECT_VERSION_TWEAK)
        set(VER_TWEAK ${PROJECT_VERSION_TWEAK})
    endif()

    # Buat file resource Windows di binary/build dir agar root bersih
    set(WIN_RESOURCE_FILE ${CMAKE_CURRENT_BINARY_DIR}/app.rc)

    # Pastikan file icon.ico ada
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/resources/images/icon.ico)
        # Buat file resource.h di binary dir
        set(RESOURCE_HEADER ${CMAKE_CURRENT_BINARY_DIR}/resource.h)
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
            "IDI_ICON1 ICON \"${CMAKE_CURRENT_SOURCE_DIR}/resources/images/icon.ico\"\n"
            "\n"
            "// Version Information\n"
            "VS_VERSION_INFO VERSIONINFO\n"
            " FILEVERSION ${PROJECT_VERSION_MAJOR},${PROJECT_VERSION_MINOR},${PROJECT_VERSION_PATCH},${VER_TWEAK}\n"
            " PRODUCTVERSION ${PROJECT_VERSION_MAJOR},${PROJECT_VERSION_MINOR},${PROJECT_VERSION_PATCH},${VER_TWEAK}\n"
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
            "            VALUE \"CompanyName\", \"Pranala\"\n"
            "            VALUE \"FileDescription\", \"Deskmon Activity Monitor\"\n"
            "            VALUE \"FileVersion\", \"${PROJECT_VERSION}\"\n"
            "            VALUE \"InternalName\", \"Deskmon\"\n"
            "            VALUE \"LegalCopyright\", \"Copyright (C) 2026 PT Pranala Digital Teknologi\"\n"
            "            VALUE \"OriginalFilename\", \"Deskmon.exe\"\n"
            "            VALUE \"ProductName\", \"Deskmon\"\n"
            "            VALUE \"ProductVersion\", \"${PROJECT_VERSION}\"\n"
            "        END\n"
            "    END\n"
            "    BLOCK \"VarFileInfo\"\n"
            "    BEGIN\n"
            "        VALUE \"Translation\", 0x409, 1200\n"
            "    END\n"
            "END\n"
        )

        if(MINGW)
            set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
        endif()

        # Tambahkan resource file ke sources
        set(SOURCES ${SOURCES} ${WIN_RESOURCE_FILE})
        message(STATUS "Windows resource file created and added: ${WIN_RESOURCE_FILE}")
    else()
        message(WARNING "resources/images/icon.ico not found! Icon will not be embedded.")
    endif()
endif()

# Menambahkan executable
qt_add_executable(Deskmon
    ${SOURCES}
    ${HEADERS}
)

# Menambahkan direktori include untuk compiler
target_include_directories(Deskmon PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/features
    ${CMAKE_CURRENT_SOURCE_DIR}/src/features/tracking
)

# Declare QML Singleton types
set_source_files_properties(qml/theme/Theme.qml qml/theme/Lang.qml
    PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

# Menambahkan modul QML
qt_add_qml_module(Deskmon
    URI window_logger
    VERSION 1.0
    QML_FILES ${QML_FILES}
    SOURCES
)

# Otomatisasi Sinkronisasi & Terjemahan i18n sebelum kompilasi
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    add_custom_target(sync_i18n ALL
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/sync_i18n.py
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Auto-syncing and translating i18n strings from QML..."
    )
    add_dependencies(Deskmon sync_i18n)
endif()

# Menambahkan resource file gambar dan icon
qt_add_resources(Deskmon "images"
    PREFIX "/"
    BASE "resources/images"
    FILES
        resources/images/icon.ico
        resources/images/icon.png
        resources/images/play_icon_app.png
        resources/images/pause_icon_app.png
        resources/images/profilImage.png
        resources/images/default_app_icon.png
)

qt_add_resources(Deskmon "icons"
    PREFIX "/icons"
    BASE "resources/icons"
    FILES
        resources/icons/arrow_back.svg
        resources/icons/camera.svg
        resources/icons/check.svg
        resources/icons/chevron-left.svg
        resources/icons/chevron-right.svg
        resources/icons/close.svg
        resources/icons/danger.svg
        resources/icons/dark_mode.svg
        resources/icons/edit.svg
        resources/icons/light_mode.svg
        resources/icons/more-horiz.svg
        resources/icons/refresh.svg
        resources/icons/review.svg
        resources/icons/search.svg
        resources/icons/settings.svg
        resources/icons/visibility.svg
        resources/icons/visibility_off.svg
        resources/icons/website.svg
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

# Teruskan versi proyek ke compiler C++
target_compile_definitions(Deskmon PRIVATE APP_VERSION="${PROJECT_VERSION}")

# Link library spesifik platform
if(WIN32)
    target_link_libraries(Deskmon PRIVATE user32 psapi ole32 oleaut32)
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
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.pranala.deskmon")
    set(MACOSX_BUNDLE_BUNDLE_NAME "Deskmon")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}")
    set(MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION}")
    set(MACOSX_BUNDLE_INFO_STRING "Deskmon Activity Monitor")
    set(MACOSX_BUNDLE_COPYRIGHT "Copyright © 2026 PT Pranala Digital Teknologi")
endif()

# Set properti target
option(CONSOLE_LOGS "Enable console output on Windows" OFF)

if(WIN32 AND CONSOLE_LOGS)
    set(WIN32_EXE_OPTION FALSE)
else()
    set(WIN32_EXE_OPTION TRUE)
endif()

set_target_properties(Deskmon PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE ${WIN32_EXE_OPTION}
)

# Install rules
include(GNUInstallDirs)
install(TARGETS Deskmon
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
