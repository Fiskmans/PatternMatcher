
list(APPEND Files Concepts.h)
list(APPEND Files Fragment.h)
list(APPEND Files PatternBuilder.cpp)
list(APPEND Files PatternBuilder.h)
list(APPEND Files PatternMatcher.h)
list(APPEND Files PatternMatchingTypes.h)
list(APPEND Files RepeatCount.cpp)
list(APPEND Files RepeatCount.h)

add_library(pattern_matcher ${Files} )

target_include_directories(pattern_matcher PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)