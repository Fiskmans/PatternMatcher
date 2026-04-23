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
        std::vector<const Fragment*> fragments;

        if (!IsPrimary())
        {
            for (std::string key : myParts)
            {
                const Fragment* fragment = aMatcher[key];
                if (!fragment)
                {
                    if (key.length() == 1)
                    {
                        fragment = aMatcher[key[0]];
                    }
                    else
                    {
                        fprintf(stderr, "Missing fragment with key: %s\n", key.c_str());
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

        for (auto& [key, fragment] : matcher.Fragments())
        {
            if (CheckForRecursion(&fragment, &fragment))
                fprintf(stderr, "Recursion found in %s", key.c_str());
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

        out["whitespace-char"].OneOf(" \r\n\t\b\v");
        out["whitespace-optional"] = {"whitespace-char", {0, RepeatCount::Unbounded}};

        for (Success& declaration : parsed->SearchFor(metaParser["decl"]))
        {
            std::string key = ToString(*declaration.Find(metaParser["identifier"]));

            std::vector<std::vector<std::string>> options;

            for (Success& option : declaration.SearchFor(metaParser["value"]))
            {
                std::vector<std::string> sequence;

                for (Success& val : option.SearchFor(metaParser["value-part"]))
                {
                    Success* subFragmentName = val.Find(metaParser["identifier"]);
                    Success* subLiteral = val.Find(metaParser["literal-content"]);
                    std::string fragment;
                    if (subFragmentName)
                    {
                        fragment = ToString(*subFragmentName);
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
                    }
                    else if (subLiteral)
                    {
                        std::string literalString = ToString(*subLiteral);
                        fragment   = "literal-" + literalString;

                        if (!out.HasKey(fragment))
                            out[fragment] = literalString;
                    }

                    if (sequence.size() > 0)
                        if (!val.Find(metaParser["identifier-pipe"]))
                            sequence.push_back("whitespace-optional");

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

                std::reverse(std::begin(subKeys), std::end(subKeys));

                out[key] || subKeys;
            }
        }

        std::string::iterator::difference_type left = std::end(aBNF) - parsed->myEnd;

        if (left > 0)
        {
            std::string_view data(parsed->myEnd, std::end(aBNF)); 
            fprintf(stderr, "%u bytes left at the end of bnf\n==== Skipped section ====\n%.*s\n==== End of skipped section ====\n", static_cast<unsigned int>(left), static_cast<int>(data.size()), &data.at(0));
        }

        return out.Finalize();
    }

    std::vector<const Fragment*> NextSteps(const Fragment* aNode) 
    {
        switch (aNode->GetType())
        {
            case Fragment::Type::Alternative:
                return aNode->SubFragments();
            case Fragment::Type::Sequence:
            case Fragment::Type::Repeat:
                if (aNode->SubFragments().size() == 0)
                    return {};
                return {aNode->SubFragments()[0]};
        }
        return {};
    }

    bool PatternBuilder::CheckForRecursion(const Fragment* aTortoise, const Fragment* aHare) 
    {
        std::vector<const Fragment*> hare1 = NextSteps(aHare);
        std::vector<const Fragment*> hare2;

        if (std::find(std::begin(hare1), std::end(hare1), aTortoise) != std::end(hare1))
            return true;

        for (const Fragment* n : hare1)
            for (const Fragment* n2 : NextSteps(n))
                if (std::find(std::begin(hare2), std::end(hare2), n2) != std::end(hare2))
                    hare2.push_back(n2);

        if (std::find(std::begin(hare2), std::end(hare2), aTortoise) != std::end(hare2))
            return true;

        for (const Fragment* t : NextSteps(aTortoise))
            for (const Fragment* h : hare2)
                if (CheckForRecursion(t, h))
                    return true;

        return false;
    }

    PatternMatcher<std::string> PatternBuilder::Builtin::BNF()
    {
        PatternBuilder builder;

        builder["identifier-char"].OneOf("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-");
        builder["literal-char"].NotOf("\" \t\n\r");
        builder["repeat-char"].OneOf("+*?");
        builder["comment-initializer"] = "#";
        builder["colon"]               = ":";
        builder["pipe"]                = "|";
        builder["quote"]               = "\"";
        builder["repeat"]              = {"repeat-char", {0, 1}};
        builder["new-line-unix"]       = "\n";
        builder["new-line-win"]        = "\r\n";
        builder["new-line"] || "new-line-win" || "new-line-unix";
        builder["new-line-optional"] = {"new-line", {0, 1}};

        builder["whitespace-char"].OneOf(" \t");
        builder["whitespace"]          = {"whitespace-char", {1, RepeatCount::Unbounded}};
        builder["whitespace-optional"] = {"whitespace-char", {0, RepeatCount::Unbounded}};

        builder["identifier"] = {"identifier-char", {1, RepeatCount::Unbounded}};
        builder["literal-content"] = {"literal-char", {1, RepeatCount::Unbounded}};
        builder["literal"] && "quote" && "literal-content" && "quote";

        builder["value-subpart"] || "literal" || "identifier";

        builder["identifier-pipe"] && "pipe" && "whitespace-optional";
        builder["identifier-pipe-optional"] = {"identifier-pipe", {0, 1}};

        builder["value-part"] && "identifier-pipe-optional" && "value-subpart" && "repeat" && "whitespace-optional";
        builder["value-part-repeated"] = {"value-part", {0, RepeatCount::Unbounded}};
        builder["value-part-first"] && "value-subpart" && "repeat" && "whitespace-optional";
        builder["value"] && "value-part-first" && "value-part-repeated";

        builder["values-single"] && "new-line" && "whitespace" && "value";
        builder["values"] = {"values-single", {1, RepeatCount::Unbounded}};

        builder["decl"] && "whitespace-optional" && "identifier" && "whitespace-optional" && "colon" && "whitespace-optional" && "values"
            && "new-line-optional";

        builder["comment-char"].NotOf("\n");
        builder["comment-content"] = {"comment-char", {0, RepeatCount::Unbounded}};
        builder["comment"] && "comment-initializer" && "comment-content"
            && "new-line-optional";

        builder["empty-line"] && "whitespace-optional" && "new-line";

        builder["line"] || "decl" || "comment" || "empty-line";
        builder["doc"] = {"line", {0, RepeatCount::Unbounded}};

        return builder.Finalize();
    }

}  // namespace pattern_matcher