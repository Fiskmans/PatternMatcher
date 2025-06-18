cmake_minimum_required(VERSION 3.22)

project(PATTERN_MATCHER)

option(FISK_BUILD_TESTING "Build test executables" ON)

set(CMAKE_CXX_STANDARD 23)
set(BUILD_SHARED_LIBS OFF)

Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.0.1
)

add_subdirectory(pattern_matcher)

if(FISK_BUILD_TESTING)

	FetchContent_MakeAvailable(Catch2)

	add_subdirectory(catch_pattern_matcher)
endif()