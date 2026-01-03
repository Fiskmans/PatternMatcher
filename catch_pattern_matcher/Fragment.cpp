
#include <catch2/catch_all.hpp>
#include <stack>

#include "pattern_matcher/PatternMatcher.h"

namespace pattern_matcher
{
    std::optional<Success<const char*>> Match(Fragment& aFragment, std::string_view aText)
    {
        std::stack<MatchContext<const char*>> contexts;

        const char* end = std::ranges::end(aText);

        contexts.push(aFragment.BeginMatch(std::ranges::begin(aText)));

        Result<const char*> lastResult;

        while (!contexts.empty())
        {
            MatchContext<const char*>& ctx = contexts.top();

            lastResult = ctx.myFragment->ResumeMatch(ctx, lastResult, end);

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

        const char* str = "a";

        auto start = literal.BeginMatch(str);

        {
            REQUIRE(start.myAt == str);
            REQUIRE(*start.myAt == 'a');
            REQUIRE(start.myIndex == 0);
            REQUIRE(start.myFragment == &literal);
            REQUIRE(start.mySubMatches.empty());
        }

        {
            auto ctx = start;
            auto res = literal.ResumeMatch(ctx, {}, std::ranges::end(std::string_view(str)));

            REQUIRE(res.GetType() == MatchResultType::Success);
            REQUIRE(res.Success().myFragment == &literal);
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
        REQUIRE(Match(sequence, "ab")->myFragment == &sequence);
        REQUIRE(*Match(sequence, "ab") == "ab");
        REQUIRE(Match(sequence, "ab")->mySubMatches.size() == 2);
        REQUIRE(Match(sequence, "ab")->mySubMatches[0].myFragment == &a);
        REQUIRE(Match(sequence, "ab")->mySubMatches[0] == "a");
        REQUIRE(Match(sequence, "ab")->mySubMatches[0].mySubMatches.empty());

        REQUIRE(Match(sequence, "ab")->mySubMatches[1].myFragment == &b);
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
        REQUIRE(Match(alternative, "a")->myFragment == &alternative);
        REQUIRE(*Match(alternative, "a") == "a");
        REQUIRE(Match(alternative, "a")->mySubMatches.size() == 1);
        REQUIRE(Match(alternative, "a")->mySubMatches[0].myFragment == &a);
        REQUIRE(Match(alternative, "a")->mySubMatches[0] == "a");
        REQUIRE(Match(alternative, "a")->mySubMatches[0].mySubMatches.empty());

        REQUIRE(Match(alternative, "b"));
        REQUIRE(Match(alternative, "b")->myFragment == &alternative);
        REQUIRE(*Match(alternative, "b") == "b");
        REQUIRE(Match(alternative, "b")->mySubMatches.size() == 1);
        REQUIRE(Match(alternative, "b")->mySubMatches[0].myFragment == &b);
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
        REQUIRE(Match(repeat, "a")->myFragment == &repeat);
        REQUIRE(*Match(repeat, "a") == "a");
        REQUIRE(Match(repeat, "a")->mySubMatches.size() == 1);
        REQUIRE(Match(repeat, "a")->mySubMatches[0] == "a");

        REQUIRE(Match(repeat, "ac"));
        REQUIRE(Match(repeat, "ac")->myFragment == &repeat);
        REQUIRE(*Match(repeat, "ac") == "a");
        REQUIRE(Match(repeat, "ac")->mySubMatches.size() == 1);
        REQUIRE(Match(repeat, "ac")->mySubMatches[0] == "a");

        REQUIRE(Match(repeat, "aa"));
        REQUIRE(Match(repeat, "aa")->myFragment == &repeat);
        REQUIRE(*Match(repeat, "aa") == "aa");
        REQUIRE(Match(repeat, "aa")->mySubMatches.size() == 2);
        REQUIRE(Match(repeat, "aa")->mySubMatches[0] == "a");
        REQUIRE(Match(repeat, "aa")->mySubMatches[1] == "a");

        REQUIRE(Match(repeat, "aac"));
        REQUIRE(Match(repeat, "aac")->myFragment == &repeat);
        REQUIRE(*Match(repeat, "aac") == "aa");
        REQUIRE(Match(repeat, "aac")->mySubMatches.size() == 2);
        REQUIRE(Match(repeat, "aac")->mySubMatches[0] == "a");
        REQUIRE(Match(repeat, "aac")->mySubMatches[1] == "a");

        REQUIRE(Match(repeat, "aaa"));
        REQUIRE(Match(repeat, "aaa")->myFragment == &repeat);
        REQUIRE(*Match(repeat, "aaa") == "aaa");
        REQUIRE(Match(repeat, "aaa")->mySubMatches.size() == 3);
        REQUIRE(Match(repeat, "aaa")->mySubMatches[0] == "a");
        REQUIRE(Match(repeat, "aaa")->mySubMatches[1] == "a");
        REQUIRE(Match(repeat, "aaa")->mySubMatches[2] == "a");

        REQUIRE(Match(repeat, "aaaa"));
        REQUIRE(Match(repeat, "aaaa")->myFragment == &repeat);
        REQUIRE(*Match(repeat, "aaaa") == "aaa");
        REQUIRE(Match(repeat, "aaaa")->mySubMatches.size() == 3);
        REQUIRE(Match(repeat, "aaaa")->mySubMatches[0] == "a");
        REQUIRE(Match(repeat, "aaaa")->mySubMatches[1] == "a");
        REQUIRE(Match(repeat, "aaaa")->mySubMatches[2] == "a");
    }
}  // namespace pattern_matcher