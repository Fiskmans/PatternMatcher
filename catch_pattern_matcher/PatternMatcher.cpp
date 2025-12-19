

#include "pattern_matcher/PatternMatcher.h"

#include <catch2/catch_all.hpp>
#include <string>

TEST_CASE("pattern_matcher::pattern_matcher::basic", "")
{
    PatternMatcher<> matcher;

    using namespace std::string_view_literals;

    matcher.AddLiteral("a", "a");
    matcher.AddLiteral("b", "b");
    matcher.AddLiteral("c", "c");

    matcher.AddFragment("any") = {PatternMatcherFragmentType::Alternative, {matcher["a"], matcher["b"], matcher["c"]}};
    matcher.AddFragment("all") = {PatternMatcherFragmentType::Sequence, {matcher["a"], matcher["b"], matcher["c"]}};

    REQUIRE(matcher.Resolve());

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