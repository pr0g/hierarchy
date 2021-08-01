cmake_minimum_required(VERSION 3.15)
project(hierarchy)

include(FetchContent)
FetchContent_Declare(
  thh_handles
  GIT_REPOSITORY https://github.com/pr0g/cpp-handle-container.git
  GIT_TAG de5dfa1234d81f0054d02347567d5bf949531be5)
FetchContent_MakeAvailable(thh_handles)

add_library(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE src/entity.cpp)
target_include_directories(
  ${PROJECT_NAME}
  PUBLIC
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(${PROJECT_NAME} PUBLIC thh_handles)

option(HIERARCHY_DEMO "Builds simple terminal example of hierarchy" OFF)
option(HIERARCHY_TEST "Builds unit tests for hierarchy library" OFF)
option(HIERARCHY_BENCH "Builds benchmarks for hierarchy library" OFF)

if (${HIERARCHY_DEMO})
  if (${CMAKE_HOST_APPLE})
    find_package(Curses)

    add_executable(${PROJECT_NAME}-demo)
    target_sources(${PROJECT_NAME}-demo PRIVATE main-macos.cpp)
    target_link_libraries(${PROJECT_NAME}-demo PRIVATE Curses ${PROJECT_NAME})

    add_executable(${PROJECT_NAME}-scroll-demo)
    target_sources(${PROJECT_NAME}-scroll-demo PRIVATE main-scroll-macos.cpp)
    target_link_libraries(${PROJECT_NAME}-scroll-demo PRIVATE Curses
                                                              ${PROJECT_NAME})
    target_compile_features(${PROJECT_NAME}-scroll-demo PRIVATE cxx_std_17)

    add_executable(scroll-demo)
    target_sources(scroll-demo PRIVATE main-scroll-simple-macos.cpp)
    target_link_libraries(scroll-demo PRIVATE Curses)
    target_compile_features(scroll-demo PRIVATE cxx_std_17)
  endif ()

  if (${CMAKE_HOST_WIN32})
    add_executable(${PROJECT_NAME}-scroll-demo)
    target_sources(${PROJECT_NAME}-scroll-demo PRIVATE main-scroll-macos.cpp)
    target_link_libraries(${PROJECT_NAME}-scroll-demo PRIVATE pdcurses
                                                              ${PROJECT_NAME})
    target_link_directories(${PROJECT_NAME}-scroll-demo PRIVATE
                            ${CMAKE_SOURCE_DIR}/third-party/pdcurses)
    target_compile_features(${PROJECT_NAME}-scroll-demo PRIVATE cxx_std_17)
    target_link_options(${PROJECT_NAME}-scroll-demo PRIVATE
                        "/NODEFAULTLIB:LIBCMT")
  endif ()
endif ()

if (${HIERARCHY_TEST})
  add_executable(${PROJECT_NAME}-test)
  target_sources(${PROJECT_NAME}-test PRIVATE hierarchy-test.cpp)
  FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/onqtam/doctest.git
    GIT_TAG f0ab4e793fa863d4a5929ff263ddd3bebec8767e)
  FetchContent_MakeAvailable(doctest)
  target_link_libraries(${PROJECT_NAME}-test PRIVATE ${PROJECT_NAME} doctest)
endif ()

if (${HIERARCHY_BENCH})
  set(BENCHMARK_ENABLE_TESTING OFF)
  FetchContent_Declare(benchmark
                       GIT_REPOSITORY https://github.com/google/benchmark.git)
  FetchContent_MakeAvailable(benchmark)
  add_executable(${PROJECT_NAME}-bench)
  target_sources(${PROJECT_NAME}-bench PRIVATE ${PROJECT_NAME}-bench.cpp)
  target_link_libraries(${PROJECT_NAME}-bench ${PROJECT_NAME} benchmark)
endif ()
