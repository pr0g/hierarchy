cmake_minimum_required(VERSION 3.15)
project(hierarchy)

include(FetchContent)
FetchContent_Declare(
  thh-handle-vector
  GIT_REPOSITORY https://github.com/pr0g/cpp-handle-container.git
  GIT_TAG 8424c42ed497a3d37d9cde1c0e3f0cf440c6cf1a)
FetchContent_MakeAvailable(thh-handle-vector)

add_library(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE src/entity.cpp src/entity-old.cpp)
target_include_directories(
  ${PROJECT_NAME}
  PUBLIC
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(${PROJECT_NAME} PUBLIC thh-handle-vector)

option(HIERARCHY_DEMO "Builds simple terminal example of hierarchy" OFF)
option(HIERARCHY_TEST "Builds unit tests for hierarchy library" OFF)
option(HIERARCHY_BENCH "Builds benchmarks for hierarchy library" OFF)

find_package(Curses)

if (${HIERARCHY_DEMO})
  add_executable(${PROJECT_NAME}-scroll-demo)
  target_sources(${PROJECT_NAME}-scroll-demo PRIVATE main-scroll.cpp)
  target_compile_features(${PROJECT_NAME}-scroll-demo PRIVATE cxx_std_17)
  target_link_libraries(
    ${PROJECT_NAME}-scroll-demo $<$<PLATFORM_ID:Linux>:ncursesw>
    $<$<PLATFORM_ID:Darwin>:ncurses> $<$<PLATFORM_ID:Windows>:pdcurses>
    ${PROJECT_NAME})

  if (${CMAKE_HOST_APPLE})
    add_executable(${PROJECT_NAME}-demo)
    target_sources(${PROJECT_NAME}-demo PRIVATE main-macos.cpp)
    target_link_libraries(${PROJECT_NAME}-demo PRIVATE Curses ${PROJECT_NAME})

    add_executable(scroll-demo)
    target_sources(scroll-demo PRIVATE main-scroll-simple-macos.cpp)
    target_link_libraries(scroll-demo PRIVATE Curses)
    target_compile_features(scroll-demo PRIVATE cxx_std_17)
  endif ()

  if (${CMAKE_HOST_WIN32})
    target_link_directories(${PROJECT_NAME}-scroll-demo PRIVATE
                            ${CMAKE_SOURCE_DIR}/third-party/pdcurses)
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
    GIT_TAG ae7a13539fb71f270b87eb2e874fbac80bc8dda2)
  FetchContent_MakeAvailable(doctest)
  target_link_libraries(${PROJECT_NAME}-test PRIVATE ${PROJECT_NAME} doctest)

  enable_testing()
  add_test(NAME ${PROJECT_NAME}-test COMMAND ${PROJECT_NAME}-test)
  set(args -C Debug)
  add_custom_command(
    TARGET ${PROJECT_NAME}-test
    POST_BUILD
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failures
            "$<$<PLATFORM_ID:Windows>:${args}>"
    COMMAND_EXPAND_LISTS)
endif ()

if (${HIERARCHY_BENCH})
  set(BENCHMARK_ENABLE_TESTING OFF)
  FetchContent_Declare(benchmark
                       GIT_REPOSITORY https://github.com/google/benchmark.git
                       GIT_TAG main)
  FetchContent_MakeAvailable(benchmark)
  add_executable(${PROJECT_NAME}-bench)
  target_sources(${PROJECT_NAME}-bench PRIVATE ${PROJECT_NAME}-bench.cpp)
  target_link_libraries(${PROJECT_NAME}-bench ${PROJECT_NAME} benchmark)
endif ()
