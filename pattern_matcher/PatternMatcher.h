#pragma once

#include <memory>
#include <ranges>
#include <stack>
#include <string>
#include <unordered_map>

#include "pattern_matcher/PatternMatcherFragment.h"

template<class Key = std::string, std::ranges::range LiteralType = std::string>
class PatternMatcher {
public:
    PatternMatcher()                                 = default;
    PatternMatcher(const PatternMatcher&)            = delete;
    PatternMatcher& operator=(const PatternMatcher&) = delete;

    PatternMatcher(PatternMatcher&&)            = default;
    PatternMatcher& operator=(PatternMatcher&&) = default;

    PatternMatcherFragment<LiteralType>& AddFragment(Key aKey)
    {
        assert(myFragments.find(aKey) == myFragments.end());
        myFragments.emplace(aKey, PatternMatcherFragment<LiteralType>{});

        return *operator[](aKey);
    }

    void AddLiteral(Key aKey, LiteralType aLiteral)
    {
        assert(myFragments.find(aKey) == myFragments.end());
        myFragments.emplace(aKey, PatternMatcherFragment<LiteralType>(aLiteral));
    }

    PatternMatcherFragment<LiteralType>* operator[](Key aKey)
    {
        auto it = myFragments.find(aKey);
        if (it == myFragments.end())
            return nullptr;

        return &it->second;
    }

    Expect Resolve()
    {
        for (auto& [name, fragment] : myFragments)
        {
            Expect result = fragment.Resolve();
            if (!result)
                return result;
        }

        return {};
    }

    template<RangeComparable<LiteralType> TokenRange>
    std::optional<MatchSuccess<LiteralType, TokenRange>> Match(Key aRoot, TokenRange aRange, size_t aMaxDepth = 2'048,
                                                               size_t aMaxSteps = 4'294'967'296)
    {
        auto it = myFragments.find(aRoot);
        if (it == myFragments.end())
            return {};

        size_t steps = 0;

        std::stack<MatchContext<LiteralType, TokenRange>> contexts;

        contexts.push(it->second.template BeginMatch<TokenRange>(std::ranges::begin(aRange), std::ranges::end(aRange)));

        Result<LiteralType, TokenRange> lastResult;

        while (!contexts.empty())
        {
            MatchContext<LiteralType, TokenRange>& ctx = contexts.top();

            const PatternMatcherFragment<LiteralType>* fragment = ctx.myPattern;

            lastResult = ctx.myPattern->ResumeMatch(ctx, lastResult);

            switch (lastResult.GetType())
            {
                case MatchResultType::Success:
                case MatchResultType::Failure:
                    contexts.pop();
                    break;

                case MatchResultType::InProgress:
                    if (contexts.size() >= aMaxDepth)
                    {
                        lastResult = MatchFailure{};
                        break;
                    }
                    contexts.push(lastResult.Context());
                    lastResult = {};
                    break;
                case MatchResultType::None:
                    assert(false);
                    break;
            }

            if (steps++ >= aMaxSteps)
                return {};
        }

        switch (lastResult.GetType())
        {
            case MatchResultType::Success:
                return lastResult.Success();

            case MatchResultType::Failure:
                return {};

            case MatchResultType::InProgress:
            case MatchResultType::None:
                assert(false);
                break;
        }

        std::unreachable();
    }

    std::optional<MatchSuccess<LiteralType, std::string_view>> Match(Key aRoot, const char* aRange,
                                                                     size_t aMaxDepth = 2'048,
                                                                     size_t aMaxSteps = 4'294'967'296)
    {
        return Match<std::string_view>(aRoot, aRange, aMaxDepth, aMaxSteps);
    }

private:
    std::unordered_map<Key, PatternMatcherFragment<LiteralType>> myFragments;
};
