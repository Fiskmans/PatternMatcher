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
namespace pattern_matcher
{
    class Fragment
    {
    public:
        using Literal = unsigned char;
        static_assert(sizeof(Literal) == sizeof(std::byte));

        enum class Type
        {
            Literal,
            Repeat,
            Sequence,
            Alternative
        };

        Fragment() : myType(Type::Literal), myLiteral(0) {}
        Fragment(const Literal& aLiteral) : myType(Type::Literal), myLiteral(aLiteral) {}
        Fragment(Fragment* aSubPattern, RepeatCount aCount)
            : myType(Type::Repeat), mySubFragments({aSubPattern}), myCount(aCount)
        {
        }
        Fragment(Type aType, const std::vector<Fragment*> aFragments) : myType(aType), mySubFragments(aFragments)
        {
            assert(aType == Type::Sequence || aType == Type::Alternative);
            for (Fragment* frag : aFragments) assert(frag);

            if (myType == Type::Alternative)
            {
                for (const Fragment*& fragment : myLUT)
                {
                    fragment = nullptr;
                }

                myLUTPortion = 0;
                for (const Fragment* fragment : mySubFragments)
                {
                    if (fragment->myType != Type::Literal)
                        break;

                    myLUT[(int)fragment->myLiteral] = fragment;
                    myLUTPortion++;
                }
            }
        }

        Fragment(const Fragment&)            = default;
        Fragment& operator=(const Fragment&) = default;

        Fragment(Fragment&&)            = default;
        Fragment& operator=(Fragment&&) = default;

        ~Fragment() = default;

        Type GetType() const { return myType; }

        template<RangeComparable<Literal> TokenRange>
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

        template<RangeComparable<Literal> TokenRange>
        Result<TokenRange> ResumeMatch(MatchContext<TokenRange>& aContext, const Result<TokenRange>& aResult) const
        {
            switch (myType)
            {
                case Type::Literal:
                    return LiteralMatch(aContext, aResult);
                case Type::Sequence:
                    return SequenceMatch(aContext, aResult);
                case Type::Alternative:
                    return AlternativeMatch(aContext, aResult);
                case Type::Repeat:
                    return RepeatMatch(aContext, aResult);
            }

            assert(false);
            std::unreachable();
        }

    private:
        template<RangeComparable<Literal> TokenRange>
        Result<TokenRange> LiteralMatch(MatchContext<TokenRange>& aContext, const Result<TokenRange>& aResult) const
        {
            if (myLiteral == *aContext.myAt)
                return MatchSuccess<TokenRange>{this, aContext.myAt, aContext.myAt + 1};

            return MatchFailure{};
        }

        template<RangeComparable<Literal> TokenRange>
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

        template<RangeComparable<Literal> TokenRange>
        Result<TokenRange> AlternativeMatch(MatchContext<TokenRange>& aContext, const Result<TokenRange>& aResult) const
        {
            if (aContext.myIndex == 0 && myLUTPortion > 0)
            {
                if (aContext.myBegin != aContext.myEnd)
                {
                    std::ranges::range_value_t<TokenRange> v = *aContext.myAt;

                    const Fragment* fragment = myLUT[(Literal)v];

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

            return mySubFragments[aContext.myIndex++]->template BeginMatch<TokenRange>(aContext.myBegin,
                                                                                       aContext.myEnd);
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
        const Fragment* myLUT[1 << (sizeof(Literal) * CHAR_BIT)];
        size_t myLUTPortion;
        Type myType = Type::Literal;
        Literal myLiteral;
        RepeatCount myCount;
        std::vector<Fragment*> mySubFragments;
    };

}  // namespace pattern_matcher