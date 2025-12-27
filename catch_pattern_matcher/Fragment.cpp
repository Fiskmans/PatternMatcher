
#include <catch2/catch_all.hpp>
#include <stack>

#include "pattern_matcher/PatternMatcher.h"

namespace pattern_matcher
{
    std::optional<MatchSuccess<std::string_view>> Match(Fragment& aFragment, std::string_view aText)
    {
        std::stack<MatchContext<std::string_view>> contexts;

        contexts.push(aFragment.BeginMatch<std::string_view>(std::ranges::begin(aText), std::ranges::end(aText)));

        Result<std::string_view> lastResult;

        while (!contexts.empty())
        {
            MatchContext<std::string_view>& ctx = contexts.top();

            lastResult = ctx.myPattern->ResumeMatch(ctx, lastResult);

            switch (lastResult.GetType())
            {
                case MatchResultType::Success:
                case MatchResultType::Failure:
                    contexts.pop();
                    break;

                case MatchResultType::InProgress:
                    contexts.push(lastResult.Context());
                    lastResult = {};
                    break;
                case MatchResultType::None:
                    assert(false);
                    break;
            }
        }

        switch (lastResult.GetType())
        {
            case MatchResultType::Success:
                return lastResult.Success();

            case MatchResultType::Failure:
                return {};

            case MatchResultType::None:
            case MatchResultType::InProgress:
                assert(false);
                break;
        }

        std::unreachable();
    }

    TEST_CASE("fragment::Literal", "[fragments]")
    {
        Fragment literal('a');

        auto start = literal.BeginMatch("a");

        {
            REQUIRE(*start.myAt == 'a');
            REQUIRE(start.myIndex == 0);
            REQUIRE(start.myPattern == &literal);
            REQUIRE(start == "a");
            REQUIRE(start.mySubMatches.empty());
        }

        {
            auto ctx = start;
            auto res = literal.ResumeMatch(ctx, {});

            REQUIRE(res.GetType() == MatchResultType::Success);
            REQUIRE(res.Success().myPattern == &literal);
        }

        REQUIRE(Match(literal, "ab"));
        REQUIRE(Match(literal, "abc"));
        REQUIRE(Match(literal, "abc")->mySubMatches.empty());
        REQUIRE(*Match(literal, "abc") == "a");
    }

    TEST_CASE("fragment::sequence", "[fragments]")
    {
        Fragment a('a');
        Fragment b('b');

        Fragment sequence(Fragment::Type::Sequence, {&a, &b});

        REQUIRE(!Match(sequence, "a"));
        REQUIRE(Match(sequence, "ab")->myPattern == &sequence);
        REQUIRE(*Match(sequence, "ab") == "ab");
        REQUIRE(Match(sequence, "ab")->mySubMatches.size() == 2);
        REQUIRE(Match(sequence, "ab")->mySubMatches[0].myPattern == &a);
        REQUIRE(Match(sequence, "ab")->mySubMatches[0] == "a");
        REQUIRE(Match(sequence, "ab")->mySubMatches[0].mySubMatches.empty());

        REQUIRE(Match(sequence, "ab")->mySubMatches[1].myPattern == &b);
        REQUIRE(Match(sequence, "ab")->mySubMatches[1] == "b");
        REQUIRE(Match(sequence, "ab")->mySubMatches[1].mySubMatches.empty());

        REQUIRE(Match(sequence, "abc"));
        REQUIRE(*Match(sequence, "abc") == "ab");
    }

