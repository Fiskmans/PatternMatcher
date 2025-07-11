#include "PatternBuilder.h"

#include <cassert>

PatternBuilder::Builder::Builder()
	: myMode(Mode::Unkown)
{
}

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

	myMode = Mode::Alternative;

	for (int c = std::numeric_limits<char>::min(); c <= std::numeric_limits<char>::max(); c++)
	{
		if (aChars.find(c) == std::string::npos)
		{
			myParts.push_back("char-" + std::to_string(c));
		}
	}
}

void PatternBuilder::Builder::OneOf(std::string aChars)
{
	assert(myMode == Mode::Unkown || myMode == Mode::Alternative);

	myMode = Mode::Alternative;

	for (char c : aChars)
	{
		myParts.push_back("char-" + std::to_string(c));
	}
}

std::unique_ptr<IPatternMatcherFragment<>> PatternBuilder::Builder::Bake(std::string aKey)
{
	switch (myMode)
	{
	case Mode::Unkown:
		assert(false);
		break;

	case Mode::Literal:
		assert(myParts.size() == 1);
		return std::make_unique<fragments::LiteralFragment<>>(aKey, myParts[0]);

	case Mode::Sequence:
		return std::make_unique<fragments::SequenceFragment<>>(aKey, myParts);

	case Mode::Alternative:
		return std::make_unique<fragments::AlternativeFragment<>>(aKey, myParts);

	case Mode::Repeat:
		return std::make_unique<fragments::RepeatFragment<>>(aKey, myParts[0], myCount);
	}

	std::unreachable();
}

PatternBuilder::Builder& PatternBuilder::Add(std::string aKey)
{
	return myParts[aKey];
}

PatternMatcher<std::string, std::string_view> PatternBuilder::Finalize()
{
	PatternMatcher<std::string, std::string_view> matcher;

	for (auto& [ key, part ] : myParts)
	{
		matcher.AddFragment(part.Bake(key));
	}
	
	assert(matcher.Resolve());

	return matcher;
}
