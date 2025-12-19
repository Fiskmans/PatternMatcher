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

template<std::ranges::range LiteralType = std::string>
class PatternMatcherFragment {
public:
    PatternMatcherFragment() = default;
    PatternMatcherFragment(const LiteralType& aLiteral)
        : myType(PatternMatcherFragmentType::Literal), myLiteral(aLiteral)
    {
    }
    PatternMatcherFragment(PatternMatcherFragment* aSubPattern, RepeatCount aCount)
        : myType(PatternMatcherFragmentType::Repeat), mySubFragments({aSubPattern}), myCount(aCount)
    {
    }
    PatternMatcherFragment(PatternMatcherFragmentType aType, const std::vector<PatternMatcherFragment*> aFragments)
        : myType(aType), mySubFragments(aFragments)
    {
        assert(aType == PatternMatcherFragmentType::Sequence || aType == PatternMatcherFragmentType::Alternative);
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
            if constexpr (decltype(myLUT)::IsIndexable)
            {
                for (const PatternMatcherFragment*& fragment : myLUT.myValues)
                {
                    fragment = nullptr;
                }

                myLUTPortion = 0;
                for (const PatternMatcherFragment* fragment : mySubFragments)
                {
                    if (fragment->myType != PatternMatcherFragmentType::Literal)
                        break;

                    if (std::ranges::size(fragment->myLiteral) != 1)
                        break;

                    myLUT[*std::ranges::begin(fragment->myLiteral)] = fragment;
                    myLUTPortion++;
                }
            }
        }

        return {};
    }

    template<RangeComparable<LiteralType> TokenRange>
    MatchContext<LiteralType, TokenRange> BeginMatch(std::ranges::iterator_t<TokenRange> aBegin,
                                                     std::ranges::sentinel_t<TokenRange> aEnd) const
    {
        MatchContext<LiteralType, TokenRange> ctx;

        ctx.myPattern = this;
        ctx.myBegin   = aBegin;
        ctx.myAt      = aBegin;
        ctx.myEnd     = aEnd;
        ctx.myIndex   = 0;

        return ctx;
    }

    MatchContext<LiteralType, std::string_view> BeginMatch(const char* aString) const
    {
        std::string_view view(aString);
        return BeginMatch<std::string_view>(std::ranges::begin(view), std::ranges::end(view));
    }

    template<RangeComparable<LiteralType> TokenRange>
    Result<LiteralType, TokenRange> ResumeMatch(MatchContext<LiteralType, TokenRange>& aContext,
                                                const Result<LiteralType, TokenRange>& aResult) const
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
    Result<LiteralType, TokenRange> LiteralMatch(MatchContext<LiteralType, TokenRange>& aContext,
                                                 const Result<LiteralType, TokenRange>& aResult) const
    {
        std::ranges::iterator_t<TokenRange> at = aContext.myAt;

        std::ranges::iterator_t<const LiteralType> right    = std::ranges::begin(myLiteral);
        std::ranges::sentinel_t<const LiteralType> rightEnd = std::ranges::end(myLiteral);

        while (right != rightEnd)
        {
            if (at == aContext.myEnd)
                return MatchFailure{};

            if (*at != *right)
                return MatchFailure{};

            ++at;
            ++right;
        }

        return MatchSuccess<LiteralType, TokenRange>{this, aContext.myAt, at};
    }

    template<RangeComparable<LiteralType> TokenRange>
    Result<LiteralType, TokenRange> SequenceMatch(MatchContext<LiteralType, TokenRange>& aContext,
                                                  const Result<LiteralType, TokenRange>& aResult) const
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
            return MatchSuccess<LiteralType, TokenRange>{this, aContext.myBegin, aContext.myAt, aContext.mySubMatches};

        return mySubFragments[aContext.myIndex++]->template BeginMatch<TokenRange>(aContext.myAt, aContext.myEnd);
    }

    template<RangeComparable<LiteralType> TokenRange>
    Result<LiteralType, TokenRange> AlternativeMatch(MatchContext<LiteralType, TokenRange>& aContext,
                                                     const Result<LiteralType, TokenRange>& aResult) const
    {
        if constexpr (decltype(myLUT)::IsIndexable &&
                      std::convertible_to<std::ranges::range_value_t<TokenRange>, LiteralValueType>)
        {
            if (aContext.myIndex == 0)
            {
                if (aContext.myBegin != aContext.myEnd)
                {
                    const PatternMatcherFragment* fragment = myLUT[LiteralValueType{*aContext.myAt}];

                    if (fragment)
                    {
                        return MatchSuccess<LiteralType, TokenRange>{
                            this,
                            aContext.myAt,
                            aContext.myAt + 1,
                            {MatchSuccess<LiteralType, TokenRange>{fragment, aContext.myAt, aContext.myAt + 1}}};
                    }
                }

                aContext.myIndex += myLUTPortion;
            }
        }

        switch (aResult.GetType())
        {
            case MatchResultType::Failure:
            case MatchResultType::None:
                break;
            case MatchResultType::Success:
                return MatchSuccess<LiteralType, TokenRange>{
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

    template<RangeComparable<LiteralType> TokenRange>
    Result<LiteralType, TokenRange> RepeatMatch(MatchContext<LiteralType, TokenRange>& aContext,
                                                const Result<LiteralType, TokenRange>& aResult) const
    {
        switch (aResult.GetType())
        {
            case MatchResultType::Failure:

                if (aContext.myIndex > myCount.myMin)
                    return MatchSuccess<LiteralType, TokenRange>{this, aContext.myBegin, aContext.myAt,
                                                                 aContext.mySubMatches};

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
            return MatchSuccess<LiteralType, TokenRange>{this, aContext.myBegin, aContext.myAt, aContext.mySubMatches};

        aContext.myIndex++;

        return mySubFragments[0]->template BeginMatch<TokenRange>(aContext.myAt, aContext.myEnd);
    }

private:
    using LiteralValueType = std::ranges::range_value_t<LiteralType>;

    Indexable<LiteralValueType, const PatternMatcherFragment*> myLUT;
    size_t myLUTPortion;
    PatternMatcherFragmentType myType = PatternMatcherFragmentType::Literal;
    LiteralType myLiteral;
    RepeatCount myCount;
    std::vector<PatternMatcherFragment*> mySubFragments;
};