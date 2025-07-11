

#include <catch2/catch_all.hpp>

#include "pattern_matcher/PatternMatcher.h"

#include <string>

TEST_CASE("pattern_matcher::pattern_matcher::basic", "")
{
	PatternMatcher<> matcher;

	using namespace std::string_literals;

	matcher.AddLiteral("a", "a");
	matcher.AddLiteral("b", "b");
	matcher.AddLiteral("c", "c");

	matcher.AddFragment(std::make_unique<fragments::AlternativeFragment<>>("any", std::vector<std::string>{ "a", "b", "c" }));
	matcher.AddFragment(std::make_unique<fragments::SequenceFragment<>>("all", std::vector<std::string>{ "a", "b", "c" }));

	REQUIRE(matcher.Resolve());

	REQUIRE(matcher.Match("a", "a"));
	REQUIRE(matcher.Match("a", "a")->myRange == "a");
	REQUIRE(matcher.Match("b", "b"));
	REQUIRE(matcher.Match("b", "b")->myRange == "b");
	REQUIRE(matcher.Match("c", "c"));
	REQUIRE(matcher.Match("c", "c")->myRange == "c");

	REQUIRE(matcher.Match("any", "a"));
	REQUIRE(matcher.Match("any", "a")->myRange == "a");
	REQUIRE(matcher.Match("any", "b"));
	REQUIRE(matcher.Match("any", "b")->myRange == "b");
	REQUIRE(matcher.Match("any", "c"));
	REQUIRE(matcher.Match("any", "c")->myRange == "c");

	REQUIRE(!matcher.Match("all", "a"));
	REQUIRE(!matcher.Match("all", "ab"));
	REQUIRE(matcher.Match("all", "abc"));
	REQUIRE(matcher.Match("all", "abc")->myRange == "abc");
	REQUIRE(matcher.Match("all", "abc")->mySubMatches.size() == 3);
	REQUIRE(matcher.Match("all", "abc")->mySubMatches[0].myRange == "a");
	REQUIRE(matcher.Match("all", "abc")->mySubMatches[1].myRange == "b");
	REQUIRE(matcher.Match("all", "abc")->mySubMatches[2].myRange == "c");
}