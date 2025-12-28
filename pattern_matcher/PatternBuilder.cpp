#include "PatternBuilder.h"

#include <cassert>

namespace pattern_matcher
{
    PatternBuilder::Builder::Builder() : myMode(Mode::Unkown) {}

    void PatternBuilder::Builder::operator=(std::string aLiteral)
    {
        assert(myMode == Mode::Unkown);

        myMode = Mode::Literal;

        myParts.push_back(aLiteral);
    }

    void PatternBuilder::Builder::operator=(builder_parts::Repeat aRepeat)
    {
        assert(myMode == Mode::Unkown);

        myMode = Mode::Repeat;

        myParts.push_back(aRepeat.myBase);
        myCount = aRepeat.myCount;
    }

    PatternBuilder::Builder& PatternBuilder::Builder::operator&&(std::string aPart)
    {
        assert(myMode == Mode::Unkown || myMode == Mode::Sequence);

        myMode = Mode::Sequence;

        myParts.push_back(aPart);

        return *this;
    }

    PatternBuilder::Builder& PatternBuilder::Builder::operator||(std::string aPart)
    {
        assert(myMode == Mode::Unkown || myMode == Mode::Alternative);

        myMode = Mode::Alternative;

        myParts.push_back(aPart);

        return *this;
    }

    void PatternBuilder::Builder::NotOf(std::string aChars)
    {
        assert(myMode == Mode::Unkown || myMode == Mode::Alternative);

        myMode = Mode::NotOf;

        myParts.push_back(aChars);
    }

    void PatternBuilder::Builder::OneOf(std::string aChars)
    {
        assert(myMode == Mode::Unkown || myMode == Mode::Alternative);

        myMode = Mode::Of;

        myParts.push_back(aChars);
    }

    std::optional<Fragment> PatternBuilder::Builder::Bake(PatternMatcher<>& aMatcher)
    {
        std::vector<Fragment*> fragments;

        if (!IsPrimary())
        {
            for (std::string key : myParts)
            {
                Fragment* fragment = aMatcher[key];
                if (!fragment)
                {
                    fprintf(stderr, "Missing fragment with key %s\n", key.c_str());
                    return {};
                }

                fragments.push_back(fragment);
            }
        }

        switch (myMode)
        {
            case Mode::Unkown:
                assert(false);
                break;

            case Mode::Literal:
                return Fragment(Fragment::Type::Sequence, aMatcher.Of(myParts[0]));

            case Mode::Sequence:
                return Fragment(Fragment::Type::Sequence, fragments);

            case Mode::Alternative:
                return Fragment(Fragment::Type::Alternative, fragments);

            case Mode::Of:
                return Fragment(Fragment::Type::Alternative, aMatcher.Of(myParts[0]));
            case Mode::NotOf:
                return Fragment(Fragment::Type::Alternative, aMatcher.NotOf(myParts[0]));

            case Mode::Repeat:
                assert(myParts.size() == 1);
                return Fragment(fragments[0], myCount);
        }

        std::unreachable();
    }

    bool PatternBuilder::Builder::IsPrimary()
    {
        switch (myMode)
        {
            case Mode::Literal:
            case Mode::Of:
            case Mode::NotOf:
                return true;

            default:
                return false;
        }
    }

    PatternBuilder::Builder& PatternBuilder::operator[](std::string aKey)
    {
        myParts.push_back({aKey, {}});
        return myParts[myParts.size() - 1].second;
    }

    PatternMatcher<std::string> PatternBuilder::Finalize()
    {
        PatternMatcher<std::string> matcher;

        for (auto& [key, part] : myParts)
        {
            matcher.EmplaceFragment(key);

            if (part.IsPrimary())
            {
                std::optional<Fragment> fragment = part.Bake(matcher);
                if (!fragment)
                {
                    fprintf(stderr, "  In fragment %s\n", key.c_str());
                    continue;
                }

                *matcher[key] = *fragment;
            }
        }

        for (auto& [key, part] : myParts)
        {
            if (!part.IsPrimary())
            {
                std::optional<Fragment> fragment = part.Bake(matcher);
                if (!fragment)
                {
                    fprintf(stderr, "  In fragment %s\n", key.c_str());
                    continue;
                }

                *matcher[key] = *fragment;
            }
        }

        return matcher;
    }

}  // namespace pattern_matcher