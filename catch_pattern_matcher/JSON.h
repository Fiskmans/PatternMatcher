#pragma once

#include <string>

#include "pattern_matcher/PatternBuilder.h"

inline pattern_matcher::PatternBuilder MakeJsonParser()
{
    pattern_matcher::PatternBuilder builder;

    pattern_matcher::RepeatCount optionally = {0, 1};
    pattern_matcher::RepeatCount anyAmount  = {0, pattern_matcher::RepeatCount::Unbounded};
    pattern_matcher::RepeatCount repeated   = {1, pattern_matcher::RepeatCount::Unbounded};

    builder["quote"] = "\"";

    auto CharName = [](char c) { return "char-" + std::to_string(c); };

    for (int i = std::numeric_limits<char>::min(); i <= std::numeric_limits<char>::max(); i++)
    {
        builder[CharName(i)] = std::string(1, i);
    }

    builder["whitespace-char"].OneOf(" \n\r\t");
    builder["whitespace"] = {"whitespace-char", anyAmount};

    builder["true"]  = "true";
    builder["false"] = "false";
    builder["null"]  = "null";

    builder["digit-nonzero"].OneOf("123456789");
    builder["digit"] || CharName('0') || "digit-nonzero";
    builder["digits"] = {"digit", anyAmount};

    builder["hexadecimal-digit"].OneOf("0123456789aAbBcCdDeEfF");

    builder["minus-optional"] = {CharName('-'), optionally};

    builder["number-at-least-one-digit"] = {"digit", repeated};
    builder["number-decimal-nonzero"] && "digit-nonzero" && "digits";
    builder["number-decimal"] || CharName('0') || "number-decimal-nonzero";

    builder["number-fraction"] && CharName('.') && "number-at-least-one-digit";
    builder["number-fraction-optional"] = {"number-fraction", optionally};

    builder["number-exponent-e"].OneOf("eE");
    builder["number-exponent-sign"].OneOf("+-");
    builder["number-exponent-sign-optional"] = {"number-exponent-sign", optionally};

    builder["number-exponent"] && "number-exponent-e" && "number-exponent-sign-optional" && "number-at-least-one-digit";
    builder["number-exponent-optional"] = {"number-exponent", optionally};

    builder["number"] && "minus-optional" && "number-decimal" && "number-fraction-optional"
        && "number-exponent-optional";

    builder["string-char-non-escaped"].NotOf(std::string("\\\"\n\b\t") + '\0');

    builder["string-unicode-digits"] = {"hexadecimal-digit", {4, 4}};
    builder["string-unicode-escape"] && CharName('u') && "string-unicode-digits";
    builder["string-escape-char"].OneOf("\"\\/bfnrt");
    builder["string-char-escape-sequence"] || "string-escape-char" || "string-unicode-escape";

    builder["string-char-escaped"] && CharName('\\') && "string-char-escape-sequence";

    builder["string-char"] || "string-char-escaped" || "string-char-non-escaped";
    builder["string-content"] = {"string-char", anyAmount};
    builder["string"] && "quote" && "string-content" && "quote";

    builder["value-raw"] || "array" || "object" || "true" || "false" || "null" || "string" || "number";
    builder["value"] && "whitespace" && "value-raw" && "whitespace";

    builder["array-cont"] && "whitespace" && CharName(',') && "whitespace" && "value-raw";
    builder["array-continuations"] = {"array-cont", anyAmount};
    builder["array-items"] && "value-raw" && "array-continuations" && "whitespace";
    builder["array-content"] = {"array-items", optionally};
    builder["array"] && CharName('[') && "whitespace" && "array-content" && CharName(']');

    builder["object-item"] && "whitespace" && "string" && CharName(':') && "value";

    builder["object-continuation"] && CharName(',') && "object-item";
    builder["object-continuations"] = {"object-continuation", anyAmount};

    builder["object-items"] && "object-item" && "object-continuations";

    builder["object-content"] || "object-items" || "whitespace";
    builder["object"] && CharName('{') && "object-content" && CharName('}');

    return builder;
}