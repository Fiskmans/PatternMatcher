#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "pattern_matcher/PatternMatcher.h"
#include "pattern_matcher/RepeatCount.h"

namespace pattern_matcher {

    namespace builder_parts {
        struct Repeat {
            std::string myBase;
            RepeatCount myCount;
        };
    }  // namespace builder_parts

    class PatternBuilder
    {
    public:
        class Builder
        {
        public:
            Builder();

            void operator=(std::string aLiteral);
            void operator=(builder_parts::Repeat aRepeat);
            Builder& operator&&(std::string aPart);
            Builder& operator||(std::string aPart);

            void NotOf(std::string aChars);
            void OneOf(std::string aChars);

            std::optional<pattern_matcher::Fragment> Bake(PatternMatcher<>& Patterns);

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

        Builder& Add(std::string aKey);

        PatternMatcher<std::string> Finalize();

    private:
        std::unordered_map<std::string, Builder> myParts;
    };
}  // namespace pattern_matcher