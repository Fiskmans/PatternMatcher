
#include "catch2/catch_all.hpp"
#include "catch_pattern_matcher/JSON.h"

TEST_CASE("regression::long_string", "[regression]")
{
    pattern_matcher::PatternMatcher matcher = MakeJsonParser().Finalize();

    std::string frag = "[{\"\":";

    std::string full;

    for (int i = 0; i < 50000; i++) full += frag;

    REQUIRE(!matcher.Match("value", full));
}
