#pragma once

#include <string>
#include <vector>
#include <string_view>
#include <optional>
#include <unordered_map>
#include <memory>
#include <expected>

#include "pattern_matcher/RepeatCount.h"

class IPatternMatcherFragment;
using CharRange = std::string_view;
using FragmentCollection = std::unordered_map<std::string, std::unique_ptr<IPatternMatcherFragment>>;
using Expect = std::expected<void, std::string>;

struct PatternMatch
{
	IPatternMatcherFragment* myPattern;
	CharRange myRange;

	std::vector<PatternMatch> mySubMatches;
};

class IPatternMatcherFragment
{
public:
	IPatternMatcherFragment(std::string aName);

	std::string myName;

	virtual ~IPatternMatcherFragment() = default;
	virtual std::optional<PatternMatch> Match(const CharRange aRange) = 0;
	virtual Expect Resolve(const FragmentCollection& aFragments) = 0;

	PatternMatch Success(const CharRange aRange);
	PatternMatch Success(const CharRange aRange, std::vector<PatternMatch> aSubMatches);
};

namespace fragments
{
	class LiteralFragment : public IPatternMatcherFragment
	{
	public:
		LiteralFragment(std::string aName, std::string aLiteral);

		Expect Resolve(const FragmentCollection& aFragments) override;

		std::optional<PatternMatch> Match(const CharRange aRange) override;

	private:
		std::string myLiteral;
	};

	class RepeatFragment : public IPatternMatcherFragment
	{
	public:
		RepeatFragment(std::string aName, std::string aBase, RepeatCount aCount);

		Expect Resolve(const FragmentCollection& aFragments) override;

		std::optional<PatternMatch> Match(const CharRange aRange) override;

	private:
		RepeatCount myCount;
		IPatternMatcherFragment* myResolved;
		std::string myBase;
	};

	class SequenceFragment : public IPatternMatcherFragment
	{
	public:
		SequenceFragment(std::string aName, std::vector<std::string> aParts);

		Expect Resolve(const FragmentCollection& aFragments) override;

		std::optional<PatternMatch> Match(const CharRange aRange) override;

	private:
		std::vector<std::string> myParts;
		std::vector<IPatternMatcherFragment*> myResolvedParts;
	};

	class AlternativeFragment : public IPatternMatcherFragment
	{
	public:
		AlternativeFragment(std::string aName, std::vector<std::string> aParts);

		Expect Resolve(const FragmentCollection& aFragments) override;

		std::optional<PatternMatch> Match(const CharRange aRange) override;

	private:
		std::vector<std::string> myParts;
		std::vector<IPatternMatcherFragment*> myResolvedParts;
	};

}
