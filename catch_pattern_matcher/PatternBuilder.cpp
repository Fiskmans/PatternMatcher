
#include "pattern_matcher/PatternBuilder.h"

#include <catch2/catch_all.hpp>

TEST_CASE("pattern_matcher::builder::basic")
{
    using namespace std::string_view_literals;
    PatternBuilder builder;

    builder.Add("a") = "a";
    builder.Add("b") = "b";
    builder.Add("c") = "c";

    builder.Add("any") || "a" || "b" || "c";
    builder.Add("all") && "a" && "b" && "c";

    PatternMatcher matcher = builder.Finalize();

    REQUIRE(matcher.Match("a", "a"));
    REQUIRE(*matcher.Match("a", "a") == "a");
    REQUIRE(matcher.Match("b", "b"));
    REQUIRE(*matcher.Match("b", "b") == "b");
    REQUIRE(matcher.Match("c", "c"));
    REQUIRE(*matcher.Match("c", "c") == "c");

    REQUIRE(matcher.Match("any", "a"));
    REQUIRE(*matcher.Match("any", "a") == "a");
    REQUIRE(matcher.Match("any", "b"));
    REQUIRE(*matcher.Match("any", "b") == "b");
    REQUIRE(matcher.Match("any", "c"));
    REQUIRE(*matcher.Match("any", "c") == "c");

    REQUIRE(!matcher.Match("all", "a"));
    REQUIRE(!matcher.Match("all", "ab"));
    REQUIRE(matcher.Match("all", "abc"));
    REQUIRE(*matcher.Match("all", "abc") == "abc");
    REQUIRE(matcher.Match("all", "abc")->mySubMatches.size() == 3);
    REQUIRE(matcher.Match("all", "abc")->mySubMatches[0] == "a");
    REQUIRE(matcher.Match("all", "abc")->mySubMatches[1] == "b");
    REQUIRE(matcher.Match("all", "abc")->mySubMatches[2] == "c");
}
