#include "PatternMatcherFragment.h"

#include <ranges>
#include <algorithm>
#include <format>

namespace fragments
{

	LiteralFragment::LiteralFragment(std::string aLiteral)
	{
		myLiteral = aLiteral;
	}

	Expect LiteralFragment::Resolve(const FragmentCollection& aFragments)
	{
		return {};
	}

	std::optional<PatternMatch> LiteralFragment::Match(const CharRange aRange)
	{
		if (std::ranges::starts_with(myLiteral, aRange))
			return Success(aRange.substr(0, std::ranges::size(myLiteral)));

		return {};
	}

	
	SequenceFragment::SequenceFragment(std::vector<std::string> aParts)
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

	std::optional<PatternMatch> SequenceFragment::Match(const CharRange aRange)
	{
		auto start = std::ranges::begin(aRange);
		auto at = start;
		for (IPatternMatcherFragment* fragment : myResolvedParts)
		{
			CharRange subRange(at, std::ranges::end(aRange));

			std::optional<PatternMatch> result = fragment->Match(subRange);

			if (!result)
				return {};

			at = std::ranges::end(result->myRange);
		}
		return Success(CharRange(start, at));
	}

	AlternativeFragment::AlternativeFragment(std::vector<std::string> aParts)
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

	std::optional<PatternMatch> AlternativeFragment::Match(const CharRange aRange)
	{
		for (IPatternMatcherFragment* fragment : myResolvedParts)
		{
			std::optional<PatternMatch> result = fragment->Match(aRange);

			if (!result)
				continue;

			return result;
		}
		return {};
	}

}

PatternMatch IPatternMatcherFragment::Success(const CharRange aRange)
{
	return { this, aRange, {} };
}
