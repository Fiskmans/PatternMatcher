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

    void PatternBuilder::Builder::operator=(Repeat aRepeat)
    {
        assert(myMode == Mode::Unkown);

        myMode = Mode::Repeat;

        myParts.push_back(aRepeat.myBase);
        myCount = aRepeat.myCount;
    }

    PatternBuilder::Builder& PatternBuilder::Builder::operator&&(std::string aPart)
    {
        return *this && std::vector<std::string>{aPart};
    }

    PatternBuilder::Builder& PatternBuilder::Builder::operator&&(const std::vector<std::string>& aParts)
    {
        assert(myMode == Mode::Unkown || myMode == Mode::Sequence);

        myMode = Mode::Sequence;
        myParts.insert(std::end(myParts), std::begin(aParts), std::end(aParts));

        return *this;
    }

    PatternBuilder::Builder& PatternBuilder::Builder::operator||(std::string aPart)
    {
        return *this || std::vector<std::string>{aPart};
    }

    PatternBuilder::Builder& PatternBuilder::Builder::operator||(const std::vector<std::string>& aParts)
    {
        assert(myMode == Mode::Unkown || myMode == Mode::Alternative);

        myMode = Mode::Alternative;
        myParts.insert(std::end(myParts), std::begin(aParts), std::end(aParts));

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
                    if (key.length() == 1)
                    {
                        fragment = aMatcher[key[0]];
                    }
                    else
                    {
                        fprintf(stderr, "Missing fragment with key %s\n", key.c_str());
                        return {};
                    }
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

    bool PatternBuilder::HasKey(std::string aKey)
    {
        for (auto [key, builder] : myParts)
            if (key == aKey)
                return true;

        return false;
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

    std::string PatternBuilder::ToString(Success<std::ranges::iterator_t<std::string>>& aSuccess)
    {
        return std::string(std::ranges::begin(aSuccess), std::ranges::end(aSuccess));
    }

    PatternMatcher<std::string> PatternBuilder::FromBNF(std::string aBNF)
    {
        PatternBuilder out;
        PatternMatcher<std::string> metaParser = Builtin::BNF();

        using Success = Success<std::ranges::iterator_t<std::string>>;

        std::optional<Success> parsed = metaParser.Match(metaParser["doc"], aBNF);

        if (!parsed)
            return out.Finalize();

        for (Success& declaration : parsed->SearchFor(metaParser["decl"]))
        {
            std::string key = ToString(declaration[1]);

            std::vector<std::vector<std::string>> options;

            for (Success& option : declaration.SearchFor(metaParser["value"]))
            {
                std::vector<std::string> sequence;

                for (Success& val : option.SearchFor(metaParser["value-part"]))
                {
                    std::string fragment = ToString(val[1]);
                    Success* modifier    = val.Find(metaParser["repeat-char"]);

                    if (modifier)
                    {
                        Builder::Repeat repeat;
                        repeat.myBase = fragment;

                        switch (*modifier->begin())
                        {
                            case '*':
                                fragment += "-any";
                                repeat.myCount = {0, RepeatCount::Unbounded};
                                break;
                            case '+':
                                fragment += "-repeated";
                                repeat.myCount = {1, RepeatCount::Unbounded};
                                break;
                            case '?':
                                fragment += "-optional";
                                repeat.myCount = {0, 1};
                                break;
                        }
                        if (!out.HasKey(fragment))
                            out[fragment] = repeat;
                    }

                    sequence.push_back(fragment);
                }

                options.push_back(sequence);
            }

            if (options.size() == 1)
            {
                out[key] && options[0];
            }
            else
            {
                std::vector<std::string> subKeys;
                for (size_t i = 0; i < options.size(); i++)
                {
                    std::string subKey = key + "-" + std::to_string(i);
                    out[subKey] && options[i];
                    subKeys.push_back(subKey);
                }

                out[key] || subKeys;
            }
        }

        return out.Finalize();
    }

    PatternMatcher<std::string> PatternBuilder::Builtin::BNF()
    {
        PatternBuilder builder;

        builder["identifier-char"].OneOf("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-");
        builder["repeat-char"].OneOf("+*?");
        builder["comment-initializer"].OneOf("#");
        builder["colon"].OneOf(":");
        builder["repeat"] = {"repeat-char", {0, 1}};
        builder["new-line"].OneOf("\n");
        builder["new-line-optional"] = {"new-line", {0, 1}};

        builder["whitespace-char"].OneOf(" \t");
        builder["whitespace"]          = {"whitespace-char", {1, RepeatCount::Unbounded}};
        builder["whitespace-optional"] = {"whitespace-char", {0, RepeatCount::Unbounded}};

        builder["identifier"] = {"identifier-char", {1, RepeatCount::Unbounded}};

        builder["value-part"] && "whitespace" && "identifier" && "repeat";
        builder["value"] = {"value-part", {1, RepeatCount::Unbounded}};

        builder["values-single"] && "new-line" && "value";
        builder["values"] = {"values-single", {1, RepeatCount::Unbounded}};

        builder["decl"] && "whitespace-optional" && "identifier" && "whitespace-optional" && "colon" && "values"
            && "new-line-optional";

        builder["comment-char"].NotOf("\n");
        builder["comment-content"] = {"comment-char", {0, RepeatCount::Unbounded}};
        builder["comment"] && "whitespace-optional" && "comment-initializer" && "comment-content"
            && "new-line-optional";

        builder["empty-line"] && "whitespace-optional" && "new-line";

        builder["line"] || "decl" || "comment" || "empty-line";
        builder["doc"] = {"line", {0, RepeatCount::Unbounded}};

        return builder.Finalize();
    }

}  // namespace pattern_matcher