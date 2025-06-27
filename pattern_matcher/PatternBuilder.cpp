#include "PatternBuilder.h"

#include <cassert>

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

std::unique_ptr<IPatternMatcherFragment> PatternBuilder::Builder::Bake()
{
	switch (myMode)
	{
	case Mode::Unkown:
		assert(false);
		break;

	case Mode::Literal:
		assert(myParts.size() == 1);
		return std::make_unique<fragments::LiteralFragment>(myName, myParts[0]);

	case Mode::Sequence:
		return std::make_unique<fragments::SequenceFragment>(myName, myParts);

	case Mode::Alternative:
		return std::make_unique<fragments::AlternativeFragment>(myName, myParts);

	case Mode::Repeat:
		return std::make_unique<fragments::RepeatFragment>(myName, myParts[0], myCount);
	}

	std::unreachable();
}

PatternBuilder::Builder& PatternBuilder::Add(std::string aKey)
{
	PatternBuilder::Builder& out = myParts[aKey];

	out.myName = aKey;

	return out;
}

PatternMatcher PatternBuilder::Finalize()
{
	PatternMatcher matcher;

	for (auto& [ name, part ] : myParts)
	{
		matcher.AddFragment(part.Bake());
	}
	
	assert(matcher.Resolve());

	return matcher;
}
