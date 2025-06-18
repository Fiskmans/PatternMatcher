
#include <catch2/catch_all.hpp>

#include "pattern_matcher/PatternMatcher.h"

TEST_CASE("pattern_matcher::fragments::LiteralFragment", "[fragments]")
{
	fragments::LiteralFragment literal("a", "a");

	REQUIRE(literal.Match("a"));
	REQUIRE(literal.Match("a")->myPattern == &literal);
	REQUIRE(literal.Match("ab"));
	REQUIRE(literal.Match("abc"));
	REQUIRE(literal.Match("abc")->mySubMatches.empty());
	REQUIRE(literal.Match("abc")->myRange == "a");
}

TEST_CASE("pattern_matcher::fragments::SequenceFragment", "[fragments]")
{
	fragments::SequenceFragment sequence("sequence", { "a", "b" });

	std::unordered_map<std::string, std::unique_ptr<IPatternMatcherFragment>> patterns;

	patterns["a"] = std::make_unique<fragments::LiteralFragment>("a", "a");
	patterns["b"] = std::make_unique<fragments::LiteralFragment>("b", "b");

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

TEST_CASE("pattern_matcher::fragments::AlternativeFragment", "[fragments]")
{
	fragments::AlternativeFragment alternative("alt", { "a", "b" });

	std::unordered_map<std::string, std::unique_ptr<IPatternMatcherFragment>> patterns;

	patterns["a"] = std::make_unique<fragments::LiteralFragment>("a", "a");
	patterns["b"] = std::make_unique<fragments::LiteralFragment>("b", "b");

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

TEST_CASE("pattern_matcher::fragments::RepeatFragment", "[fragments]")
{
	fragments::RepeatFragment repeat("rep", "a", RepeatCount( 1, 3 ));

	std::unordered_map<std::string, std::unique_ptr<IPatternMatcherFragment>> patterns;

	patterns["a"] = std::make_unique<fragments::LiteralFragment>("a", "a");

	REQUIRE(repeat.Resolve(patterns));

	REQUIRE(!repeat.Match(""));
	REQUIRE(!repeat.Match("c"));
	REQUIRE(repeat.Match("a"));
	REQUIRE(repeat.Match("a")->myPattern == &repeat);
	REQUIRE(repeat.Match("a")->myRange == "a");
	REQUIRE(repeat.Match("a")->mySubMatches.size() == 1);
	REQUIRE(repeat.Match("a")->mySubMatches[0].myRange == "a");

	REQUIRE(repeat.Match("ac"));
	REQUIRE(repeat.Match("ac")->myPattern == &repeat);
	REQUIRE(repeat.Match("ac")->myRange == "a");
	REQUIRE(repeat.Match("ac")->mySubMatches.size() == 1);
	REQUIRE(repeat.Match("ac")->mySubMatches[0].myRange == "a");

	REQUIRE(repeat.Match("aa"));
	REQUIRE(repeat.Match("aa")->myPattern == &repeat);
	REQUIRE(repeat.Match("aa")->myRange == "aa");
	REQUIRE(repeat.Match("aa")->mySubMatches.size() == 2);
	REQUIRE(repeat.Match("aa")->mySubMatches[0].myRange == "a");
	REQUIRE(repeat.Match("aa")->mySubMatches[1].myRange == "a");

	REQUIRE(repeat.Match("aac"));
	REQUIRE(repeat.Match("aac")->myPattern == &repeat);
	REQUIRE(repeat.Match("aac")->myRange == "aa");
	REQUIRE(repeat.Match("aac")->mySubMatches.size() == 2);
	REQUIRE(repeat.Match("aac")->mySubMatches[0].myRange == "a");
	REQUIRE(repeat.Match("aac")->mySubMatches[1].myRange == "a");

	REQUIRE(repeat.Match("aaa"));
	REQUIRE(repeat.Match("aaa")->myPattern == &repeat);
	REQUIRE(repeat.Match("aaa")->myRange == "aaa");
	REQUIRE(repeat.Match("aaa")->mySubMatches.size() == 3);
	REQUIRE(repeat.Match("aaa")->mySubMatches[0].myRange == "a");
	REQUIRE(repeat.Match("aaa")->mySubMatches[1].myRange == "a");
	REQUIRE(repeat.Match("aaa")->mySubMatches[2].myRange == "a");

	REQUIRE(repeat.Match("aaaa"));
	REQUIRE(repeat.Match("aaaa")->myPattern == &repeat);
	REQUIRE(repeat.Match("aaaa")->myRange == "aaa");
	REQUIRE(repeat.Match("aaaa")->mySubMatches.size() == 3);
	REQUIRE(repeat.Match("aaaa")->mySubMatches[0].myRange == "a");
	REQUIRE(repeat.Match("aaaa")->mySubMatches[1].myRange == "a");
	REQUIRE(repeat.Match("aaaa")->mySubMatches[2].myRange == "a");
}