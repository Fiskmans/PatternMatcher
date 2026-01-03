#pragma once

#include <coroutine>
#include <cstddef>
#include <generator>
#include <ranges>

using Expect = std::expected<void, std::string>;

namespace pattern_matcher
{
    class Fragment;

    enum class MatchResultType
    {
        Success,
        Failure,
        InProgress,
        None
    };

    struct MatchFailure
    {
    };
    struct MatchNone
    {
    };

    template<class Iterator>
    struct Success
    {
        const Fragment* myFragment;

        Iterator myBegin;
        Iterator myEnd;

        std::vector<Success> mySubMatches;

        auto begin() { return myBegin; }
        auto end() { return myEnd; }

        Success& operator[](int aIndex) { return mySubMatches[aIndex]; }
        const Success& operator[](int aIndex) const { return mySubMatches[aIndex]; }

        Success* Find(const Fragment* aFragment)
        {
            for (Success& child : mySubMatches)
            {
                if (child.myFragment == aFragment)
                    return &child;

                Success* found = child.Find(aFragment);
                if (found)
                    return found;
            }

            return nullptr;
        }

        enum class SearchMode
        {
            // Search only among children
            TopLevelOnly,

            // Search among all grandchildren, but not children of objects that match
            Recursive,

            // Search among all grandchildren, regardless of them being a child of an already returned object or not
            All

            /*
            With the hierarchy:
            1. foo
            2.   foo
            3. bar
            4.   foo

            SearchFor("foo", TopLevelOnly); -> [1]
            SearchFor("foo", Recursive);    -> [1, 4]       (2 is skipped as its a child of 1)
            SearchFor("foo", All);          -> [1, 2, 4]
            */
        };

        std::generator<Success&> SearchFor(const Fragment* aFragment, SearchMode aMode = SearchMode::Recursive)
        {
            for (Success& child : mySubMatches)
            {
                if (child.myFragment == aFragment)
                {
                    co_yield child;

                    if (aMode != SearchMode::All)
                        continue;
                }

                if (aMode == SearchMode::TopLevelOnly)
                    continue;

                co_yield std::ranges::elements_of(child.SearchFor(aFragment, aMode));
            }
        }

        bool operator==(const char* aString) const { return this->operator== <std::string_view>(aString); }
        template<size_t Length>
        bool operator==(const char aString[Length]) const
        {
            return this->operator== <std::string_view>(aString);
        }

        template<RangeComparable<std::iter_value_t<Iterator>> Range>
        bool operator==(Range&& aRange) const
        {
            auto lAt  = myBegin;
            auto lEnd = myEnd;

            auto rAt  = std::ranges::begin(aRange);
            auto rEnd = std::ranges::end(aRange);

            while (lAt != lEnd && rAt != rEnd)
            {
                if (!(*lAt == *rAt))
                    return false;

                ++lAt;
                ++rAt;
            }

            return lAt == lEnd && rAt == rEnd;
        }
    };

    template<class Iterator>
    struct MatchContext
    {
        const pattern_matcher::Fragment* myFragment;

        Iterator myBegin;
        Iterator myAt;
        int myIndex;

        std::vector<Success<Iterator>> mySubMatches;
    };

    template<class Iterator>
    struct Result
    {
        using SuccessType = Success<Iterator>;
        using ContextType = MatchContext<Iterator>;

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
}  // namespace pattern_matcher