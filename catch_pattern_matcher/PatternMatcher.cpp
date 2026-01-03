

#include "pattern_matcher/PatternMatcher.h"

#include <catch2/catch_all.hpp>
#include <string>

TEST_CASE("construct::basic", "")
{
    pattern_matcher::PatternMatcher<> matcher;
}
TEST_CASE("construct::items::empty", "")
{
    pattern_matcher::PatternMatcher<> matcher;

    int amount = GENERATE(1, 2, 4, 8, 16, 32, 64, 128);

    for (size_t i = 0; i < amount; i++) matcher.EmplaceFragment(std::to_string(i));
}

TEST_CASE("construct::items::repeat", "")
{
    pattern_matcher::PatternMatcher<> matcher;

    int amount = GENERATE(1, 2, 4, 8, 16, 32, 64, 128);

    for (size_t i = 0; i < amount; i++)
        matcher.EmplaceFragment(std::to_string(i), matcher.Of("a")[0], pattern_matcher::RepeatCount{1, 1});
}

TEST_CASE("construct::items::sequence", "")
{
    pattern_matcher::PatternMatcher<> matcher;

    int amount = GENERATE(1, 2, 4, 8, 16, 32, 64, 128);

    for (size_t i = 0; i < amount; i++)
        matcher.EmplaceFragment(std::to_string(i), pattern_matcher::Fragment::Type::Sequence, matcher.Of("a"));
}

TEST_CASE("construct::items::alternative", "")
{
    pattern_matcher::PatternMatcher<> matcher;

    int amount = GENERATE(1, 2, 4, 8, 16, 32, 64, 128);

    for (size_t i = 0; i < amount; i++)
        matcher.EmplaceFragment(std::to_string(i), pattern_matcher::Fragment::Type::Alternative, matcher.Of("a"));
}

TEST_CASE("basic", "")
{
    using namespace pattern_matcher;
    PatternMatcher<> matcher;

    matcher.EmplaceFragment("a", matcher[(Fragment::Literal)'a'], RepeatCount{1, 1});
    matcher.EmplaceFragment("b", matcher[(Fragment::Literal)'b'], RepeatCount{1, 1});
    matcher.EmplaceFragment("c", matcher[(Fragment::Literal)'c'], RepeatCount{1, 1});
    matcher.EmplaceFragment("any", Fragment::Type::Alternative, matcher.Of("abc"));
    matcher.EmplaceFragment("all", Fragment::Type::Sequence, matcher.Of("abc"));

    {
        std::string data("a");
        auto m = matcher.Match("a", data);

        REQUIRE(m);
        REQUIRE(std::ranges::begin(data) == std::ranges::begin(*m));
        REQUIRE(std::ranges::end(data) == std::ranges::end(*m));
    }

    auto am = matcher.Match("a", "a");
    REQUIRE(am);
    CHECK(*am == "a");

    auto bm = matcher.Match("b", "b");
    REQUIRE(bm);
    CHECK(*bm == "b");

    auto cm = matcher.Match("c", "c");
    REQUIRE(cm);
    CHECK(*cm == "c");

    auto anyAM = matcher.Match("any", "a");
    REQUIRE(anyAM);
    CHECK(*anyAM == "a");

    auto anyBM = matcher.Match("any", "b");
    REQUIRE(anyBM);
    CHECK(*anyBM == "b");

    auto anyCM = matcher.Match("any", "c");
    REQUIRE(anyCM);
    CHECK(*anyCM == "c");

    REQUIRE(!matcher.Match("all", "a"));
    REQUIRE(!matcher.Match("all", "ab"));
    REQUIRE(matcher.Match("all", "abc"));
    REQUIRE(*matcher.Match("all", "abc") == "abc");
    REQUIRE(matcher.Match("all", "abc")->mySubMatches.size() == 3);
    REQUIRE(matcher.Match("all", "abc")->mySubMatches[0] == "a");
    REQUIRE(matcher.Match("all", "abc")->mySubMatches[1] == "b");
    REQUIRE(matcher.Match("all", "abc")->mySubMatches[2] == "c");
}