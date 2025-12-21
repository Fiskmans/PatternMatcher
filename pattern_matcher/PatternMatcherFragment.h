#pragma once

#include <algorithm>
#include <cassert>
#include <expected>
#include <format>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "pattern_matcher/Concepts.h"
#include "pattern_matcher/PatternMatchingTypes.h"
#include "pattern_matcher/RepeatCount.h"

enum class PatternMatcherFragmentType
{
    Literal,
    Repeat,
    Sequence,
    Alternative
};

class PatternMatcherFragment
{
public:
    using LiteralType = unsigned char;
    static_assert(sizeof(LiteralType) == sizeof(std::byte));

    PatternMatcherFragment() = default;
    PatternMatcherFragment(const LiteralType& aLiteral)
        : myType(PatternMatcherFragmentType::Literal), myLiteral(aLiteral)
    {
    }
    PatternMatcherFragment(const char& aLiteral) : PatternMatcherFragment((LiteralType)aLiteral) {}
    PatternMatcherFragment(PatternMatcherFragment* aSubPattern, RepeatCount aCount)
        : myType(PatternMatcherFragmentType::Repeat), mySubFragments({aSubPattern}), myCount(aCount)
    {
    }
    PatternMatcherFragment(PatternMatcherFragmentType aType, const std::vector<PatternMatcherFragment*> aFragments)
        : myType(aType), mySubFragments(aFragments)
    {
        assert(aType == PatternMatcherFragmentType::Sequence || aType == PatternMatcherFragmentType::Alternative);
        for (PatternMatcherFragment* frag : aFragments) assert(frag);
    }

    PatternMatcherFragment(const PatternMatcherFragment&)            = default;
    PatternMatcherFragment& operator=(const PatternMatcherFragment&) = default;

    PatternMatcherFragment(PatternMatcherFragment&&)            = default;
    PatternMatcherFragment& operator=(PatternMatcherFragment&&) = default;

    ~PatternMatcherFragment() = default;

    Expect Resolve()
    {
        if (myType == PatternMatcherFragmentType::Alternative)
        {
            for (const PatternMatcherFragment*& fragment : myLUT)
            {
                fragment = nullptr;
            }

            myLUTPortion = 0;
            for (const PatternMatcherFragment* fragment : mySubFragments)
            {
                if (fragment->myType != PatternMatcherFragmentType::Literal)
                    break;

                myLUT[(int)fragment->myLiteral] = fragment;
                myLUTPortion++;
            }
        }

        return {};
    }

    template<RangeComparable<LiteralType> TokenRange>
    MatchContext<TokenRange> BeginMatch(std::ranges::iterator_t<TokenRange> aBegin,
                                        std::ranges::sentinel_t<TokenRange> aEnd) const
    {
        MatchContext<TokenRange> ctx;

        ctx.myPattern = this;
        ctx.myBegin   = aBegin;
        ctx.myAt      = aBegin;
        ctx.myEnd     = aEnd;
        ctx.myIndex   = 0;

        return ctx;
    }

    MatchContext<std::string_view> BeginMatch(const char* aString) const
    {
        std::string_view view(aString);
        return BeginMatch<std::string_view>(std::ranges::begin(view), std::ranges::end(view));
    }

    template<RangeComparable<LiteralType> TokenRange>
    Result<TokenRange> ResumeMatch(MatchContext<TokenRange>& aContext, const Result<TokenRange>& aResult) const
    {
        switch (myType)
        {
            case PatternMatcherFragmentType::Literal:
                return LiteralMatch(aContext, aResult);
            case PatternMatcherFragmentType::Sequence:
                return SequenceMatch(aContext, aResult);
            case PatternMatcherFragmentType::Alternative:
                return AlternativeMatch(aContext, aResult);
            case PatternMatcherFragmentType::Repeat:
                return RepeatMatch(aContext, aResult);
        }

        assert(false);
        std::unreachable();
    }

private:
    template<RangeComparable<LiteralType> TokenRange>
    Result<TokenRange> LiteralMatch(MatchContext<TokenRange>& aContext, const Result<TokenRange>& aResult) const
    {
        if (myLiteral == *aContext.myAt)
            return MatchSuccess<TokenRange>{this, aContext.myAt, aContext.myAt + 1};

        return MatchFailure{};
    }

