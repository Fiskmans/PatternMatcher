#pragma once

#include "pattern_matcher/PatternMatcher.h"

namespace pattern_matcher
{

    class BNFPlus
    {
        enum class BNFFrag
        {
            Document,
            Space,

            Key,
            Key_Char,

            Declaration,
            Value,
            Value_Part,

            Optional_Multiplier,
            Multiplier,
            Multiplier_Plus,
            Multiplier_Star,
            Multiplier_Question
        };

        static PatternMatcher<BNFFrag>& BNFParser();

        static PatternMatcher<std::string> Parse(const std::string& aText);
        static PatternMatcher<std::string> Parse(MatchSuccess<std::string> aSuccess);
    };

}  // namespace pattern_matcher