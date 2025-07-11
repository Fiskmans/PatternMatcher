#pragma once

#include <string>
#include <memory>
#include <unordered_map>

#include "pattern_matcher/PatternMatcher.h"
#include "pattern_matcher/RepeatCount.h"

namespace builder_parts
{
	struct Repeat
	{
		std::string myBase;
		RepeatCount myCount;
	};
}

class PatternBuilder
{
public:

	class Builder
	{
	public:
		Builder() = default;

		void operator=(std::string aLiteral);
		void operator=(builder_parts::Repeat aRepeat);
		Builder& operator &&(std::string aPart);
		Builder& operator ||(std::string aPart);

		void NotOf(std::string aChars);
		void OneOf(std::string aChars);
		

		std::unique_ptr<IPatternMatcherFragment<>> Bake();

		enum class Mode
		{
			Unkown,
			Literal,
			Sequence,
			Alternative,
			Repeat
		};

		RepeatCount myCount;
		std::string myName;
		Mode myMode = Mode::Unkown;
		std::vector<std::string> myParts;
	};

	Builder& Add(std::string aKey);

	PatternMatcher<std::string, std::string_view> Finalize();

private:
	std::unordered_map<std::string, Builder> myParts;
};