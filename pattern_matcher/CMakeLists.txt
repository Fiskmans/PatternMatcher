
list(APPEND Files PatternBuilder.h)
list(APPEND Files PatternBuilder.cpp)
list(APPEND Files PatternMatcher.h)
list(APPEND Files PatternMatcher.cpp)
list(APPEND Files PatternMatcherFragment.h)
list(APPEND Files PatternMatcherFragment.cpp)
list(APPEND Files RepeatCount.h)
list(APPEND Files RepeatCount.cpp)
list(APPEND Files pattern_matcher.natvis)

add_library(pattern_matcher ${Files} )

target_include_directories(pattern_matcher PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)