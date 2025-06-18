
#include <catch2/catch_all.hpp>

#include "pattern_matcher/PatternMatcher.h"

TEST_CASE("FiniteStateMachineFragment", "[patterns]")
{
	SECTION("literal")
	{
		fragments::LiteralFragment literal("a");

		REQUIRE(literal.Match("a"));
		REQUIRE(literal.Match("a")->myPattern == &literal);
		REQUIRE(literal.Match("ab"));
		REQUIRE(literal.Match("abc"));
		REQUIRE(literal.Match("abc")->mySubMatches.empty());
		REQUIRE(literal.Match("abc")->myRange == "a");
	}

	SECTION("sequence")
	{
		fragments::SequenceFragment sequence({ "a", "b" });

		std::unordered_map<std::string, std::unique_ptr<IPatternMatcherFragment>> patterns;

		patterns["a"] = std::make_unique<fragments::LiteralFragment>("a");
		patterns["b"] = std::make_unique<fragments::LiteralFragment>("b");

		REQUIRE(sequence.Resolve(patterns));

		REQUIRE(!sequence.Match("a"));
		REQUIRE(sequence.Match("ab")->myPattern == &sequence);
		REQUIRE(sequence.Match("ab")->myRange == "ab");
		REQUIRE(sequence.Match("ab")->mySubMatches.size() == 2);
		REQUIRE(sequence.Match("ab")->mySubMatches[0].myPattern == patterns["a"].get());
		REQUIRE(sequence.Match("ab")->mySubMatches[0].myRange == "a");
		REQUIRE(sequence.Match("ab")->mySubMatches[0].mySubMatches.empty());

		REQUIRE(sequence.Match("ab")->mySubMatches[1].myPattern == patterns["b"].get());
		REQUIRE(sequence.Match("ab")->mySubMatches[1].myRange == "b");
		REQUIRE(sequence.Match("ab")->mySubMatches[1].mySubMatches.empty());

		REQUIRE(sequence.Match("abc"));
		REQUIRE(sequence.Match("abc")->myRange == "ab");
	}

	SECTION("alternative")
	{
		fragments::AlternativeFragment alternative({ "a", "b" });

		std::unordered_map<std::string, std::unique_ptr<IPatternMatcherFragment>> patterns;

		patterns["a"] = std::make_unique<fragments::LiteralFragment>("a");
		patterns["b"] = std::make_unique<fragments::LiteralFragment>("b");

		REQUIRE(alternative.Resolve(patterns));

		REQUIRE(alternative.Match("a"));
		REQUIRE(alternative.Match("a")->myPattern == &alternative);
		REQUIRE(alternative.Match("a")->myRange == "a");
		REQUIRE(alternative.Match("a")->mySubMatches.size() == 1);
		REQUIRE(alternative.Match("a")->mySubMatches[0].myPattern == patterns["a"].get());
		REQUIRE(alternative.Match("a")->mySubMatches[0].myRange == "a");
		REQUIRE(alternative.Match("a")->mySubMatches[0].mySubMatches.empty());

		REQUIRE(alternative.Match("b"));
		REQUIRE(alternative.Match("b")->myPattern == &alternative);
		REQUIRE(alternative.Match("b")->myRange == "b");
		REQUIRE(alternative.Match("b")->mySubMatches.size() == 1);
		REQUIRE(alternative.Match("b")->mySubMatches[0].myPattern == patterns["b"].get());
		REQUIRE(alternative.Match("b")->mySubMatches[0].myRange == "b");
		REQUIRE(alternative.Match("b")->mySubMatches[0].mySubMatches.empty());

		REQUIRE(!alternative.Match("c"));
	}
}