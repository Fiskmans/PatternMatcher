
#include <catch2/catch_all.hpp>

#include "pattern_matcher/PatternMatcher.h"

#include <stack>

std::optional<MatchSuccess> Match(IPatternMatcherFragment& aFragment, CharRange aText)
{
	std::stack<MatchContext> contexts;

	contexts.push(aFragment.Match(aText));

	Result lastResult;

	while (!contexts.empty())
	{
		MatchContext& ctx = contexts.top();

		lastResult = ctx.myPattern->Resume(ctx, lastResult);

		switch (lastResult.GetType())
		{
		case Result::Type::Success:
		case Result::Type::Failure:
			contexts.pop();
			break;

		case Result::Type::InProgress:
			contexts.push(lastResult.Context());
			lastResult = {};
			break;
		case Result::Type::None:
			assert(false);
			break;
		}
	}

	switch (lastResult.GetType())
	{
	case Result::Type::Success:
		return lastResult.Success();

	case Result::Type::Failure:
		return {};

	case Result::Type::None:
	case Result::Type::InProgress:
		assert(false);
		break;
	}

	std::unreachable();
}

TEST_CASE("pattern_matcher::fragments::LiteralFragment", "[fragments]")
{
	fragments::LiteralFragment literal("a", "a");

	MatchContext start = literal.Match("a");

	{
		REQUIRE(*start.myAt == 'a');
		REQUIRE(start.myIndex == 0);
		REQUIRE(start.myPattern == &literal);
		REQUIRE(start.myRange == "a");
		REQUIRE(start.mySubMatches.empty());
	}

	{
		MatchContext ctx = start;
		Result res = literal.Resume(ctx, MatchFailure{});

		REQUIRE(res.GetType() == Result::Type::Success);
		REQUIRE(res.Success().myPattern == &literal);
	}

	REQUIRE(Match(literal, "ab"));
	REQUIRE(Match(literal, "abc"));
	REQUIRE(Match(literal, "abc")->mySubMatches.empty());
	REQUIRE(Match(literal, "abc")->myRange == "a");
}

TEST_CASE("pattern_matcher::fragments::SequenceFragment", "[fragments]")
{
	fragments::SequenceFragment sequence("sequence", { "a", "b" });

	std::unordered_map<std::string, std::unique_ptr<IPatternMatcherFragment>> patterns;

	patterns["a"] = std::make_unique<fragments::LiteralFragment>("a", "a");
	patterns["b"] = std::make_unique<fragments::LiteralFragment>("b", "b");

	REQUIRE(sequence.Resolve(patterns));

	REQUIRE(!Match(sequence, "a"));
	REQUIRE(Match(sequence, "ab")->myPattern == &sequence);
	REQUIRE(Match(sequence, "ab")->myRange == "ab");
	REQUIRE(Match(sequence, "ab")->mySubMatches.size() == 2);
	REQUIRE(Match(sequence, "ab")->mySubMatches[0].myPattern == patterns["a"].get());
	REQUIRE(Match(sequence, "ab")->mySubMatches[0].myRange == "a");
	REQUIRE(Match(sequence, "ab")->mySubMatches[0].mySubMatches.empty());

	REQUIRE(Match(sequence, "ab")->mySubMatches[1].myPattern == patterns["b"].get());
	REQUIRE(Match(sequence, "ab")->mySubMatches[1].myRange == "b");
	REQUIRE(Match(sequence, "ab")->mySubMatches[1].mySubMatches.empty());

	REQUIRE(Match(sequence, "abc"));
	REQUIRE(Match(sequence, "abc")->myRange == "ab");
}

