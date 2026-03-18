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

target_include_directories(glate PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/QHotkey
)

target_compile_definitions(glate PRIVATE
    QT_DEPRECATED_WARNINGS
    $<$<CONFIG:Release>:QT_NO_DEBUG_OUTPUT>
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

# Installation
include(GNUInstallDirs)

install(TARGETS glate
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES icons/linguist.png
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/512x512/apps/
)

install(FILES glate.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications/
)
