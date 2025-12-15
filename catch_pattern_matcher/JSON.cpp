
#include <filesystem>
#include <fstream>

#include "catch2/catch_all.hpp"
#include "pattern_matcher/PatternBuilder.h"

namespace {

// This class shows how to implement a simple generator for Catch tests
class FilesGenerator final : public Catch::Generators::IGenerator<std::filesystem::path> {
public:
    FilesGenerator(std::filesystem::path aRoot, std::string aStartAt = "") : myIterator(aRoot)
    {
        assert(myIterator != std::filesystem::directory_iterator{});

        if (!aStartAt.empty())
        {
            while (myIterator->path().filename() != aStartAt)
            {
                assert(next());
            }
        }
    }

    std::filesystem::path const& get() const override;
    bool next() override
    {
        myIterator++;
        return myIterator != std::filesystem::directory_iterator{};
    }

private:
    std::filesystem::directory_iterator myIterator;
};

std::filesystem::path const& FilesGenerator::get() const { return myIterator->path(); }

Catch::Generators::GeneratorWrapper<std::filesystem::path> Files(std::filesystem::path aRoot, std::string aStartAt = "")
{
    return Catch::Generators::GeneratorWrapper<std::filesystem::path>(new FilesGenerator(aRoot, aStartAt));
}

}  // namespace

TEST_CASE("pattern_matcher::integration::json", "")
{
    PatternBuilder builder;

    RepeatCount optionally = {0, 1};
    RepeatCount anyAmount  = {0, RepeatCount::Unbounded};
    RepeatCount repeated   = {1, RepeatCount::Unbounded};

    builder.Add("")                = "";
    builder.Add("space")           = " ";
    builder.Add("new-line")        = "\n";
    builder.Add("carriage-return") = "\r";
    builder.Add("horizontal-tab")  = "\t";
    builder.Add("quote")           = "\"";

    auto CharName = [](char c) { return "char-" + std::to_string(c); };

    for (int i = std::numeric_limits<char>::min(); i <= std::numeric_limits<char>::max(); i++)
    {
        builder.Add(CharName(i)) = std::string(1, i);
    }

    builder.Add("whitespace-char") || "space" || "new-line" || "carriage-return" || "horizontal-tab";
    builder.Add("whitespace") = {"whitespace-char", anyAmount};

    builder.Add("true")  = "true";
    builder.Add("false") = "false";
    builder.Add("null")  = "null";

    builder.Add("digit-nonzero").OneOf("123456789");
    builder.Add("digit") || CharName('0') || "digit-nonzero";
    builder.Add("digits") = {"digit", anyAmount};

    builder.Add("hexadecimal-digit").OneOf("0123456789aAbBcCdDeEfF");

    builder.Add("minus-optional") = {CharName('-'), optionally};

    builder.Add("number-at-least-one-digit") = {"digit", repeated};
    builder.Add("number-decimal-nonzero") && "digit-nonzero" && "digits";
    builder.Add("number-decimal") || CharName('0') || "number-decimal-nonzero";

    builder.Add("number-fraction") && CharName('.') && "number-at-least-one-digit";
    builder.Add("number-fraction-optional") = {"number-fraction", optionally};

    builder.Add("number-exponent-e") || CharName('e') || CharName('E');
    builder.Add("number-exponent-sign") || CharName('-') || CharName('+') || "";

    builder.Add("number-exponent") && "number-exponent-e" && "number-exponent-sign" && "number-at-least-one-digit";
    builder.Add("number-exponent-optional") = {"number-exponent", optionally};

    builder.Add("number") && "minus-optional" && "number-decimal" && "number-fraction-optional" &&
        "number-exponent-optional";

    builder.Add("string-char-non-escaped").NotOf(std::string("\\\"\n\b\t") + '\0');

    builder.Add("string-unicode-digits") = {"hexadecimal-digit", {4, 4}};
    builder.Add("string-unicode-escape") && CharName('u') && "string-unicode-digits";
    builder.Add("string-escape-char").OneOf("\"\\/bfnrt");
    builder.Add("string-char-escape-sequence") || "string-escape-char" || "string-unicode-escape";

    builder.Add("string-char-escaped") && CharName('\\') && "string-char-escape-sequence";

    builder.Add("string-char") || "string-char-escaped" || "string-char-non-escaped" || "space";
    builder.Add("string-content") = {"string-char", anyAmount};
    builder.Add("string") && "quote" && "string-content" && "quote";

    builder.Add("value-raw") || "array" || "object" || "true" || "false" || "null" || "string" || "number";
    builder.Add("value") && "whitespace" && "value-raw" && "whitespace";

    builder.Add("array-cont") && "whitespace" && CharName(',') && "whitespace" && "value-raw";
    builder.Add("array-continuations") = {"array-cont", anyAmount};
    builder.Add("array-items") && "value-raw" && "array-continuations" && "whitespace";
    builder.Add("array-content") || "array-items" || "";
    builder.Add("array") && CharName('[') && "whitespace" && "array-content" && CharName(']');

    builder.Add("object-item") && "whitespace" && "string" && CharName(':') && "value";

    builder.Add("object-continuation") && CharName(',') && "object-item";
    builder.Add("object-continuations") = {"object-continuation", anyAmount};

    builder.Add("object-items") && "object-item" && "object-continuations";

    builder.Add("object-content") || "object-items" || "whitespace";
    builder.Add("object") && CharName('{') && "object-content" && CharName('}');

    PatternMatcher matcher = builder.Finalize();

    std::filesystem::path file = GENERATE(Files(CATCH_JSON_TEST_CASES_PATH));

    std::ifstream in(file);

    std::string all;
    std::string line;
    while (std::getline(in, line))
    {
        if (!all.empty()) all += "\n";
        all += line;
    }

    char type = file.filename().c_str()[0];

    CAPTURE(file.filename());

    try
    {
        switch (type)
        {
            case 'i':
                break;
            case 'n': {
                auto match = matcher.Match("value", all);
                if (match) REQUIRE(!(*match == all));
            }
            break;
            case 'y':
                auto result = matcher.Match("value", all);
                REQUIRE(result);
                break;
        }
    }
    catch (const std::exception& e)
    {
        REQUIRE_FALSE(e.what());
    }
}