cmake_minimum_required(VERSION 3.15)
project(hierarchy)

include(FetchContent)
FetchContent_Declare(
  thh_handles
  GIT_REPOSITORY https://github.com/pr0g/cpp-handle-container.git
  GIT_TAG        16df2db55d32a68fe9c3f8e49733a98d27bd2dba)
FetchContent_MakeAvailable(thh_handles)

add_library(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE src/entity.cpp)
target_include_directories(
  ${PROJECT_NAME} PUBLIC
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(
  ${PROJECT_NAME} PUBLIC thh_handles)

option(HIERARCHY_DEMO "Builds simple terminal example of hierarchy" OFF)

if (${HIERARCHY_DEMO})
  add_executable(${PROJECT_NAME}-demo)

  if (${CMAKE_HOST_APPLE})
    find_package(Curses)
    target_sources(${PROJECT_NAME}-demo PRIVATE main-macos.cpp)
    target_link_libraries(${PROJECT_NAME}-demo PRIVATE Curses  ${PROJECT_NAME})
  endif()

  if (${CMAKE_HOST_WIN32})
    FetchContent_Declare(
      wil
      GIT_REPOSITORY https://github.com/microsoft/wil.git
      GIT_TAG        6b508a358fb7b12264ada0829cff83862d33f44e)
    FetchContent_MakeAvailable(wil)
    target_sources(${PROJECT_NAME}-demo PRIVATE main-win.cpp)
    target_link_libraries(${PROJECT_NAME}-demo PRIVATE ${PROJECT_NAME} WIL)
  endif()
endif()
