cmake_minimum_required(VERSION 3.19)
project(whu LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Sql Widgets Test)

include(CTest)

qt_standard_project_setup()

add_subdirectory(src/core)

qt_add_executable(flown
    WIN32 MACOSX_BUNDLE
    main.cpp
    src/gui/mainwindow.cpp
    src/gui/mainwindow.h
    src/gui/mainwindow.ui
)

target_link_libraries(flown
    PRIVATE
        flown_core
        Qt::Widgets
)

install(TARGETS flown
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET flown
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
