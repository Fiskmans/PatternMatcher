

list(APPEND Files JSON.cpp)
list(APPEND Files PatternBuilder.cpp)
list(APPEND Files PatternMatcher.cpp)
list(APPEND Files PatternMatcherFragment.cpp)

add_executable(catch_pattern_matcher ${Files})

target_link_libraries(catch_pattern_matcher PUBLIC pattern_matcher)
target_link_libraries(catch_pattern_matcher PRIVATE Catch2::Catch2WithMain)

add_compile_definitions(CATCH_JSON_TEST_CASES_PATH="${jsontestsuite_SOURCE_DIR}/test_parsing/")