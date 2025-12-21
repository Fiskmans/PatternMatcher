#include "pattern_matcher/BNFPlus.h"

PatternMatcher<BNFPlus::BNFFrag>& BNFPlus::BNFParser()
{
    static PatternMatcher<BNFPlus::BNFFrag> matcher;

    if (matcher[BNFFrag::Document])
        return matcher;

    matcher.AllocateFragment(BNFFrag::Document);
    matcher.AllocateFragment(BNFFrag::Space);
    matcher.AllocateFragment(BNFFrag::Key);
    matcher.AllocateFragment(BNFFrag::Key_Char);
    matcher.AllocateFragment(BNFFrag::Declaration);
    matcher.AllocateFragment(BNFFrag::Value);
    matcher.AllocateFragment(BNFFrag::Value_Part);
    matcher.AllocateFragment(BNFFrag::Optional_Multiplier);
    matcher.AllocateFragment(BNFFrag::Multiplier);
    matcher.AllocateFragment(BNFFrag::Multiplier_Plus);
    matcher.AllocateFragment(BNFFrag::Multiplier_Star);
    matcher.AllocateFragment(BNFFrag::Multiplier_Question);

    std::size_t any = std::numeric_limits<std::size_t>::max();

    *matcher[BNFFrag::Document] = PatternMatcherFragment(matcher[BNFFrag::Declaration], {0, any});

    *matcher[BNFFrag::Declaration] =
        PatternMatcherFragment(PatternMatcherFragmentType::Sequence, {matcher[BNFFrag::Key], matcher[BNFFrag::Value]});

    *matcher[BNFFrag::Key] = PatternMatcherFragment(matcher[BNFFrag::Key_Char], {1, any});

    return matcher;
}

PatternMatcher<std::string> BNFPlus::Parse(const std::string& aText)
{
    PatternMatcher<std::string> matcher;

    return matcher;
}