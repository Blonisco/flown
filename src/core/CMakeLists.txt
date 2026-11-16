add_library(flown_core STATIC
    launch/resource_launcher.cpp
    launch/resource_launcher.h
    model/context.cpp
    model/context.h
    model/resource.h
    storage/resource_store.cpp
    storage/resource_store.h
)

target_include_directories(flown_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(flown_core
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Sql
)

if(BUILD_TESTING)
    add_subdirectory(test)
endif()
