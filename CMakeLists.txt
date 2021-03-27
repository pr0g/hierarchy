cmake_minimum_required(VERSION 3.15)
project(hierarchy)

include(FetchContent)
FetchContent_Declare(
  thh_handles
  GIT_REPOSITORY https://github.com/pr0g/cpp-handle-container.git
  GIT_TAG        16df2db55d32a68fe9c3f8e49733a98d27bd2dba)
FetchContent_MakeAvailable(thh_handles)

add_executable(${PROJECT_NAME})

if (${CMAKE_HOST_APPLE})
  find_package(Curses)
  target_sources(${PROJECT_NAME} PRIVATE main-macos.cpp entity.cpp)
  target_link_libraries(${PROJECT_NAME} PRIVATE Curses thh_handles)
endif()

if (${CMAKE_HOST_WIN32})
  target_sources(${PROJECT_NAME} PRIVATE main-win.cpp entity.cpp)
  target_link_libraries(${PROJECT_NAME} PRIVATE thh_handles)
endif()
