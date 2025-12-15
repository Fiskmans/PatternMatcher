#pragma once

#include <cstddef>
#include <ranges>

using Expect = std::expected<void, std::string>;

template<class Key, std::ranges::range LiteralType>
class PatternMatcherFragment;

template<typename T>
class Indexable {
public:
    static constexpr bool IsIndexable = false;
    static constexpr size_t Count     = 0;
};

template<>
class Indexable<char> {
public:
    static constexpr bool IsIndexable  = true;
    static constexpr std::size_t Count = 0x100;
    static std::size_t ConvertToIndex(char aValue) { return (std::size_t)aValue - std::numeric_limits<char>::min(); }
};

enum class MatchResultType
{
    Success,
    Failure,
    InProgress,
    None
};

struct MatchFailure {};
struct MatchNone {};

template<class Key, std::ranges::range LiteralType, std::ranges::range TokenRange>
struct MatchSuccess {
    const PatternMatcherFragment<Key, LiteralType>* myPattern;

    std::ranges::iterator_t<TokenRange> myBegin;
    std::ranges::iterator_t<TokenRange> myEnd;

    std::vector<MatchSuccess> mySubMatches;

    auto begin() { return myBegin; }
    auto end() { return myEnd; }

    template<RangeComparable<TokenRange> Range>
    bool operator==(Range&& aRange)
    {
        auto lAt  = myBegin;
        auto lEnd = myEnd;

        auto rAt  = std::ranges::begin(aRange);
        auto rEnd = std::ranges::end(aRange);

        while (lAt != lEnd && rAt != rEnd)
        {
            if (!(*lAt == *rAt)) return false;

            ++lAt;
            ++rAt;
        }

        return lAt == lEnd && rAt == rEnd;
    }
};

template<class Key, std::ranges::range LiteralType, std::ranges::range TokenRange>
struct MatchContext {
    const PatternMatcherFragment<Key, LiteralType>* myPattern;

    std::ranges::iterator_t<TokenRange> myBegin;
    std::ranges::iterator_t<TokenRange> myAt;
    std::ranges::sentinel_t<TokenRange> myEnd;
    int myIndex;

    std::vector<MatchSuccess<Key, LiteralType, TokenRange>> mySubMatches;

    auto begin() { return myBegin; }
    auto end() { return myEnd; }

    template<RangeComparable<TokenRange> Range>
    bool operator==(Range&& aRange)
    {
        auto lAt  = myBegin;
        auto lEnd = myEnd;

        auto rAt  = std::ranges::begin(aRange);
        auto rEnd = std::ranges::end(aRange);

        while (lAt != lEnd && rAt != rEnd)
        {
            if (!(*lAt == *rAt)) return false;

            ++lAt;
            ++rAt;
        }

        return lAt == lEnd && rAt == rEnd;
    }
};

template<class Key, std::ranges::range LiteralType, std::ranges::range TokenRange>
struct Result {
    using SuccessType = MatchSuccess<Key, LiteralType, TokenRange>;
    using ContextType = MatchContext<Key, LiteralType, TokenRange>;

    Result(SuccessType aResult) : myResult(aResult) {}
    Result(MatchFailure aResult) : myResult(aResult) {}
    Result(ContextType aResult) : myResult(aResult) {}
    Result() : myResult(MatchNone{}) {}

    MatchResultType GetType() const
    {
        switch (myResult.index())
        {
            case 0:
                return MatchResultType::Success;
            case 1:
                return MatchResultType::Failure;
            case 2:
                return MatchResultType::InProgress;
            case 3:
                return MatchResultType::None;
        }

        std::unreachable();
    }

    const SuccessType& Success() const { return std::get<SuccessType>(myResult); }

    const ContextType& Context() const { return std::get<ContextType>(myResult); }

private:
    std::variant<SuccessType, MatchFailure, ContextType, MatchNone> myResult;
};

// Simple comparison helpers
inline bool operator==(MatchSuccess<std::string, std::string, std::string_view> aMatch, const char* aString)
{
    return aMatch == std::string_view(aString);
}

inline bool operator==(MatchContext<std::string, std::string, std::string_view> aContext, const char* aString)
{
    return aContext == std::string_view(aString);
}