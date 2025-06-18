

list(APPEND Files PatternMatcher.cpp)
list(APPEND Files PatternMatcherFragment.cpp)

add_executable(pattern_matcher_test ${Files})

target_link_libraries(pattern_matcher_test PUBLIC pattern_matcher)
target_link_libraries(pattern_matcher_test PRIVATE Catch2::Catch2WithMain)