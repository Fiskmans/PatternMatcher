
#include "catch2/catch_all.hpp"

#include "pattern_matcher/PatternBuilder.h"

#include <fstream>
#include <filesystem>

namespace {

// This class shows how to implement a simple generator for Catch tests
class FilesGenerator final : public Catch::Generators::IGenerator<std::filesystem::path>
{
public:
	FilesGenerator(std::filesystem::path aRoot)
		: myIterator(aRoot)
	{
		assert(myIterator != std::filesystem::directory_iterator{});
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

std::filesystem::path const& FilesGenerator::get() const
{
	return myIterator->path();
}

Catch::Generators::GeneratorWrapper<std::filesystem::path> Files(std::filesystem::path aRoot)
{
	return Catch::Generators::GeneratorWrapper<std::filesystem::path>(
		new FilesGenerator(aRoot)
	);
}

} // end anonymous namespaces


TEST_CASE("pattern_matcher::integration::json", "")
{
	PatternBuilder builder;

	builder.Add("")					= "";
	builder.Add("space")			= " ";
	builder.Add("new-line")			= "\n";
	builder.Add("carriage-return")	= "\r";
	builder.Add("horizontal-tab")	= "\t";
	builder.Add("quote")			= "\"";

	for (size_t i = 0; i < std::numeric_limits<char>::max(); i++)
	{
		if (isprint(i) && !isspace(i))
		{
			std::string s(1, i);
			builder.Add(s) = s;
		}
	}

	builder.Add("whitespace-char") || "space" || "new-line" || "carriage-return" || "horizontal-tab";
	builder.Add("whitespace")		= { "whitespace-char", { 0, RepeatCount::Unbounded} };

	builder.Add("true") = "true";
	builder.Add("false") = "false";
	builder.Add("null") = "null";


	builder.Add("digit-nonzero") || "1" || "2" || "3" || "4" || "5" || "6" || "7" || "8" || "9";
	builder.Add("digit") || "0" || "digit-nonzero";
	builder.Add("digits") = { "digit", { 0, RepeatCount::Unbounded } };

	builder.Add("hexadecimal-digit") || "digit" || "a" || "b" || "c" || "d" || "e" || "f" || "A" || "B" || "C" || "D" || "E" || "F";

	builder.Add("minus-optional") = { "-", {0, 1} };

	builder.Add("number-at-least-one-digit") = { "digit", {1, RepeatCount::Unbounded } };
	builder.Add("number-decimal-nonzero") && "digit-nonzero" && "digits";
	builder.Add("number-decimal") || "0" || "number-decimal-nonzero";

	builder.Add("number-fraction") && "." && "number-at-least-one-digit";
	builder.Add("number-fraction-optional") = { "number-fraction", {0,1} };

	builder.Add("number-exponent-e") || "e" || "E";
	builder.Add("number-exponent-sign") || "-" || "+" || "";

	builder.Add("number-exponent") && "number-exponent-e" && "number-exponent-sign" && "number-at-least-one-digit";
	builder.Add("number-exponent-optional") = { "number-exponent", { 0, 1 } };

	builder.Add("number") && "minus-optional" && "number-decimal" && "number-fraction-optional" && "number-exponent-optional";

	builder.Add("string-char-non-escaped").NotOf("\\\"");


	builder.Add("string-unicode-digits") = { "hexadecimal-digit", { 4, 4 } };
	builder.Add("string-unicode-escape") && "u" && "string-unicode-digits";
	builder.Add("string-char-escape-sequence") || "\"" || "\\" || "/" || "b" || "f" || "n" || "r" || "t" || "string-unicode-escape";

	builder.Add("string-char-escaped") && "\\" && "string-char-escape-sequence";

	builder.Add("string-char")  || "string-char-escaped" || "string-char-non-escaped";
	builder.Add("string-content") = { "string-char", { 0, RepeatCount::Unbounded } };
	builder.Add("string") && "quote" && "string-content" && "quote";

	builder.Add("value-raw") || "string" || "number" || "object" || "array" || "true" || "false" || "null";
	builder.Add("value") && "whitespace" && "value-raw" && "whitespace";

	builder.Add("array-cont") && "," && "value";
	builder.Add("array-continuations") = { "array-cont", { 0, RepeatCount::Unbounded } };
	builder.Add("array-items") && "value" && "array-continuations";
	builder.Add("array") && "[" && "whitespace" && "array-items" && "whitespace" && "]";

	builder.Add("object-item") && "whitespace" && "string" && ":" && "value";

	builder.Add("object-continuation") && "," && "object-item";
	builder.Add("object-continuations") = { "object-continuations", { 0, RepeatCount::Unbounded } };

	builder.Add("object-items") && "object-item" && "object-continuations";

	builder.Add("object-content") || "whitespace" || "object-items";
	builder.Add("object") && "{" && "object-content" && "}";

	PatternMatcher matcher = builder.Finalize();

	std::filesystem::path file = GENERATE(Files(CATCH_JSON_TEST_CASES_PATH));

	std::ifstream in(file);

	std::string all;
	std::string line;
	while (std::getline(in, line))
		all += line;

	char type = file.filename().c_str()[0];

	CAPTURE(file.filename());

	switch (type)
	{
	case 'i':
		break;
	case 'n':
		{
			std::optional<PatternMatch> match = matcher.Match("value", all);
			REQUIRE((!match || match->myRange != all));
		}
		break;
	case 'y':
		REQUIRE(matcher.Match("value", all));
		break;
	}
}