#pragma once

#include <string>
#include <vector>
#include <string_view>
#include <optional>
#include <unordered_map>
#include <memory>
#include <expected>
#include <variant>
#include <ranges>
#include <algorithm>
#include <format>
#include <cassert>

#include "pattern_matcher/RepeatCount.h"

using Expect = std::expected<void, std::string>;

template<typename T>
class Indexable
{
public:
	static constexpr bool IsIndexable = false;
	static constexpr size_t Count = 0;
};

template<>
class Indexable<char>
{
public:
	static constexpr bool IsIndexable = true;
	static constexpr size_t Count = 0x100;
	static size_t ConvertToIndex(char aValue) { return (size_t)aValue - std::numeric_limits<char>::min(); }
};

template<class Key, std::ranges::range TokenRange>
class IPatternMatcherFragment;

template<class TokenRange>
using IteratorType = decltype(std::ranges::begin(std::declval<TokenRange>()));

enum class MatchResultType
{
	Success,
	Failure,
	InProgress,
	None
};

struct MatchFailure {};
struct MatchNone {};

template<
	class Key = std::string,
	std::ranges::range TokenRange = std::string_view>
struct MatchSuccess
{
	IPatternMatcherFragment<Key, TokenRange>* myPattern;
	TokenRange myRange;

	std::vector<MatchSuccess> mySubMatches;
};

template<
	class Key = std::string,
	std::ranges::range TokenRange = std::string_view>
struct MatchContext
{
	IPatternMatcherFragment<Key, TokenRange>* myPattern;

	TokenRange myRange;
	IteratorType<TokenRange> myAt;
	int myIndex;

	std::vector<MatchSuccess<Key, TokenRange>> mySubMatches;
};

template<
	class Key = std::string,
	std::ranges::range TokenRange = std::string_view>
struct Result
{
	Result(MatchSuccess<Key, TokenRange>	aResult) : myResult(aResult) {}
	Result(MatchFailure						aResult) : myResult(aResult) {}
	Result(MatchContext<Key, TokenRange>	aResult) : myResult(aResult) {}
	Result() : myResult(MatchNone{}) {}

	MatchResultType GetType()
	{
		switch (myResult.index())
		{
		case 0: return MatchResultType::Success;
		case 1: return MatchResultType::Failure;
		case 2: return MatchResultType::InProgress;
		case 3: return MatchResultType::None;
		}

		std::unreachable();
	}

	MatchSuccess<Key, TokenRange>& Success()
	{
		return std::get<MatchSuccess<Key, TokenRange>>(myResult);
	}

	MatchContext<Key, TokenRange>& Context()
	{
		return std::get<MatchContext<Key, TokenRange>>(myResult);
	}

private:
	std::variant<MatchSuccess<Key, TokenRange>, MatchFailure, MatchContext<Key, TokenRange>, MatchNone> myResult;
};

template<
	class Key = std::string, 
	std::ranges::range TokenRange = std::string_view>
class IPatternMatcherFragment
{
public:
	using TokenType = std::iterator_traits<IteratorType<TokenRange>>::value_type;
	using Collection = std::unordered_map<Key, std::unique_ptr<IPatternMatcherFragment<Key, TokenRange>>>;

	IPatternMatcherFragment(Key aKey)
		: myKey(aKey)
	{
	}

	virtual ~IPatternMatcherFragment() = default;
	virtual Expect Resolve(const Collection& aFragments) = 0;
	virtual Result<Key, TokenRange> Resume(MatchContext<Key, TokenRange>& aContext, Result<Key, TokenRange> aResult) = 0;

	virtual MatchContext<Key, TokenRange> Match(const TokenRange aRange)
	{
		MatchContext<Key, TokenRange> ctx;

		ctx.myPattern = this;
		ctx.myAt = std::ranges::begin(aRange);
		ctx.myRange = aRange;
		ctx.myIndex = 0;

		return ctx;
	}

	Key myKey;
};

namespace fragments
{
	template<
		class Key = std::string, 
		std::ranges::range TokenRange = std::string_view, 
		std::ranges::range LiteralRange = std::string>
	class LiteralFragment : public IPatternMatcherFragment<Key, TokenRange>
	{
	public:
		using Collection = IPatternMatcherFragment<Key, TokenRange>::Collection;