TEST_CASE("pattern_matcher::fragments::AlternativeFragment", "[fragments]")
{
	fragments::AlternativeFragment alternative("alt", { "a", "b" });

	std::unordered_map<std::string, std::unique_ptr<IPatternMatcherFragment>> patterns;

	patterns["a"] = std::make_unique<fragments::LiteralFragment>("a", "a");
	patterns["b"] = std::make_unique<fragments::LiteralFragment>("b", "b");

	REQUIRE(alternative.Resolve(patterns));

	REQUIRE(Match(alternative, "a"));
	REQUIRE(Match(alternative, "a")->myPattern == &alternative);
	REQUIRE(Match(alternative, "a")->myRange == "a");
	REQUIRE(Match(alternative, "a")->mySubMatches.size() == 1);
	REQUIRE(Match(alternative, "a")->mySubMatches[0].myPattern == patterns["a"].get());
	REQUIRE(Match(alternative, "a")->mySubMatches[0].myRange == "a");
	REQUIRE(Match(alternative, "a")->mySubMatches[0].mySubMatches.empty());

	REQUIRE(Match(alternative, "b"));
	REQUIRE(Match(alternative, "b")->myPattern == &alternative);
	REQUIRE(Match(alternative, "b")->myRange == "b");
	REQUIRE(Match(alternative, "b")->mySubMatches.size() == 1);
	REQUIRE(Match(alternative, "b")->mySubMatches[0].myPattern == patterns["b"].get());
	REQUIRE(Match(alternative, "b")->mySubMatches[0].myRange == "b");
	REQUIRE(Match(alternative, "b")->mySubMatches[0].mySubMatches.empty());

	REQUIRE(!Match(alternative, "c"));
}

TEST_CASE("pattern_matcher::fragments::RepeatFragment", "[fragments]")
{
	fragments::RepeatFragment repeat("rep", "a", RepeatCount( 1, 3 ));

	std::unordered_map<std::string, std::unique_ptr<IPatternMatcherFragment>> patterns;

	patterns["a"] = std::make_unique<fragments::LiteralFragment>("a", "a");

	REQUIRE(repeat.Resolve(patterns));

	REQUIRE(!Match(repeat, ""));
	REQUIRE(!Match(repeat, "c"));
	REQUIRE(Match(repeat, "a"));
	REQUIRE(Match(repeat, "a")->myPattern == &repeat);
	REQUIRE(Match(repeat, "a")->myRange == "a");
	REQUIRE(Match(repeat, "a")->mySubMatches.size() == 1);
	REQUIRE(Match(repeat, "a")->mySubMatches[0].myRange == "a");

	REQUIRE(Match(repeat, "ac"));
	REQUIRE(Match(repeat, "ac")->myPattern == &repeat);
	REQUIRE(Match(repeat, "ac")->myRange == "a");
	REQUIRE(Match(repeat, "ac")->mySubMatches.size() == 1);
	REQUIRE(Match(repeat, "ac")->mySubMatches[0].myRange == "a");

	REQUIRE(Match(repeat, "aa"));
	REQUIRE(Match(repeat, "aa")->myPattern == &repeat);
	REQUIRE(Match(repeat, "aa")->myRange == "aa");
	REQUIRE(Match(repeat, "aa")->mySubMatches.size() == 2);
	REQUIRE(Match(repeat, "aa")->mySubMatches[0].myRange == "a");
	REQUIRE(Match(repeat, "aa")->mySubMatches[1].myRange == "a");

	REQUIRE(Match(repeat, "aac"));
	REQUIRE(Match(repeat, "aac")->myPattern == &repeat);
	REQUIRE(Match(repeat, "aac")->myRange == "aa");
	REQUIRE(Match(repeat, "aac")->mySubMatches.size() == 2);
	REQUIRE(Match(repeat, "aac")->mySubMatches[0].myRange == "a");
	REQUIRE(Match(repeat, "aac")->mySubMatches[1].myRange == "a");

	REQUIRE(Match(repeat, "aaa"));
	REQUIRE(Match(repeat, "aaa")->myPattern == &repeat);
	REQUIRE(Match(repeat, "aaa")->myRange == "aaa");
	REQUIRE(Match(repeat, "aaa")->mySubMatches.size() == 3);
	REQUIRE(Match(repeat, "aaa")->mySubMatches[0].myRange == "a");
	REQUIRE(Match(repeat, "aaa")->mySubMatches[1].myRange == "a");
	REQUIRE(Match(repeat, "aaa")->mySubMatches[2].myRange == "a");

	REQUIRE(Match(repeat, "aaaa"));
	REQUIRE(Match(repeat, "aaaa")->myPattern == &repeat);
	REQUIRE(Match(repeat, "aaaa")->myRange == "aaa");
	REQUIRE(Match(repeat, "aaaa")->mySubMatches.size() == 3);
	REQUIRE(Match(repeat, "aaaa")->mySubMatches[0].myRange == "a");
	REQUIRE(Match(repeat, "aaaa")->mySubMatches[1].myRange == "a");
	REQUIRE(Match(repeat, "aaaa")->mySubMatches[2].myRange == "a");
}