    TEST_CASE("fragment::alternative", "[fragments]")
    {
        Fragment a('a');
        Fragment b('b');

        Fragment alternative(Fragment::Type::Alternative, {&a, &b});

        REQUIRE(Match(alternative, "a"));
        REQUIRE(Match(alternative, "a")->myPattern == &alternative);
        REQUIRE(*Match(alternative, "a") == "a");
        REQUIRE(Match(alternative, "a")->mySubMatches.size() == 1);
        REQUIRE(Match(alternative, "a")->mySubMatches[0].myPattern == &a);
        REQUIRE(Match(alternative, "a")->mySubMatches[0] == "a");
        REQUIRE(Match(alternative, "a")->mySubMatches[0].mySubMatches.empty());

        REQUIRE(Match(alternative, "b"));
        REQUIRE(Match(alternative, "b")->myPattern == &alternative);
        REQUIRE(*Match(alternative, "b") == "b");
        REQUIRE(Match(alternative, "b")->mySubMatches.size() == 1);
        REQUIRE(Match(alternative, "b")->mySubMatches[0].myPattern == &b);
        REQUIRE(Match(alternative, "b")->mySubMatches[0] == "b");
        REQUIRE(Match(alternative, "b")->mySubMatches[0].mySubMatches.empty());

        REQUIRE(!Match(alternative, "c"));
    }

    TEST_CASE("fragment::repeat", "[fragments]")
    {
        Fragment a('a');
        Fragment repeat(&a, RepeatCount(1, 3));

        REQUIRE(!Match(repeat, ""));
        REQUIRE(!Match(repeat, "c"));
        REQUIRE(Match(repeat, "a"));
        REQUIRE(Match(repeat, "a")->myPattern == &repeat);
        REQUIRE(*Match(repeat, "a") == "a");
        REQUIRE(Match(repeat, "a")->mySubMatches.size() == 1);
        REQUIRE(Match(repeat, "a")->mySubMatches[0] == "a");

        REQUIRE(Match(repeat, "ac"));
        REQUIRE(Match(repeat, "ac")->myPattern == &repeat);
        REQUIRE(*Match(repeat, "ac") == "a");
        REQUIRE(Match(repeat, "ac")->mySubMatches.size() == 1);
        REQUIRE(Match(repeat, "ac")->mySubMatches[0] == "a");

        REQUIRE(Match(repeat, "aa"));
        REQUIRE(Match(repeat, "aa")->myPattern == &repeat);
        REQUIRE(*Match(repeat, "aa") == "aa");
        REQUIRE(Match(repeat, "aa")->mySubMatches.size() == 2);
        REQUIRE(Match(repeat, "aa")->mySubMatches[0] == "a");
        REQUIRE(Match(repeat, "aa")->mySubMatches[1] == "a");

        REQUIRE(Match(repeat, "aac"));
        REQUIRE(Match(repeat, "aac")->myPattern == &repeat);
        REQUIRE(*Match(repeat, "aac") == "aa");
        REQUIRE(Match(repeat, "aac")->mySubMatches.size() == 2);
        REQUIRE(Match(repeat, "aac")->mySubMatches[0] == "a");
        REQUIRE(Match(repeat, "aac")->mySubMatches[1] == "a");

        REQUIRE(Match(repeat, "aaa"));
        REQUIRE(Match(repeat, "aaa")->myPattern == &repeat);
        REQUIRE(*Match(repeat, "aaa") == "aaa");
        REQUIRE(Match(repeat, "aaa")->mySubMatches.size() == 3);
        REQUIRE(Match(repeat, "aaa")->mySubMatches[0] == "a");
        REQUIRE(Match(repeat, "aaa")->mySubMatches[1] == "a");
        REQUIRE(Match(repeat, "aaa")->mySubMatches[2] == "a");

        REQUIRE(Match(repeat, "aaaa"));
        REQUIRE(Match(repeat, "aaaa")->myPattern == &repeat);
        REQUIRE(*Match(repeat, "aaaa") == "aaa");
        REQUIRE(Match(repeat, "aaaa")->mySubMatches.size() == 3);
        REQUIRE(Match(repeat, "aaaa")->mySubMatches[0] == "a");
        REQUIRE(Match(repeat, "aaaa")->mySubMatches[1] == "a");
        REQUIRE(Match(repeat, "aaaa")->mySubMatches[2] == "a");
    }
}  // namespace pattern_matcher