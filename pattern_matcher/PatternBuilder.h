#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "pattern_matcher/PatternMatcher.h"
#include "pattern_matcher/RepeatCount.h"

namespace pattern_matcher
{
    class PatternBuilder
    {
    public:
        class Builder
        {
        public:
            struct Repeat
            {
                std::string myBase;
                RepeatCount myCount;
            };

            Builder();

            void operator=(std::string aLiteral);
            void operator=(Repeat aRepeat);
            Builder& operator&&(std::string aPart);
            Builder& operator&&(const std::vector<std::string>& aParts);
            Builder& operator||(std::string aPart);
            Builder& operator||(const std::vector<std::string>& aParts);

            void NotOf(std::string aChars);
            void OneOf(std::string aChars);

            std::optional<pattern_matcher::Fragment> Bake(PatternMatcher<>& Patterns);

            bool IsPrimary();

        private:
            enum class Mode
            {
                Unkown,
                Literal,
                Sequence,
                Alternative,
                Of,
                NotOf,
                Repeat
            };

            RepeatCount myCount;
            Mode myMode;
            std::vector<std::string> myParts;
        };

        bool HasKey(std::string aKey);
        Builder& operator[](std::string aKey);

        PatternMatcher<std::string> Finalize();

        static std::string ToString(Success<std::ranges::iterator_t<std::string>>& aSuccess);

        static PatternMatcher<std::string> FromBNF(std::string aBNF);
        struct Builtin
        {
            static PatternMatcher<std::string> BNF();
        };

    private:
        std::vector<std::pair<std::string, Builder>> myParts;
    };
}  // namespace pattern_matcher