
#include <catch2/catch_all.hpp>

#include "pattern_matcher/PatternMatcher.h"

TEST_CASE("FiniteStateMachineFragment", "[patterns]")
{
	{
		fragments::LiteralFragment literal("a");

		REQUIRE(literal.Match("a"));
		REQUIRE(literal.Match("a")->myPattern == &literal);
		REQUIRE(literal.Match("ab"));
		REQUIRE(literal.Match("abc"));
		REQUIRE(literal.Match("abc")->mySubMatches.empty());
		REQUIRE(literal.Match("abc")->myRange == "a");

	}
}