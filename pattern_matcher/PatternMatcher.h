#pragma once

#include <memory>
#include <ranges>
#include <stack>
#include <string>
#include <unordered_map>

#include "pattern_matcher/PatternMatcherFragment.h"

template<class Key = std::string>
class PatternMatcher
{
public:
    PatternMatcher()
    {
        PatternMatcherFragment::LiteralType i = std::numeric_limits<PatternMatcherFragment::LiteralType>::min();
        do
        {
            myBuiltIns[i] = {i};
            i++;
        } while (i < std::numeric_limits<PatternMatcherFragment::LiteralType>::max());
    };

    PatternMatcher(const PatternMatcher&)            = delete;
    PatternMatcher& operator=(const PatternMatcher&) = delete;

    PatternMatcher(PatternMatcher&&)            = default;
    PatternMatcher& operator=(PatternMatcher&&) = default;

    PatternMatcherFragment& AllocateFragment(Key aKey)
    {
        assert(myFragments.find(aKey) == myFragments.end());
        myFragments.emplace(aKey, PatternMatcherFragment{});

        return *operator[](aKey);
    }

    PatternMatcherFragment* operator[](Key aKey)
    {
        auto it = myFragments.find(aKey);
        if (it == myFragments.end())
            return nullptr;

        return &it->second;
    }

    PatternMatcherFragment* operator[](PatternMatcherFragment::LiteralType aLiteral) { return myBuiltIns + aLiteral; }

    std::vector<PatternMatcherFragment*> Of(std::string aList)
    {
        std::vector<PatternMatcherFragment*> out;

        for (char c : aList) out.push_back((*this)[(PatternMatcherFragment::LiteralType)c]);

        return out;
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

    template<RangeComparable<PatternMatcherFragment::LiteralType> TokenRange>
    void DebugDump(std::stack<MatchContext<TokenRange>> aStack)
    {
        if (aStack.empty())
            throw "No Contexts Supplied";

        std::stack<MatchContext<TokenRange>> copy(aStack);
        std::vector<MatchContext<TokenRange>> deStacked;

        while (!copy.empty())
        {
            deStacked.push_back(copy.top());
            copy.pop();
        }
        std::reverse(std::begin(deStacked), std::end(deStacked));
        std::vector<std::string> lines;
        lines.resize(deStacked.size());

        auto flushLines = [&lines]() {
            fprintf(stderr, "\n");
            for (int i = 0; i < lines.size(); i++)
            {
                fprintf(stderr, "%s%i\n", lines[i].c_str(), i);
                lines[i] = "";
            }
        };

        auto it = deStacked[0].begin();

        while (it != deStacked[0].end())
        {
            PatternMatcherFragment::LiteralType c = (PatternMatcherFragment::LiteralType)*it;

            if (c < std::numeric_limits<char>::max())
                fprintf(stderr, "%c", (char)c);
            else
                fprintf(stderr, "?");

            for (size_t i = 0; i < deStacked.size(); i++)
            {
                std::string& line             = lines[i];
                MatchContext<TokenRange>& ctx = deStacked[i];
                if (ctx.begin() == it)
                    line += '^';
                else if (line.empty())
                    line += ' ';
                else
                {
                    switch (line[line.length() - 1])
                    {
                        case ' ':
                            line += ' ';
                            break;
                        default:
                            line += '~';
                            break;
                    }
                }
            }
            if (lines[0].length() >= 80)
                flushLines();
            it++;
        }
        flushLines();
        fprintf(stderr, "\n");
    }

    template<RangeComparable<PatternMatcherFragment::LiteralType> TokenRange>
    std::optional<MatchSuccess<TokenRange>> Match(Key aRoot, TokenRange aRange, size_t aMaxDepth = 2'048,
                                                  size_t aMaxSteps = 4'294'967'296)
    {
        std::stack<MatchContext<TokenRange>> contexts;
        auto it = myFragments.find(aRoot);
        if (it == myFragments.end())
            throw "No such pattern";

        size_t steps = 0;

        contexts.push(it->second.template BeginMatch<TokenRange>(std::ranges::begin(aRange), std::ranges::end(aRange)));

        Result<TokenRange> lastResult;

        bool debugDump = false;

        while (!contexts.empty())
        {
            MatchContext<TokenRange>& ctx = contexts.top();

            const PatternMatcherFragment* fragment = ctx.myPattern;

            lastResult = ctx.myPattern->ResumeMatch(ctx, lastResult);

            if (debugDump)
                DebugDump(contexts);

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

    std::optional<MatchSuccess<std::string_view>> Match(Key aRoot, const char* aRange, size_t aMaxDepth = 2'048,
                                                        size_t aMaxSteps = 4'294'967'296)
    {
        return Match<std::string_view>(aRoot, aRange, aMaxDepth, aMaxSteps);
    }

private:
    std::unordered_map<Key, PatternMatcherFragment> myFragments;

    PatternMatcherFragment myBuiltIns[0x1 << (sizeof(PatternMatcherFragment::LiteralType) * CHAR_BIT)];
};
