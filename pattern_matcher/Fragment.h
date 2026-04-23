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
        using SmallIndex = unsigned char;
        static constexpr SmallIndex NoIndex = std::numeric_limits<SmallIndex>::max();
        static_assert(sizeof(Literal) == sizeof(std::byte));

        enum class Type
        {
            None,
            Literal,
            Repeat,
            Sequence,
            Alternative
        };

        Fragment() : myType(Type::None), myLiteral(0) {}
        Fragment(const Literal& aLiteral) : myType(Type::Literal), myLiteral(aLiteral) {}
        Fragment(const Fragment* aSubPattern, RepeatCount aCount)
            : myType(Type::Repeat), mySubFragments({aSubPattern}), myCount(aCount)
        {
        }
        Fragment(Type aType, const std::vector<const Fragment*> aFragments) : myType(aType), mySubFragments(aFragments)
        {
            assert(aType == Type::Sequence || aType == Type::Alternative);
            for (const Fragment* frag : aFragments) assert(frag);

            if (myType == Type::Alternative)
            {
                for (SmallIndex& fragment : myLUT)
                {
                    fragment = NoIndex;
                }

                myLUTPortion = 0;
                for (SmallIndex i = 0; i < mySubFragments.size(); i++)
                {
                    if (i == NoIndex)
                        break;

                    if (mySubFragments[i]->myType != Type::Literal)
                        break;

                    myLUT[(int)mySubFragments[i]->myLiteral] = i;
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
        const std::vector<const Fragment*>& SubFragments() const { return mySubFragments; }

        template<class Iterator>
        MatchContext<Iterator> BeginMatch(Iterator aBegin) const
        {
            MatchContext<Iterator> ctx;

            ctx.myFragment = this;
            ctx.myBegin    = aBegin;
            ctx.myAt       = aBegin;
            ctx.myIndex    = 0;

            return ctx;
        }

        template<class Iterator, class Sentinel>
            requires std::equality_comparable_with<std::iter_value_t<Iterator>, Literal>
                  && std::equality_comparable_with<Iterator, Sentinel>
        Result<Iterator> ResumeMatch(MatchContext<Iterator>& aContext, const Result<Iterator>& aResult,
                                     Sentinel aEnd) const
        {
            switch (myType)
            {
                case Type::Literal:
                    return LiteralMatch(aContext, aResult, aEnd);
                case Type::Sequence:
                    return SequenceMatch(aContext, aResult);
                case Type::Alternative:
                    return AlternativeMatch(aContext, aResult, aEnd);
                case Type::Repeat:
                    return RepeatMatch(aContext, aResult);

                case Type::None:
                    break;
            }

            assert(false);
            std::unreachable();
        }

    private:
        template<class Iterator, class Sentinel>
        Result<Iterator> LiteralMatch(MatchContext<Iterator>& aContext, const Result<Iterator>& aResult,
                                      Sentinel aEnd) const
        {
            if (aContext.myAt == aEnd)
                return MatchFailure{};

            if (myLiteral == *aContext.myAt)
                return Success<Iterator>{this, aContext.myAt, aContext.myAt + 1};

            return MatchFailure{};
        }

        template<class Iterator>
        Result<Iterator> SequenceMatch(MatchContext<Iterator>& aContext, const Result<Iterator>& aResult) const
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
                return Success<Iterator>{this, aContext.myBegin, aContext.myAt, aContext.mySubMatches};

            return mySubFragments[aContext.myIndex++]->BeginMatch(aContext.myAt);
        }

        template<class Iterator, class Sentinel>
        Result<Iterator> AlternativeMatch(MatchContext<Iterator>& aContext, const Result<Iterator>& aResult,
                                          Sentinel aEnd) const
        {
            if (aContext.myIndex == 0 && myLUTPortion > 0)
            {
                if (aContext.myBegin != aEnd)
                {
                    std::iter_value_t<Iterator> v = *aContext.myAt;

                    SmallIndex index = myLUT[(Literal)v];

                    if (index != NoIndex)
                    {
                        return Success<Iterator>{this,
                                                 aContext.myAt,
                                                 aContext.myAt + 1,
                                                 {Success<Iterator>{mySubFragments[index], aContext.myAt, aContext.myAt + 1}}};
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
                    return Success<Iterator>{
                        this, aResult.Success().myBegin, aResult.Success().myEnd, {aResult.Success()}};
                case MatchResultType::InProgress:
                default:
                    assert(false);
                    std::unreachable();
                    break;
            }

            if (aContext.myIndex == mySubFragments.size())
                return MatchFailure{};

            return mySubFragments[aContext.myIndex++]->BeginMatch(aContext.myBegin);
        }

        template<class Iterator>
        Result<Iterator> RepeatMatch(MatchContext<Iterator>& aContext, const Result<Iterator>& aResult) const
        {
            switch (aResult.GetType())
            {
                case MatchResultType::Failure:

                    if (aContext.myIndex > myCount.myMin)
                        return Success<Iterator>{this, aContext.myBegin, aContext.myAt, aContext.mySubMatches};

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
                return Success<Iterator>{this, aContext.myBegin, aContext.myAt, aContext.mySubMatches};

            aContext.myIndex++;

            return mySubFragments[0]->BeginMatch(aContext.myAt);
        }

    private:
        SmallIndex myLUT[1 << (sizeof(Literal) * CHAR_BIT)];
        Type myType;
        union
        {
            size_t myLUTPortion;    // type: Alternative
            Literal myLiteral;      // type: Literal
            RepeatCount myCount;    // type: Repeat
        };
        std::vector<const Fragment*> mySubFragments;
    };

}  // namespace pattern_matcher