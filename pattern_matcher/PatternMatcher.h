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
            for (size_t i = 0; i < std::numeric_limits<Fragment::Literal>::max(); i++) myLiterals[i] = i;
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

        Fragment* operator[](Fragment::Literal aLiteral) { return myLiterals + aLiteral; }

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
                    out.push_back(myLiterals + i);

                i++;
            } while (i < std::numeric_limits<Fragment::Literal>::max());

            return out;
        }

        template<class Iterator, class Sentinel>
        void DebugDump(std::stack<MatchContext<Iterator>> aStack, Sentinel aEnd)
        {
            const int height = 8;
            const int width  = 120;  // does not include name of the fragments

            if (aStack.empty())
                throw "No Contexts Supplied";

            std::stack<MatchContext<Iterator>> copy(aStack);
            std::vector<MatchContext<Iterator>> deStacked;

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

                int index = 0;

                // Cut leading lines
                while (index + height < lines.size())
                {
                    lines[index] = "";
                    index++;
                }

                for (; index < lines.size(); index++)
                {
                    const Fragment* fragment = deStacked[index].myFragment;
                    std::string name         = "<external>";

                    if (fragment->GetType() == Fragment::Type::Literal)
                    {
                        if (fragment > std::begin(myLiterals) && fragment < std::end(myLiterals))
                            name = "Literal " + std::to_string(fragment - myLiterals);

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

                    fprintf(stderr, "%s %3i: %s[%i]", lines[index].c_str(), index, name.c_str(),
                            deStacked[index].myIndex);

                    if (deStacked[index].mySubMatches.size() > 0)
                        fprintf(stderr, " (matches so far: %i)\n", (int)deStacked[index].mySubMatches.size());
                    else
                        fprintf(stderr, "\n");

                    lines[index] = "";
                }

                // pad to fit
                for (int i = 0; i < height - (int)lines.size(); i++) fprintf(stderr, "\n");
            };

            auto it = deStacked[0].myAt;

            while (it != aEnd)
            {
                Fragment::Literal c = (Fragment::Literal)*it;

                if (c < std::numeric_limits<char>::max())
                    switch (c)
                    {
                        case '\n':
                            fprintf(stderr, "\u00b6");  // ¶
                            break;
                        case '\t':
                            fprintf(stderr, "\u2192");  // →

                        default:
                            fprintf(stderr, "%c", (char)c);
                    }
                else
                    fprintf(stderr, "?");

                for (size_t i = 0; i < deStacked.size(); i++)
                {
                    std::string& line           = lines[i];
                    MatchContext<Iterator>& ctx = deStacked[i];
                    if (ctx.myAt == it)
                        line += '^';
                    else if (ctx.myBegin == it)
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
                if (lines[0].length() >= width)
                    break;
                it++;
            }
            flushLines();
        }

        template<class Iterator>
        void DebugDump(Result<Iterator> aRes)
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

        template<std::ranges::range Range>
        std::optional<Success<std::ranges::iterator_t<Range>>> Match(Key aRoot, Range& aRange, size_t aMaxDepth = 2'048,
                                                                     size_t aMaxSteps = 4'294'967'296)
        {
            return Match(this->operator[](aRoot), std::ranges::begin(aRange), std::ranges::end(aRange), aMaxDepth,
                         aMaxSteps);
        }
        template<std::ranges::range Range>
        std::optional<Success<std::ranges::iterator_t<Range>>> Match(const Fragment* aRoot, Range& aRange,
                                                                     size_t aMaxDepth = 2'048,
                                                                     size_t aMaxSteps = 4'294'967'296)
        {
            return Match(aRoot, std::ranges::begin(aRange), std::ranges::end(aRange), aMaxDepth, aMaxSteps);
        }

        template<class Iterator, class Sentinel>
        std::optional<Success<Iterator>> Match(const Fragment* aRoot, Iterator aBegin, Sentinel aEnd,
                                               size_t aMaxDepth = 2'048, size_t aMaxSteps = 4'294'967'296)
        {
            size_t steps = 0;
            std::stack<MatchContext<Iterator>> contexts;

            contexts.push(aRoot->BeginMatch(aBegin));

            Result<Iterator> lastResult;

            bool debugDump = false;

            while (!contexts.empty())
            {
                MatchContext<Iterator>& ctx = contexts.top();

                const Fragment* fragment = ctx.myFragment;

                if (debugDump)
                    DebugDump(contexts, aEnd);

                lastResult = ctx.myFragment->ResumeMatch(ctx, lastResult, aEnd);

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

        std::optional<Success<const char*>> Match(Key aRoot, const char* aRange, size_t aMaxDepth = 2'048,
                                                  size_t aMaxSteps = 4'294'967'296)
        {
            std::string_view view(aRange);
            return Match(this->operator[](aRoot), view.begin(), view.end(), aMaxDepth, aMaxSteps);
        }

    private:
        std::unordered_map<Key, Fragment> myFragments;

        Fragment myLiterals[256];
    };

}  // namespace pattern_matcher
