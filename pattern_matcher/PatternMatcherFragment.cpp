#include "PatternMatcherFragment.h"

#include <ranges>
#include <algorithm>
#include <format>
#include <cassert>

namespace fragments
{

	LiteralFragment::LiteralFragment(std::string aName, std::string aLiteral)
		: IPatternMatcherFragment(aName)
	{
		myLiteral = aLiteral;
	}

	Expect LiteralFragment::Resolve(const FragmentCollection& aFragments)
	{
		return {};
	}

	Result LiteralFragment::Resume(MatchContext& aContext, Result aResult)
	{
		CharIterator start = std::ranges::begin(aContext.myRange);

		if (std::ranges::starts_with(aContext.myRange, myLiteral))
			return Success(CharRange(start, start + std::ranges::size(myLiteral)));

		return MatchFailure{};
	}
	
	SequenceFragment::SequenceFragment(std::string aName, std::vector<std::string> aParts)
		: IPatternMatcherFragment(aName)
	{
		myParts = aParts;
	}

	Expect SequenceFragment::Resolve(const FragmentCollection& aFragments)
	{
		myResolvedParts.reserve(myParts.size());
		for (std::string& part : myParts)
		{
			auto resolved = aFragments.find(part);

			if (resolved == std::end(aFragments))
				return std::unexpected(std::format("Expected {} to be a valid fragment", part));

			myResolvedParts.push_back(resolved->second.get());
		}
		return {};
	}

	Result SequenceFragment::Resume(MatchContext& aContext, Result aResult)
	{
		assert(myResolvedParts.size() == myParts.size() && "Using unresolved fragment");

		std::vector<MatchSuccess> subMatches;

		switch (aResult.GetType())
		{
		case Result::Type::Failure:
			return MatchFailure{};
		case Result::Type::Success:
			aContext.mySubMatches.push_back(aResult.Success());
			aContext.myAt += std::ranges::size(aResult.Success().myRange);
			break;
		case Result::Type::None:
			break;
		case Result::Type::InProgress:
		default:
			assert(false);
			std::unreachable();
			break;
		}

		CharIterator start = std::ranges::begin(aContext.myRange);
		auto end = std::ranges::end(aContext.myRange);

		if (aContext.myIndex == myResolvedParts.size())
			return Success({ start, aContext.myAt }, aContext.mySubMatches);

		CharRange remaining{ aContext.myAt, end };

		return myResolvedParts[aContext.myIndex++]->Match(remaining);
	}

	AlternativeFragment::AlternativeFragment(std::string aName, std::vector<std::string> aParts)
		: IPatternMatcherFragment(aName)
	{
		myParts = aParts;
	}

	Expect AlternativeFragment::Resolve(const FragmentCollection& aFragments)
	{
		myResolvedParts.reserve(myParts.size());
		for (std::string& part : myParts)
		{
			auto resolved = aFragments.find(part);

			if (resolved == std::end(aFragments))
				return std::unexpected(std::format("Expected {} to be a valid fragment", part));

			myResolvedParts.push_back(resolved->second.get());
		}
		return {};
	}

	Result AlternativeFragment::Resume(MatchContext& aContext, Result aResult)
	{
		assert(myResolvedParts.size() == myParts.size() && "Using unresolved fragment");

		switch (aResult.GetType())
		{
		case Result::Type::Failure:
		case Result::Type::None:
			break;
		case Result::Type::Success:
			return Success(aResult.Success().myRange, { aResult.Success() });
		case Result::Type::InProgress:
		default:
			assert(false);
			std::unreachable();
			break;
		}

		if (aContext.myIndex == myResolvedParts.size())
			return MatchFailure{};

		return myResolvedParts[aContext.myIndex++]->Match(aContext.myRange);
	}

	RepeatFragment::RepeatFragment(std::string aName, std::string aBase, RepeatCount aCount)
		: IPatternMatcherFragment(aName)
		, myBase(aBase)
		, myCount(aCount)
		, myResolved(nullptr)
	{
	}

	Expect RepeatFragment::Resolve(const FragmentCollection& aFragments)
	{
		auto it = aFragments.find(myBase);
		if (it == aFragments.end())
			return std::unexpected(std::format("Expected {} to be a valid fragment", myBase));

		myResolved = it->second.get();

		return {};
	}

	Result RepeatFragment::Resume(MatchContext& aContext, Result aResult)
	{
		assert(myResolved && "Using unresolved fragment");

		switch (aResult.GetType())
		{
		case Result::Type::Failure:
			if (aContext.myIndex > myCount.myMin)
				return Success({ std::ranges::begin(aContext.myRange), aContext.myAt }, aContext.mySubMatches);
			return MatchFailure{};
		case Result::Type::Success:
			aContext.mySubMatches.push_back(aResult.Success());
			aContext.myAt += std::ranges::size(aResult.Success().myRange);
			break;
		case Result::Type::None:
			break;
		case Result::Type::InProgress:
		default:
			assert(false);
			std::unreachable();
			break;
		}

		if (aContext.myIndex == myCount.myMax)
			return Success({ std::ranges::begin(aContext.myRange), aContext.myAt }, aContext.mySubMatches);

		aContext.myIndex++;

		auto end = std::ranges::end(aContext.myRange);
		CharRange remaining(aContext.myAt, end);

		return myResolved->Match(remaining);
	}
}

IPatternMatcherFragment::IPatternMatcherFragment(std::string aName)
	: myName(aName)
{
}

MatchContext IPatternMatcherFragment::Match(const CharRange aRange)
{
	return InitializeContext(aRange);
}

MatchSuccess IPatternMatcherFragment::Success(const CharRange aRange)
{
	return { this, aRange, {} };
}

MatchSuccess IPatternMatcherFragment::Success(const CharRange aRange, std::vector<MatchSuccess> aSubMatches)
{
	return { this, aRange, aSubMatches };
}

MatchContext IPatternMatcherFragment::InitializeContext(const CharRange aRange)
{
	MatchContext ctx;

	ctx.myPattern = this;
	ctx.myAt = std::ranges::begin(aRange);
	ctx.myRange = aRange;
	ctx.myIndex = 0;

	return ctx;
}

Result::Type Result::GetType()
{
	switch (myResult.index())
	{
	case 0:
		return Type::Success;
	case 1:
		return Type::Failure;
	case 2:
		return Type::InProgress;
	case 3:
		return Type::None;
	}

	std::unreachable();
}

MatchSuccess& Result::Success()
{
	return std::get<MatchSuccess>(myResult);
}

MatchContext& Result::Context()
{
	return std::get<MatchContext>(myResult);
}
