cmake_minimum_required(VERSION 3.16)
project(DynamicNavigationEMS LANGUAGES C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)
add_executable(dynamic_nav_ems
  main.cpp game_types.h
  zombie_queue.c priority_queue.c inventory.c action_stack.c graph.c leaderboard.c save_load.c)
target_link_libraries(dynamic_nav_ems PRIVATE SFML::Graphics SFML::Window SFML::System)
if(MINGW)
  target_link_options(dynamic_nav_ems PRIVATE -static-libgcc -static-libstdc++)
endif()
add_custom_command(TARGET dynamic_nav_ems POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:dynamic_nav_ems>/assets)
