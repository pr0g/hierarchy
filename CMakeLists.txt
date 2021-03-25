cmake_minimum_required(VERSION 3.15)
project(ncurses-experiment)
find_package(Curses)

include(FetchContent)
FetchContent_Declare(
  thh_handles
  GIT_REPOSITORY https://github.com/pr0g/cpp-handle-container.git
  GIT_TAG        24f2b0275910da07d9d9292300f2e824e676905e)
FetchContent_MakeAvailable(thh_handles)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE main.cpp entity.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE Curses Panel thh_handles)