		LiteralFragment(Key aKey, LiteralRange aLiteral)
			: IPatternMatcherFragment<Key,TokenRange>(aKey)
			, myLiteral(aLiteral)
		{
		}

		Expect Resolve(const Collection& aFragments) override
		{
			return {};
		}

		Result<Key, TokenRange> Resume(MatchContext<Key, TokenRange>& aContext, Result<Key, TokenRange> aResult) override
		{
			IteratorType<TokenRange> start = std::ranges::begin(aContext.myRange);

			if (std::ranges::starts_with(aContext.myRange, myLiteral))
				return MatchSuccess<Key, TokenRange>{ this, { start, start + std::ranges::size(myLiteral) }};

			return MatchFailure{};
		}

		LiteralRange myLiteral;
	};

	template<
		class Key = std::string, 
		std::ranges::range TokenRange = std::string_view>
	class SequenceFragment : public IPatternMatcherFragment<Key, TokenRange>
	{
	public:
		using Super = IPatternMatcherFragment<Key, TokenRange>;

		SequenceFragment(Key aKey, std::vector<Key> aParts)
			: IPatternMatcherFragment<Key, TokenRange>(aKey)
			, myParts(aParts)
		{
		}

		Expect Resolve(const Super::Collection& aFragments) override
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

		Result<Key, TokenRange> Resume(MatchContext<Key, TokenRange>& aContext, Result<Key, TokenRange> aResult) override
		{
			assert(myResolvedParts.size() == myParts.size() && "Using unresolved fragment");

			std::vector<MatchSuccess<Key, TokenRange>> subMatches;

			switch (aResult.GetType())
			{
			case MatchResultType::Failure:
				return MatchFailure{};
			case MatchResultType::Success:
				aContext.mySubMatches.push_back(aResult.Success());
				aContext.myAt += std::ranges::size(aResult.Success().myRange);
				break;
			case MatchResultType::None:
				break;
			case MatchResultType::InProgress:
			default:
				assert(false);
				std::unreachable();
				break;
			}

			IteratorType<TokenRange> start = std::ranges::begin(aContext.myRange);
			auto end = std::ranges::end(aContext.myRange);

			if (aContext.myIndex == myResolvedParts.size())
				return MatchSuccess<Key, TokenRange>{ this, { start, aContext.myAt }, aContext.mySubMatches};

			TokenRange remaining{ aContext.myAt, end };

			return myResolvedParts[aContext.myIndex++]->Match(remaining);
		}

	private:
		std::vector<Key> myParts;
		std::vector<IPatternMatcherFragment<Key,TokenRange>*> myResolvedParts;
	};

	template<
		class Key = std::string, 
		std::ranges::range TokenRange = std::string_view>
	class AlternativeFragment : public IPatternMatcherFragment<Key, TokenRange>
	{
	public:
		using Super = IPatternMatcherFragment<Key, TokenRange>;

		AlternativeFragment(Key aKey, std::vector<Key> aParts)
			: Super(aKey)
			, myParts(aParts)
		{
		}

		Expect Resolve(const Super::Collection& aFragments) override
		{
			myResolvedParts.reserve(myParts.size());
			for (std::string& part : myParts)
			{
				auto resolved = aFragments.find(part);

				if (resolved == std::end(aFragments))
					return std::unexpected(std::format("Expected {} to be a valid fragment", part));

				myResolvedParts.push_back(resolved->second.get());
			}

			if constexpr (Indexable::IsIndexable)
			{
				for (Super*& fragment : myQuickSearchLUT)
				{
					fragment = nullptr;
				}
				myQuickSearchSize = 0;
				for (Super* fragment : myResolvedParts)
				{
					LiteralFragment<Key, TokenRange>* lit = dynamic_cast<LiteralFragment<Key, TokenRange>*>(fragment);

					if (!lit)
						break;

					if (std::ranges::size(lit->myLiteral) != 1)
						break;

					size_t c = Indexable::ConvertToIndex(*std::ranges::begin(lit->myLiteral));

					myQuickSearchLUT[c] = fragment;
					myQuickSearchSize++;
				}
			}

			return {};
		}

