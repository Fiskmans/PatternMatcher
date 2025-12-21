
#include "catch2/catch_all.hpp"
#include "catch_pattern_matcher/JSON.h"

TEST_CASE("pattern_matcher::integration::json_regression::structure_array_open_object", "[regression]")
{
    PatternMatcher matcher = MakeJsonParser().Finalize();

    std::string frag = "[{\"\":";

    std::string full;

    for (int i = 0; i < 50000; i++) full += frag;

    REQUIRE(!matcher.Match("value", full));
}

TEST_CASE("pattern_matcher::integration::json_regression::string_1_2_3_bytes_UTF-8_sequences", "[regression]")
{
    PatternMatcher matcher = MakeJsonParser().Finalize();
    std::string full(R"(["\u0060\u012a\u12AB"])");
    REQUIRE(matcher.Match("value", full));
}
