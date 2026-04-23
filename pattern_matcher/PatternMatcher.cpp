
#include "pattern_matcher/PatternMatcher.h"

namespace pattern_matcher
{
    PatternMatcherLiterals::PatternMatcherLiterals() 
    {
        for (size_t i = 0; i < std::numeric_limits<Fragment::Literal>::max(); i++) 
            ourLiterals[i] = i;
    }

    const Fragment* PatternMatcherLiterals::operator[](Fragment::Literal aIndex) const
    {
        return ourLiterals + aIndex; 
    }

    bool PatternMatcherLiterals::Contains(const Fragment* aFragment) const
    {
        return aFragment >= ourLiterals && aFragment < ourLiterals + size;
    }

    Fragment::Literal PatternMatcherLiterals::ValueOf(const Fragment* aFragment) const
    {
        assert(Contains(aFragment));
        return static_cast<Fragment::Literal>(aFragment - ourLiterals);
    }
}
