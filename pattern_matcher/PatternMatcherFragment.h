#pragma once

#include <string>
#include <vector>
#include <string_view>
#include <optional>
#include <unordered_map>
#include <memory>
#include <expected>
#include <variant>

#include "pattern_matcher/RepeatCount.h"

class IPatternMatcherFragment;
using CharRange = std::string_view;
using CharIterator = decltype(std::ranges::begin(std::declval<CharRange>()));

using FragmentCollection = std::unordered_map<std::string, std::unique_ptr<IPatternMatcherFragment>>;
using Expect = std::expected<void, std::string>;

struct MatchFailure {};

struct MatchSuccess
{
	IPatternMatcherFragment* myPattern;
	CharRange myRange;

	std::vector<MatchSuccess> mySubMatches;
};

struct MatchContext
{
	IPatternMatcherFragment* myPattern;

	CharRange myRange;
	CharIterator myAt;
	int myIndex;

	std::vector<MatchSuccess> mySubMatches;
};

struct MatchNone { };

struct Result
{
	Result(MatchSuccess aResult) : myResult(aResult) { }
	Result(MatchFailure aResult) : myResult(aResult) { }
	Result(MatchContext aResult) : myResult(aResult) { }
	Result() : myResult(MatchNone{}) {}

	enum class Type
	{
		Success,
		Failure,
		InProgress,
		None
	};

	Type GetType();

	MatchSuccess& Success();
	MatchContext& Context();

private:
	std::variant<MatchSuccess, MatchFailure, MatchContext, MatchNone> myResult;
}; 

class IPatternMatcherFragment
{
public:
	IPatternMatcherFragment(std::string aName);

	std::string myName;

	virtual ~IPatternMatcherFragment() = default;
	virtual Expect Resolve(const FragmentCollection& aFragments) = 0;
	virtual MatchContext Match(const CharRange aRange);
	virtual Result Resume(MatchContext& aContext, Result aResult) = 0;

	MatchSuccess Success(const CharRange aRange);
	MatchSuccess Success(const CharRange aRange, std::vector<MatchSuccess> aSubMatches);
	MatchContext InitializeContext(const CharRange aRange);
};

namespace fragments
{
	class LiteralFragment : public IPatternMatcherFragment
	{
	public:
		LiteralFragment(std::string aName, std::string aLiteral);

		Expect Resolve(const FragmentCollection& aFragments) override;
		Result Resume(MatchContext& aContext, Result aResult) override;

	private:
		std::string myLiteral;
	};

	class SequenceFragment : public IPatternMatcherFragment
	{
	public:
		SequenceFragment(std::string aName, std::vector<std::string> aParts);

		Expect Resolve(const FragmentCollection& aFragments) override;
		Result Resume(MatchContext& aContext, Result aResult) override;

	private:
		std::vector<std::string> myParts;
		std::vector<IPatternMatcherFragment*> myResolvedParts;
	};

	class AlternativeFragment : public IPatternMatcherFragment
	{
	public:
		AlternativeFragment(std::string aName, std::vector<std::string> aParts);

		Expect Resolve(const FragmentCollection& aFragments) override;
		Result Resume(MatchContext& aContext, Result aResult) override;

	private:
		std::vector<std::string> myParts;
		std::vector<IPatternMatcherFragment*> myResolvedParts;
	};

	class RepeatFragment : public IPatternMatcherFragment
	{
	public:
		RepeatFragment(std::string aName, std::string aBase, RepeatCount aCount);

		Expect Resolve(const FragmentCollection& aFragments) override;
		Result Resume(MatchContext& aContext, Result aResult) override;

	private:
		RepeatCount myCount;
		IPatternMatcherFragment* myResolved;
		std::string myBase;
	};

}
