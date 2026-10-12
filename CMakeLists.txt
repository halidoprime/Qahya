cmake_minimum_required(VERSION 3.19)
project(QControl LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets LinguistTools Charts)

qt_standard_project_setup()

qt_add_executable(QControl
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    servicemanager.cpp
    servicemanager.h
    firewallmanager.cpp
    firewallmanager.h
    addruledialog.cpp
    addruledialog.h
    interfacemanager.cpp
    interfacemanager.h
    connectionmanager.cpp
    connectionmanager.h
    localdevicemanager.cpp
    localdevicemanager.h
    netspeedmonitor.cpp
    netspeedmonitor.h
    netgraphwidget.cpp
    netgraphwidget.h
)

qt_add_translations(
    TARGETS QControl
    TS_FILES QControl_en_US.ts
)

target_link_libraries(QControl
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Charts
)

install(TARGETS QControl
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET QControl
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