    template<RangeComparable<LiteralType> TokenRange>
    Result<TokenRange> SequenceMatch(MatchContext<TokenRange>& aContext, const Result<TokenRange>& aResult) const
    {
        switch (aResult.GetType())
        {
            case MatchResultType::Failure:
                return MatchFailure{};
            case MatchResultType::Success:
                aContext.mySubMatches.push_back(aResult.Success());
                aContext.myAt = aResult.Success().myEnd;
                break;
            case MatchResultType::None:
                break;
            case MatchResultType::InProgress:
            default:
                assert(false);
                std::unreachable();
                break;
        }

        if (aContext.myIndex == mySubFragments.size())
            return MatchSuccess<TokenRange>{this, aContext.myBegin, aContext.myAt, aContext.mySubMatches};

        return mySubFragments[aContext.myIndex++]->template BeginMatch<TokenRange>(aContext.myAt, aContext.myEnd);
    }

    template<RangeComparable<LiteralType> TokenRange>
    Result<TokenRange> AlternativeMatch(MatchContext<TokenRange>& aContext, const Result<TokenRange>& aResult) const
    {
        if (aContext.myIndex == 0)
        {
            if (aContext.myBegin != aContext.myEnd)
            {
                const PatternMatcherFragment* fragment = myLUT[char{*aContext.myAt}];

                if (fragment)
                {
                    return MatchSuccess<TokenRange>{
                        this,
                        aContext.myAt,
                        aContext.myAt + 1,
                        {MatchSuccess<TokenRange>{fragment, aContext.myAt, aContext.myAt + 1}}};
                }
            }

            aContext.myIndex += myLUTPortion;
        }

        switch (aResult.GetType())
        {
            case MatchResultType::Failure:
            case MatchResultType::None:
                break;
            case MatchResultType::Success:
                return MatchSuccess<TokenRange>{
                    this, aResult.Success().myBegin, aResult.Success().myEnd, {aResult.Success()}};
            case MatchResultType::InProgress:
            default:
                assert(false);
                std::unreachable();
                break;
        }

        if (aContext.myIndex == mySubFragments.size())
            return MatchFailure{};

        return mySubFragments[aContext.myIndex++]->template BeginMatch<TokenRange>(aContext.myBegin, aContext.myEnd);
    }

    template<RangeComparable<char> TokenRange>
    Result<TokenRange> RepeatMatch(MatchContext<TokenRange>& aContext, const Result<TokenRange>& aResult) const
    {
        switch (aResult.GetType())
        {
            case MatchResultType::Failure:

                if (aContext.myIndex > myCount.myMin)
                    return MatchSuccess<TokenRange>{this, aContext.myBegin, aContext.myAt, aContext.mySubMatches};

                return MatchFailure{};
            case MatchResultType::Success:
                aContext.mySubMatches.push_back(aResult.Success());
                aContext.myAt = aResult.Success().myEnd;
                break;
            case MatchResultType::None:
                break;
            case MatchResultType::InProgress:
            default:
                assert(false);
                std::unreachable();
                break;
        }

        if (aContext.myIndex == myCount.myMax)
            return MatchSuccess<TokenRange>{this, aContext.myBegin, aContext.myAt, aContext.mySubMatches};

        aContext.myIndex++;

        return mySubFragments[0]->template BeginMatch<TokenRange>(aContext.myAt, aContext.myEnd);
    }

private:
    const PatternMatcherFragment* myLUT[sizeof(LiteralType)];
    size_t myLUTPortion;
    PatternMatcherFragmentType myType = PatternMatcherFragmentType::Literal;
    LiteralType myLiteral;
    RepeatCount myCount;
    std::vector<PatternMatcherFragment*> mySubFragments;
};