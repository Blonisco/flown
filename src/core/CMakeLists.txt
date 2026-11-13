add_library(flown_core STATIC
    model/context.cpp
    model/context.h
    model/resource.h
)

target_include_directories(flown_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(flown_core
    PUBLIC
        Qt6::Core
)
