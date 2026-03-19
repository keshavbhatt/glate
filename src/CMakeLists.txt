cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.10 REQUIRED COMPONENTS Core Gui Widgets Network Multimedia)

qt_standard_project_setup()

# QHotkey sources
set(QHOTKEY_SOURCES
    QHotkey/qhotkey.cpp
    QHotkey/qhotkey.h
    QHotkey/qhotkey_p.h
)

if(APPLE)
    list(APPEND QHOTKEY_SOURCES QHotkey/qhotkey_mac.cpp)
elseif(WIN32)
    list(APPEND QHOTKEY_SOURCES QHotkey/qhotkey_win.cpp)
elseif(UNIX)
    list(APPEND QHOTKEY_SOURCES QHotkey/qhotkey_x11.cpp)
endif()

# Windows: embed application icon and version info via resource file
set(PLATFORM_SOURCES "")
if(WIN32)
    list(APPEND PLATFORM_SOURCES glate.rc)
endif()

qt_add_executable(glate
    controlbutton.cpp
    controlbutton.h
    elidedlabel.cpp
    elidedlabel.h
    error.cpp
    error.h
    history.cpp
    history.h
    linebyline.cpp
    linebyline.h
    main.cpp
    mainwindow.cpp
    mainwindow.h
    request.cpp
    request.h
    settings.cpp
    settings.h
    share.cpp
    share.h
    slider.cpp
    slider.h
    systemtraymanager.cpp
    systemtraymanager.h
    translationdownloader.cpp
    translationdownloader.h
    utils.cpp
    utils.h
    waitingspinnerwidget.cpp
    waitingspinnerwidget.h
    ${QHOTKEY_SOURCES}
    ${PLATFORM_SOURCES}
    error.ui
    history.ui
    history_item.ui
    linebyline.ui
    mainwindow.ui
    settings.ui
    share.ui
    slider.ui
    textoptionform.ui
    icons.qrc
    qbreeze.qrc
    resources.qrc
    slides.qrc
)

# On Windows: suppress the console window (GUI subsystem)
if(WIN32)
    set_target_properties(glate PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

target_include_directories(glate PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/QHotkey
)

target_compile_definitions(glate PRIVATE
    QT_DEPRECATED_WARNINGS
    $<$<NOT:$<CONFIG:Debug>>:QT_NO_DEBUG_OUTPUT>
)

target_link_libraries(glate PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Multimedia
)

if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(XCB REQUIRED xcb)
    target_link_libraries(glate PRIVATE X11 ${XCB_LIBRARIES})
    target_include_directories(glate PRIVATE ${XCB_INCLUDE_DIRS})
elseif(APPLE)
    target_link_libraries(glate PRIVATE "-framework Carbon")
elseif(WIN32)
    target_link_libraries(glate PRIVATE user32)
endif()

# -------------------------
# Windows: windeployqt
# Run windeployqt after build to collect required Qt DLLs next to the exe
# -------------------------
if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${Qt6_DIR}/../../../bin")
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET glate POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                --verbose 0
                --no-compiler-runtime
                --no-translations
                "$<TARGET_FILE:glate>"
            COMMENT "Running windeployqt..."
        )
    else()
        message(WARNING "windeployqt not found - Qt DLLs will not be copied automatically")
    endif()
endif()

# Installation
include(GNUInstallDirs)

install(TARGETS glate
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Linux / macOS only installs (not applicable on Windows)
if(UNIX)
    install(FILES icons/app/icon-64.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/64x64/apps/
        RENAME com.ktechpit.glate.png
    )
    install(FILES icons/app/icon-128.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/128x128/apps/
        RENAME com.ktechpit.glate.png
    )
    install(FILES icons/app/icon-256.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps/
        RENAME com.ktechpit.glate.png
    )
    install(FILES icons/app/icon-512.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/512x512/apps/
        RENAME com.ktechpit.glate.png
    )
    install(FILES glate.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications/
        RENAME com.ktechpit.glate.desktop
    )
    install(FILES com.ktechpit.glate.metainfo.xml
        DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo/
    )
endif()

# -------------------------
# CPack packaging
# -------------------------
set(CPACK_PACKAGE_NAME "Glate")
set(CPACK_PACKAGE_VENDOR "KtechPit")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Google Translator for Desktop")
set(CPACK_PACKAGE_VERSION "4.0.0")
set(CPACK_PACKAGE_VERSION_MAJOR "4")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Glate")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE")

if(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
    set(CPACK_NSIS_DISPLAY_NAME "Glate ${CPACK_PACKAGE_VERSION}")
    set(CPACK_NSIS_PACKAGE_NAME "Glate")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/icons/app/glate.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}/icons/app/glate.ico")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\glate.exe")
    set(CPACK_NSIS_HELP_LINK "https://github.com/keshavnrj/glate")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/keshavnrj/glate")
    set(CPACK_NSIS_CONTACT "keshavnrj@gmail.com")
    set(CPACK_NSIS_MODIFY_PATH OFF)
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    # Create a desktop shortcut
    set(CPACK_NSIS_CREATE_ICONS_EXTRA
        "CreateShortCut '$DESKTOP\\\\Glate.lnk' '$INSTDIR\\\\bin\\\\glate.exe'"
    )
    set(CPACK_NSIS_DELETE_ICONS_EXTRA
        "Delete '$DESKTOP\\\\Glate.lnk'"
    )
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
elseif(UNIX)
    set(CPACK_GENERATOR "DEB;RPM;TGZ")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "KtechPit <keshavnrj@gmail.com>")
    set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt6core6, libqt6gui6, libqt6widgets6, libqt6network6, libqt6multimedia6, ffmpeg")
    set(CPACK_RPM_PACKAGE_LICENSE "GPL-3.0")
    set(CPACK_RPM_PACKAGE_GROUP "Applications/Productivity")
endif()

include(CPack)
