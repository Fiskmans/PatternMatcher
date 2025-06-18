#pragma once

#include <string>
#include <vector>
#include <string_view>
#include <optional>
#include <unordered_map>
#include <memory>
#include <expected>

class IPatternMatcherFragment;
using CharRange = std::string_view;
using FragmentCollection = std::unordered_map<std::string, std::unique_ptr<IPatternMatcherFragment>>;
using Expect = std::expected<void, std::string>;

struct PatternMatch
{
	IPatternMatcherFragment* myPattern;
	CharRange myRange;

	std::vector<std::unique_ptr<PatternMatch>> mySubMatches;
};

class IPatternMatcherFragment
{
public:
	std::string myName;

	virtual ~IPatternMatcherFragment() = default;
	virtual std::optional<PatternMatch> Match(const CharRange aRange) = 0;
	virtual Expect Resolve(const FragmentCollection& aFragments) = 0;

	PatternMatch Success(const CharRange aRange);
};


namespace fragments
{
	class LiteralFragment : public IPatternMatcherFragment
	{
	public:
		LiteralFragment(std::string aLiteral);
		Expect Resolve(const FragmentCollection& aFragments) override;

		std::optional<PatternMatch> Match(const CharRange aRange) override;

	private:
		std::string myLiteral;
	};

	class SequenceFragment : public IPatternMatcherFragment
	{
	public:
		SequenceFragment(std::vector<std::string> aParts);
		Expect Resolve(const FragmentCollection& aFragments) override;

		std::optional<PatternMatch> Match(const CharRange aRange) override;

	private:
		std::vector<std::string> myParts;
		std::vector<IPatternMatcherFragment*> myResolvedParts;
	};

	class AlternativeFragment : public IPatternMatcherFragment
	{
	public:
		AlternativeFragment(std::vector<std::string> aParts);
		Expect Resolve(const FragmentCollection& aFragments) override;

		std::optional<PatternMatch> Match(const CharRange aRange) override;

	private:
		std::vector<std::string> myParts;
		std::vector<IPatternMatcherFragment*> myResolvedParts;
	};

}
