#pragma once

#include <memory>
#include <ranges>
#include <stack>
#include <string>
#include <unordered_map>

#include "PatternMatcherFragment.h"

template<class Key = std::string, std::ranges::range LiteralType = std::string>
class PatternMatcher {
public:
    PatternMatcher() = default;

    void AddFragment(Key aKey, PatternMatcherFragment<Key, LiteralType>&& aFragment)
    {
        assert(myFragments.find(aKey) == myFragments.end());

        myFragments.emplace(aKey, std::move(aFragment));
    }

    void AddLiteral(Key aKey, LiteralType aLiteral)
    {
        AddFragment(aKey, PatternMatcherFragment<Key, LiteralType>(aLiteral));
    }

    Expect Resolve()
    {
        for (auto& [name, fragment] : myFragments)
        {
            Expect result = fragment.Resolve(myFragments);
            if (!result)
                return result;
        }

        return {};
    }

    template<RangeComparable<LiteralType> TokenRange>
    std::optional<MatchSuccess<Key, LiteralType, TokenRange>> Match(Key aRoot, TokenRange aRange,
                                                                    size_t aMaxDepth = 2'048,
                                                                    size_t aMaxSteps = 4'294'967'296)
    {
        auto it = myFragments.find(aRoot);
        if (it == myFragments.end())
            return {};

        size_t steps = 0;

        std::stack<MatchContext<Key, LiteralType, TokenRange>> contexts;

        contexts.push(it->second.BeginMatch(aRange));

        Result<Key, LiteralType, TokenRange> lastResult;

        while (!contexts.empty())
        {
            MatchContext<Key, LiteralType, TokenRange>& ctx = contexts.top();

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

    std::optional<MatchSuccess<Key, LiteralType, std::string_view>> Match(Key aRoot, const char* aRange,
                                                                          size_t aMaxDepth = 2'048,
                                                                          size_t aMaxSteps = 4'294'967'296)
    {
        return Match<std::string_view>(aRoot, aRange, aMaxDepth, aMaxSteps);
    }

private:
    std::unordered_map<Key, PatternMatcherFragment<Key, LiteralType>> myFragments;
};
