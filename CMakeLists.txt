cmake_minimum_required(VERSION 3.15)
project(ncurses-experiment)

include(FetchContent)
FetchContent_Declare(
  thh_handles
  GIT_REPOSITORY https://github.com/pr0g/cpp-handle-container.git
  GIT_TAG        24f2b0275910da07d9d9292300f2e824e676905e)
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
