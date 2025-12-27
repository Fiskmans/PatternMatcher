#pragma once

#include <memory>
#include <ranges>
#include <stack>
#include <string>
#include <unordered_map>

#include "pattern_matcher/Fragment.h"

namespace pattern_matcher
{

    template<class Key = std::string>
    class PatternMatcher
    {
    public:
        PatternMatcher()
        {
            for (size_t i = 0; i < std::numeric_limits<Fragment::Literal>::max(); i++) myBuiltIns[i] = i;
        };

        PatternMatcher(const PatternMatcher&)            = delete;
        PatternMatcher& operator=(const PatternMatcher&) = delete;

        PatternMatcher(PatternMatcher&&)            = default;
        PatternMatcher& operator=(PatternMatcher&&) = default;

        template<class... T>
        Fragment& EmplaceFragment(Key aKey, T&&... aArgs)
        {
            auto insertionResult = myFragments.emplace(std::piecewise_construct, std::forward_as_tuple(aKey),
                                                       std::forward_as_tuple(aArgs...));

            assert(insertionResult.second);

            return insertionResult.first->second;
        }

        Fragment* operator[](Key aKey)
        {
            auto it = myFragments.find(aKey);
            if (it == myFragments.end())
                return nullptr;

            return &it->second;
        }

        Fragment* operator[](Fragment::Literal aLiteral) { return myBuiltIns + aLiteral; }

        std::vector<Fragment*> Of(std::string aList)
        {
            std::vector<Fragment*> out;

            for (char c : aList) out.push_back((*this)[(Fragment::Literal)c]);

            return out;
        }

        std::vector<Fragment*> NotOf(std::string aList)
        {
            std::vector<Fragment*> out;
            Fragment::Literal i = std::numeric_limits<Fragment::Literal>::min();
            do
            {
                if (aList.find(i) == std::string::npos)
                    out.push_back(myBuiltIns + i);

                i++;
            } while (i < std::numeric_limits<Fragment::Literal>::max());

            return out;
        }

        template<RangeComparable<Fragment::Literal> TokenRange>
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

            auto flushLines = [&lines, &deStacked, this]() {
                fprintf(stderr, "\n");
                for (int i = 0; i < lines.size(); i++)
                {
                    const Fragment* fragment = deStacked[i].myPattern;
                    std::string name         = "<external>";

                    if (fragment->GetType() == Fragment::Type::Literal)
                    {
                        if (fragment > std::begin(myBuiltIns) && fragment < std::end(myBuiltIns))
                            name = "Literal " + std::to_string(fragment - myBuiltIns);

                    } else
                    {
                        for (auto& kvPair : myFragments)
                        {
                            if (&kvPair.second == fragment)
                            {
                                name = kvPair.first;
                                break;
                            }
                        }
                    }

                    fprintf(stderr, "%s %3i: %s[%i]\n", lines[i].c_str(), i, name.c_str(), deStacked[i].myIndex);
                    lines[i] = "";
                }
            };

            auto it = deStacked[0].begin();

            while (it != deStacked[0].end())
            {
                Fragment::Literal c = (Fragment::Literal)*it;

                if (c < std::numeric_limits<char>::max())
                    fprintf(stderr, "%c", (char)c);
                else
                    fprintf(stderr, "?");

                for (size_t i = 0; i < deStacked.size(); i++)
                {
                    std::string& line             = lines[i];
                    MatchContext<TokenRange>& ctx = deStacked[i];
                    if (ctx.myAt == it)
                        line += '^';
                    else if (ctx.begin() == it)
                        line += '~';
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
                    break;
                it++;
            }
            flushLines();
        }

        template<RangeComparable<Fragment::Literal> TokenRange>
        void DebugDump(Result<TokenRange> aRes)
        {
            switch (aRes.GetType())
            {
                case MatchResultType::Success:
                    fprintf(stderr, "\x1b[1;32mSuccess\x1b[0m\n");
                    break;
                case MatchResultType::Failure:
                    fprintf(stderr, "\x1b[1;31mFailure\x1b[0m\n");
                    break;
                case MatchResultType::InProgress:
                    fprintf(stderr, "\x1b[1;33mSubmatch\x1b[0m\n");
                    break;
                case MatchResultType::None:
                    fprintf(stderr, "<ERROR>\n");
                    break;
            }

            getc(stdin);
        }

        template<RangeComparable<Fragment::Literal> TokenRange>
        std::optional<MatchSuccess<TokenRange>> Match(Key aRoot, TokenRange aRange, size_t aMaxDepth = 2'048,
                                                      size_t aMaxSteps = 4'294'967'296)
        {
            std::stack<MatchContext<TokenRange>> contexts;
            auto it = myFragments.find(aRoot);
            if (it == myFragments.end())
                throw "No such pattern";

            size_t steps = 0;

            contexts.push(
                it->second.template BeginMatch<TokenRange>(std::ranges::begin(aRange), std::ranges::end(aRange)));

            Result<TokenRange> lastResult;

            bool debugDump = true;

            while (!contexts.empty())
            {
                MatchContext<TokenRange>& ctx = contexts.top();

                const Fragment* fragment = ctx.myPattern;

                if (debugDump)
                    DebugDump(contexts);

                lastResult = ctx.myPattern->ResumeMatch(ctx, lastResult);

                if (debugDump)
                    DebugDump(lastResult);

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
        std::unordered_map<Key, Fragment> myFragments;

        Fragment myBuiltIns[256];
    };

}  // namespace pattern_matcher
