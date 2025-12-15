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

template<class Key = std::string, std::ranges::range LiteralType = std::string>
class PatternMatcherFragment {
public:
    PatternMatcherFragment() = default;
    PatternMatcherFragment(const LiteralType& aLiteral)
        : myType(PatternMatcherFragmentType::Literal), myLiteral(aLiteral)
    {
    }
    PatternMatcherFragment(Key aKey, RepeatCount aCount)
        : myType(PatternMatcherFragmentType::Repeat), mySubFragments({{aKey, nullptr}}), myCount(aCount)
    {
    }
    PatternMatcherFragment(PatternMatcherFragmentType aType, const std::vector<Key> aKeys) : myType(aType)
    {
        assert(aType == PatternMatcherFragmentType::Sequence || aType == PatternMatcherFragmentType::Alternative);

        for (const Key& key : aKeys) mySubFragments.push_back({key, nullptr});
    }

    PatternMatcherFragment(const PatternMatcherFragment&)            = default;
    PatternMatcherFragment& operator=(const PatternMatcherFragment&) = default;

    PatternMatcherFragment(PatternMatcherFragment&&)            = default;
    PatternMatcherFragment& operator=(PatternMatcherFragment&&) = default;

    ~PatternMatcherFragment() = default;

    template<KeyedCollection<Key, PatternMatcherFragment&> Collection>
    Expect Resolve(const Collection& aFragments)
    {
        for (Mapping& mapping : mySubFragments)
        {
            auto resolved = aFragments.find(mapping.myKey);

            if (resolved == std::end(aFragments))
                return std::unexpected(std::format("Expected {} to be a valid fragment", mapping.myKey));

            mapping.myFragment = &resolved->second;
        }

        /*
        TODO: Figure out how to do this with the new structure

        if constexpr (Indexable::IsIndexable)
        {
                for (Super*& fragment : myQuickSearchLUT)
                {
                        fragment = nullptr;
                }
                myQuickSearchSize = 0;
                for (Super* fragment : myResolvedParts)
                {
                        LiteralFragment<Key, TokenRange>* lit =
        dynamic_cast<LiteralFragment<Key, TokenRange>*>(fragment);

                        if (!lit)
                                break;

                        if (std::ranges::size(lit->myLiteral) != 1)
                                break;

                        size_t c =
        Indexable::ConvertToIndex(*std::ranges::begin(lit->myLiteral));

                        myQuickSearchLUT[c] = fragment;
                        myQuickSearchSize++;
                }
        }
        */

        return {};
    }

    template<RangeComparable<LiteralType> TokenRange>
    MatchContext<Key, LiteralType, TokenRange> BeginMatch(TokenRange aRange) const
    {
        return BeginMatch<TokenRange>(std::ranges::begin(aRange), std::ranges::end(aRange));
    }

    template<RangeComparable<LiteralType> TokenRange>
    MatchContext<Key, LiteralType, TokenRange> BeginMatch(std::ranges::iterator_t<TokenRange> aBegin,
                                                          std::ranges::sentinel_t<TokenRange> aEnd) const
    {
        MatchContext<Key, LiteralType, TokenRange> ctx;

        ctx.myPattern = this;
        ctx.myBegin   = aBegin;
        ctx.myAt      = aBegin;
        ctx.myEnd     = aEnd;
        ctx.myIndex   = 0;

        return ctx;
    }

    MatchContext<Key, LiteralType, std::string_view> BeginMatch(const char* aRange)
    {
        return BeginMatch<std::string_view>(aRange);
    }

    template<RangeComparable<LiteralType> TokenRange>
    Result<Key, LiteralType, TokenRange> ResumeMatch(MatchContext<Key, LiteralType, TokenRange>& aContext,
                                                     const Result<Key, LiteralType, TokenRange>& aResult) const
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
    Result<Key, LiteralType, TokenRange> LiteralMatch(MatchContext<Key, LiteralType, TokenRange>& aContext,
                                                      const Result<Key, LiteralType, TokenRange>& aResult) const
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

        return MatchSuccess<Key, LiteralType, TokenRange>{this, aContext.myAt, at};
    }

    template<RangeComparable<LiteralType> TokenRange>
    Result<Key, LiteralType, TokenRange> SequenceMatch(MatchContext<Key, LiteralType, TokenRange>& aContext,
                                                       const Result<Key, LiteralType, TokenRange>& aResult) const
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
            return MatchSuccess<Key, LiteralType, TokenRange>{this, aContext.myBegin, aContext.myAt,
                                                              aContext.mySubMatches};

        return mySubFragments[aContext.myIndex++].myFragment->template BeginMatch<TokenRange>(aContext.myAt,
                                                                                              aContext.myEnd);
    }

    template<RangeComparable<LiteralType> TokenRange>
    Result<Key, LiteralType, TokenRange> AlternativeMatch(MatchContext<Key, LiteralType, TokenRange>& aContext,
                                                          const Result<Key, LiteralType, TokenRange>& aResult) const
    {
        /*
        TODO: Figure out how to do this with the new structure

        if constexpr (Indexable::IsIndexable)
        {
                if (aContext.myIndex == 0 && myQuickSearchSize > 0)
                {
                        if (std::ranges::size(aContext.myRange) > 0)
                        {
                                size_t c =
        Indexable::ConvertToIndex(*aContext.myAt);

                                IPatternMatcherFragment<Key, TokenRange>*
        fragment = myQuickSearchLUT[c];

                                if (fragment)
                                {
                                        TokenRange tokens { aContext.myAt,
        aContext.myAt + 1 }; return MatchSuccess<Key, TokenRange> { this,
        tokens, { MatchSuccess<Key, TokenRange>{ fragment, tokens } } };
                                }
                        }

                aContext.myIndex += myQuickSearchSize;
                }

        }
        */

        switch (aResult.GetType())
        {
            case MatchResultType::Failure:
            case MatchResultType::None:
                break;
            case MatchResultType::Success:
                return MatchSuccess<Key, LiteralType, TokenRange>{
                    this, aResult.Success().myBegin, aResult.Success().myEnd, {aResult.Success()}};
            case MatchResultType::InProgress:
            default:
                assert(false);
                std::unreachable();
                break;
        }

        if (aContext.myIndex == mySubFragments.size())
            return MatchFailure{};

        return mySubFragments[aContext.myIndex++].myFragment->template BeginMatch<TokenRange>(aContext.myBegin,
                                                                                              aContext.myEnd);
    }

    template<RangeComparable<LiteralType> TokenRange>
    Result<Key, LiteralType, TokenRange> RepeatMatch(MatchContext<Key, LiteralType, TokenRange>& aContext,
                                                     const Result<Key, LiteralType, TokenRange>& aResult) const
    {
        switch (aResult.GetType())
        {
            case MatchResultType::Failure:

                if (aContext.myIndex > myCount.myMin)
                    return MatchSuccess<Key, LiteralType, TokenRange>{this, aContext.myBegin, aContext.myAt,
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
            return MatchSuccess<Key, LiteralType, TokenRange>{this, aContext.myBegin, aContext.myAt,
                                                              aContext.mySubMatches};

        aContext.myIndex++;

        return mySubFragments[0].myFragment->template BeginMatch<TokenRange>(aContext.myAt, aContext.myEnd);
    }

private:
    struct Mapping {
        Key myKey;
        const PatternMatcherFragment* myFragment = nullptr;
    };

    PatternMatcherFragmentType myType = PatternMatcherFragmentType::Literal;
    LiteralType myLiteral;
    RepeatCount myCount;
    std::vector<Mapping> mySubFragments;
};