cmake_minimum_required(VERSION 3.16)
project(DynamicNavigationEMS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)

add_executable(dynamic_nav_ems main.cpp)
target_link_libraries(dynamic_nav_ems PRIVATE SFML::Graphics SFML::Window SFML::System)

# MinGW (MSYS2): fewer runtime DLLs to ship next to the exe
if(MINGW)
    target_link_options(dynamic_nav_ems PRIVATE -static-libgcc -static-libstdc++)
endif()

# Copy assets/ next to the executable so it also runs from inside build/
add_custom_command(TARGET dynamic_nav_ems POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:dynamic_nav_ems>/assets)
