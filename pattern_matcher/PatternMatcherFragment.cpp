#include "PatternMatcherFragment.h"

#include <ranges>
#include <algorithm>
#include <format>
#include <cassert>

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
		if (std::ranges::starts_with(aRange, myLiteral))
			return Success(CharRange(std::ranges::begin(aRange), std::ranges::begin(aRange) + std::ranges::size(myLiteral)));

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
		assert(myResolvedParts.size() == myParts.size() && "Using unresolved fragment");

		std::vector<PatternMatch> subMatches;

		auto start = std::ranges::begin(aRange);
		auto end = std::ranges::end(aRange);
		auto at = start;

		for (IPatternMatcherFragment* fragment : myResolvedParts)
		{
			CharRange remaining(at, end);

			std::optional<PatternMatch> result = fragment->Match(remaining);

			if (!result)
				return {};

			subMatches.push_back(*result);

			at += std::ranges::size(result->myRange);
		}

		return Success(CharRange(start, at), subMatches);
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
		assert(myResolvedParts.size() == myParts.size() && "Using unresolved fragment");

		for (IPatternMatcherFragment* fragment : myResolvedParts)
		{
			std::optional<PatternMatch> result = fragment->Match(aRange);

			if (!result)
				continue;

			return Success(result->myRange, { *result });
		}
		return {};
	}
}

PatternMatch IPatternMatcherFragment::Success(const CharRange aRange)
{
	return { this, aRange, {} };
}

PatternMatch IPatternMatcherFragment::Success(const CharRange aRange, std::vector<PatternMatch> aSubMatches)
{
	return { this, aRange, aSubMatches };
}
