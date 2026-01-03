
#include "pattern_matcher/PatternBuilder.h"

#include <catch2/catch_all.hpp>

template<std::ranges::range Left, std::ranges::range Right>
void RequireEqual(Left aLeft, Right aRight)
{
    auto l = std::ranges::begin(aLeft);
    auto r = std::ranges::begin(aRight);

    auto le = std::ranges::end(aLeft);
    auto re = std::ranges::end(aRight);

    while (true)
    {
        if (l == le)
            break;
        if (r == re)
            break;

        REQUIRE(*l == *r);

        ++l;
        ++r;
    }

    REQUIRE(l == le);
    REQUIRE(r == re);
}

TEST_CASE("builder::basic")
{
    using namespace std::string_view_literals;
    using namespace pattern_matcher;

    PatternBuilder builder;

    builder["a"] = "a";
    builder["b"] = "b";
    builder["c"] = "c";

    builder["any"] || "a" || "b" || "c";
    builder["all"] && "a" && "b" && "c";

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

TEST_CASE("builder::bnf")
{
    using namespace std::string_view_literals;
    using namespace pattern_matcher;

    PatternMatcher bnfMatcher = PatternBuilder::Builtin::BNF();

    std::string spec = R"(
foo:
        bar
    
bar:
        b a r

baz:
        b? a* r+

bat:
        b
        a
        r
)";

    using Success                 = Success<std::ranges::iterator_t<std::string>>;
    std::optional<Success> result = bnfMatcher.Match("doc", spec);

    REQUIRE(result);

    {
        std::string resultString(std::ranges::begin(*result), std::ranges::end(*result));
        REQUIRE(resultString == spec);
    }

    REQUIRE(result->mySubMatches.size() == 8);

    auto declarations = result->SearchFor(bnfMatcher["decl"]);

    auto it  = std::ranges::begin(declarations);
    auto end = std::ranges::end(declarations);

    REQUIRE(it != end);

    Success& foo = *it;
    ++it;
    REQUIRE(it != end);

    Success& bar = *it;
    ++it;
    REQUIRE(it != end);

    Success& baz = *it;
    ++it;
    REQUIRE(it != end);

    Success& bat = *it;
    ++it;
    REQUIRE(it == end);

    {
        REQUIRE(foo.myFragment == bnfMatcher["decl"]);
        REQUIRE(foo[1].myFragment == bnfMatcher["identifier"]);
        REQUIRE(foo[1] == "foo");
        REQUIRE(foo[3].myFragment == bnfMatcher["colon"]);
        REQUIRE(foo[4].myFragment == bnfMatcher["values"]);

        {
            auto values    = foo.SearchFor(bnfMatcher["value"]);
            auto valuesIt  = std::ranges::begin(values);
            auto valuesEnd = std::ranges::end(values);

            REQUIRE(valuesIt != valuesEnd);

            RequireEqual((*valuesIt).SearchFor(bnfMatcher["identifier"]), std::vector<std::string>{"bar"});

            ++valuesIt;
            REQUIRE(valuesIt == valuesEnd);
        }
    }

    REQUIRE(bar.myFragment == bnfMatcher["decl"]);
    REQUIRE(bar[1] == "bar");

    {
        auto values    = bar.SearchFor(bnfMatcher["value"]);
        auto valuesIt  = std::ranges::begin(values);
        auto valuesEnd = std::ranges::end(values);

        REQUIRE(valuesIt != valuesEnd);

        RequireEqual((*valuesIt).SearchFor(bnfMatcher["identifier"]), std::vector<std::string>{"b", "a", "r"});

        ++valuesIt;
        REQUIRE(valuesIt == valuesEnd);
    }

    REQUIRE(baz.myFragment == bnfMatcher["decl"]);
    REQUIRE(baz[1] == "baz");

    {
        auto values    = baz.SearchFor(bnfMatcher["value"]);
        auto valuesIt  = std::ranges::begin(values);
        auto valuesEnd = std::ranges::end(values);

        REQUIRE(valuesIt != valuesEnd);

        RequireEqual((*valuesIt).SearchFor(bnfMatcher["value-part"]),
                     std::vector<std::string>{"        b?", " a*", " r+"});
        RequireEqual((*valuesIt).SearchFor(bnfMatcher["whitespace"]), std::vector<std::string>{"        ", " ", " "});
        RequireEqual((*valuesIt).SearchFor(bnfMatcher["identifier"]), std::vector<std::string>{"b", "a", "r"});
        RequireEqual((*valuesIt).SearchFor(bnfMatcher["repeat"]), std::vector<std::string>{"?", "*", "+"});

        ++valuesIt;
        REQUIRE(valuesIt == valuesEnd);
    }

    REQUIRE(bat.myFragment == bnfMatcher["decl"]);
    REQUIRE(bat[1] == "bat");

    {
        auto values    = bat.SearchFor(bnfMatcher["value"]);
        auto valuesIt  = std::ranges::begin(values);
        auto valuesEnd = std::ranges::end(values);

        REQUIRE(valuesIt != valuesEnd);
        RequireEqual((*valuesIt).SearchFor(bnfMatcher["value-part"]), std::vector<std::string>{"        b"});

        ++valuesIt;
        REQUIRE(valuesIt != valuesEnd);
        RequireEqual((*valuesIt).SearchFor(bnfMatcher["value-part"]), std::vector<std::string>{"        a"});

        ++valuesIt;
        REQUIRE(valuesIt != valuesEnd);
        RequireEqual((*valuesIt).SearchFor(bnfMatcher["value-part"]), std::vector<std::string>{"        r"});

        ++valuesIt;
        REQUIRE(valuesIt == valuesEnd);
    }

    PatternMatcher matcher = PatternBuilder::FromBNF(spec);

    REQUIRE(matcher["foo"]->GetType() == Fragment::Type::Sequence);
    REQUIRE(matcher["foo"]->SubFragments().size() == 1);
    REQUIRE(matcher["foo"]->SubFragments()[0] == matcher["bar"]);

    REQUIRE(matcher["bar"]);
    REQUIRE(matcher["bar"]->GetType() == Fragment::Type::Sequence);
    REQUIRE(matcher["bar"]->SubFragments()[0] == matcher['b']);
    REQUIRE(matcher["bar"]->SubFragments()[1] == matcher['a']);
    REQUIRE(matcher["bar"]->SubFragments()[2] == matcher['r']);

    REQUIRE(matcher["baz"]);
    REQUIRE(matcher["baz"]->GetType() == Fragment::Type::Sequence);
    REQUIRE(matcher["baz"]->SubFragments()[0] == matcher["b-optional"]);
    REQUIRE(matcher["baz"]->SubFragments()[0]->GetType() == Fragment::Type::Repeat);
    REQUIRE(matcher["baz"]->SubFragments()[0]->SubFragments()[0] == matcher['b']);

    REQUIRE(matcher["baz"]->SubFragments()[1] == matcher["a-any"]);
    REQUIRE(matcher["baz"]->SubFragments()[1]->GetType() == Fragment::Type::Repeat);
    REQUIRE(matcher["baz"]->SubFragments()[1]->SubFragments()[0] == matcher['a']);

    REQUIRE(matcher["baz"]->SubFragments()[2] == matcher["r-repeated"]);
    REQUIRE(matcher["baz"]->SubFragments()[2]->GetType() == Fragment::Type::Repeat);
    REQUIRE(matcher["baz"]->SubFragments()[2]->SubFragments()[0] == matcher['r']);

    REQUIRE(matcher["bat"]);
    REQUIRE(matcher["bat"]->GetType() == Fragment::Type::Alternative);
    REQUIRE(matcher["bat"]->SubFragments().size() == 3);
    REQUIRE(matcher["bat"]->SubFragments()[0]->GetType() == Fragment::Type::Sequence);
    REQUIRE(matcher["bat"]->SubFragments()[0]->SubFragments()[0] == matcher['b']);
    REQUIRE(matcher["bat"]->SubFragments()[0] == matcher["bat-0"]);

    REQUIRE(matcher["bat"]->SubFragments()[1]->GetType() == Fragment::Type::Sequence);
    REQUIRE(matcher["bat"]->SubFragments()[1]->SubFragments()[0] == matcher['a']);
    REQUIRE(matcher["bat"]->SubFragments()[1] == matcher["bat-1"]);

    REQUIRE(matcher["bat"]->SubFragments()[2]->GetType() == Fragment::Type::Sequence);
    REQUIRE(matcher["bat"]->SubFragments()[2]->SubFragments()[0] == matcher['r']);
    REQUIRE(matcher["bat"]->SubFragments()[2] == matcher["bat-2"]);

    REQUIRE(*matcher.Match("foo", "bar") == "bar");

    REQUIRE(*matcher.Match("bar", "bar") == "bar");

    REQUIRE(*matcher.Match("baz", "bar") == "bar");
    REQUIRE(*matcher.Match("baz", "ar") == "ar");
    REQUIRE(!matcher.Match("baz", "a"));
    REQUIRE(*matcher.Match("baz", "r") == "r");
    REQUIRE(*matcher.Match("baz", "rr") == "rr");
    REQUIRE(*matcher.Match("baz", "aar") == "aar");
    REQUIRE(*matcher.Match("baz", "aarr") == "aarr");
    REQUIRE(*matcher.Match("baz", "baarr") == "baarr");
    REQUIRE(!matcher.Match("baz", "bbaarr"));

    REQUIRE(*matcher.Match("bat", "b") == "b");
    REQUIRE(*matcher.Match("bat", "a") == "a");
    REQUIRE(*matcher.Match("bat", "r") == "r");

    REQUIRE(*matcher.Match("bat", "bar") == "b");
}