		Result<Key, TokenRange> Resume(MatchContext<Key, TokenRange>& aContext, Result<Key, TokenRange> aResult) override
		{
			assert(myResolvedParts.size() == myParts.size() && "Using unresolved fragment");

			if constexpr (Indexable::IsIndexable)
			{
				if (aContext.myIndex == 0 && myQuickSearchSize > 0)
				{
					if (std::ranges::size(aContext.myRange) > 0)
					{
						size_t c = Indexable::ConvertToIndex(*aContext.myAt);

						IPatternMatcherFragment<Key, TokenRange>* fragment = myQuickSearchLUT[c];

						if (fragment)
						{
							TokenRange tokens { aContext.myAt, aContext.myAt + 1 };
							return MatchSuccess<Key, TokenRange> { this, tokens, { MatchSuccess<Key, TokenRange>{ fragment, tokens } } };
						}
					}

					aContext.myIndex += myQuickSearchSize;
				}

			}

			switch (aResult.GetType())
			{
			case MatchResultType::Failure:
			case MatchResultType::None:
				break;
			case MatchResultType::Success:
				return MatchSuccess<Key, TokenRange>{ this, aResult.Success().myRange, { aResult.Success() } };
			case MatchResultType::InProgress:
			default:
				assert(false);
				std::unreachable();
				break;
			}

			if (aContext.myIndex == myResolvedParts.size())
				return MatchFailure{};

			return myResolvedParts[aContext.myIndex++]->Match(aContext.myRange);
		}

	private:

		struct Empty {};
		using Indexable = Indexable<typename IPatternMatcherFragment<Key, TokenRange>::TokenType>;

		template<class V>
		using IfIndexable = std::conditional_t<Indexable::IsIndexable, V, Empty>;

		IfIndexable<IPatternMatcherFragment<Key,TokenRange>* [Indexable::Count]> myQuickSearchLUT;
		IfIndexable<size_t> myQuickSearchSize;

		std::vector<Key> myParts;
		std::vector<IPatternMatcherFragment<Key,TokenRange>*> myResolvedParts;
	};

	template<
		class Key = std::string, 
		std::ranges::range TokenRange = std::string_view>
	class RepeatFragment : public IPatternMatcherFragment<Key, TokenRange>
	{
	public:
		using Collection = IPatternMatcherFragment<Key, TokenRange>::Collection;

		RepeatFragment(Key aKey, Key aBase, RepeatCount aCount)
			: IPatternMatcherFragment<Key, TokenRange>(aKey)
			, myBase(aBase)
			, myCount(aCount)
			, myResolved(nullptr)
		{
		}

		Expect Resolve(const Collection& aFragments) override
		{
			auto it = aFragments.find(myBase);
			if (it == aFragments.end())
				return std::unexpected(std::format("Expected {} to be a valid fragment", myBase));

			myResolved = it->second.get();

			return {};
		}

		Result<Key, TokenRange> Resume(MatchContext<Key, TokenRange>& aContext, Result<Key, TokenRange> aResult) override
		{
			assert(myResolved && "Using unresolved fragment");

			switch (aResult.GetType())
			{
			case MatchResultType::Failure:
				if (aContext.myIndex > myCount.myMin)
					return MatchSuccess<Key, TokenRange>{ this, { std::ranges::begin(aContext.myRange), aContext.myAt }, aContext.mySubMatches};
				return MatchFailure{};
			case MatchResultType::Success:
				aContext.mySubMatches.push_back(aResult.Success());
				aContext.myAt += std::ranges::size(aResult.Success().myRange);
				break;
			case MatchResultType::None:
				break;
			case MatchResultType::InProgress:
			default:
				assert(false);
				std::unreachable();
				break;
			}

			if (aContext.myIndex == myCount.myMax)
				return MatchSuccess<Key, TokenRange>{ this, { std::ranges::begin(aContext.myRange), aContext.myAt }, aContext.mySubMatches };

			aContext.myIndex++;

			auto end = std::ranges::end(aContext.myRange);
			TokenRange remaining(aContext.myAt, end);

			return myResolved->Match(remaining);
		}

	private:
		RepeatCount myCount;
		IPatternMatcherFragment<Key,TokenRange>* myResolved;
		Key myBase;
	};
}