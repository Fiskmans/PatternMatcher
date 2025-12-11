cmake_minimum_required(VERSION 3.22)

project(PATTERN_MATCHER)

option(FISK_BUILD_TESTING "Build test executables" ON)

set(CMAKE_CXX_STANDARD 23)
set(BUILD_SHARED_LIBS OFF)

Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.11.0
)

FetchContent_Declare(
  JSONTestSuite
  GIT_REPOSITORY https://github.com/nst/JSONTestSuite.git
  GIT_TAG        1ef36fa01286573e846ac449e8683f8833c5b26a
)

add_subdirectory(pattern_matcher)

if(FISK_BUILD_TESTING)
  message("Fisk - Building with tests")

	FetchContent_MakeAvailable(Catch2)
	FetchContent_MakeAvailable(JSONTestSuite)

	add_subdirectory(catch_pattern_matcher)
endif()