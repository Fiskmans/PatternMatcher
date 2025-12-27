#include "pattern_matcher/BNFPlus.h"

namespace pattern_matcher
{

    PatternMatcher<BNFPlus::BNFFrag>& BNFPlus::BNFParser()
    {
        static PatternMatcher<BNFPlus::BNFFrag> matcher;

        if (matcher[BNFFrag::Document])
            return matcher;

        matcher.EmplaceFragment(BNFFrag::Document);
        matcher.EmplaceFragment(BNFFrag::Space);
        matcher.EmplaceFragment(BNFFrag::Key);
        matcher.EmplaceFragment(BNFFrag::Key_Char);
        matcher.EmplaceFragment(BNFFrag::Declaration);
        matcher.EmplaceFragment(BNFFrag::Value);
        matcher.EmplaceFragment(BNFFrag::Value_Part);
        matcher.EmplaceFragment(BNFFrag::Optional_Multiplier);
        matcher.EmplaceFragment(BNFFrag::Multiplier);
        matcher.EmplaceFragment(BNFFrag::Multiplier_Plus);
        matcher.EmplaceFragment(BNFFrag::Multiplier_Star);
        matcher.EmplaceFragment(BNFFrag::Multiplier_Question);

        std::size_t any = std::numeric_limits<std::size_t>::max();

        *matcher[BNFFrag::Document] = Fragment(matcher[BNFFrag::Declaration], {0, any});

        *matcher[BNFFrag::Declaration] =
            Fragment(Fragment::Type::Sequence, {matcher[BNFFrag::Key], matcher[BNFFrag::Value]});

        *matcher[BNFFrag::Key] = Fragment(matcher[BNFFrag::Key_Char], {1, any});

        return matcher;
    }

    PatternMatcher<std::string> BNFPlus::Parse(const std::string& aText)
    {
        PatternMatcher<std::string> matcher;

        return matcher;
    }

}  // namespace pattern_matcher