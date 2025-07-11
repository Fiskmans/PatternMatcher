#pragma once

#include "PatternMatcherFragment.h"

#include <unordered_map>
#include <memory>
#include <string>
#include <ranges>
#include <stack>

template<class Key = std::string, std::ranges::range TokenRange = std::string_view>
class PatternMatcher
{
public:
	PatternMatcher() = default;

	void AddFragment(std::unique_ptr<IPatternMatcherFragment<Key, TokenRange>>&& aFragment)
	{
		assert(myFragments.find(aFragment->myKey) == myFragments.end());

		myFragments[aFragment->myKey] = std::move(aFragment);
	}

	void AddLiteral(Key aKey, std::string aLiteral)
	{
		AddFragment(std::make_unique<fragments::LiteralFragment<Key, TokenRange>>(aKey, aLiteral));
	}

	Expect Resolve()
	{
		for (auto& [name, fragment] : myFragments)
		{
			Expect result = fragment->Resolve(myFragments);
			if (!result)
				return result;
		}

		return {};
	}

	std::optional<MatchSuccess<Key, TokenRange>> Match(Key aRoot, TokenRange aRange, size_t aMaxDepth = 2'048, size_t aMaxSteps = 4'294'967'296)
	{
		auto it = myFragments.find(aRoot);
		if (it == myFragments.end())
			return {};

		size_t steps = 0;

		std::stack<MatchContext<Key,TokenRange>> contexts;

		contexts.push(it->second->Match(aRange));

		Result<Key,TokenRange> lastResult;

		while (!contexts.empty())
		{
			MatchContext<Key,TokenRange>& ctx = contexts.top();

			lastResult = ctx.myPattern->Resume(ctx, lastResult);

			if (contexts.size() >= aMaxDepth)
			{
				lastResult = MatchFailure{};
			}

			switch (lastResult.GetType())
			{
			case MatchResultType::Success:
			case MatchResultType::Failure:
				contexts.pop();
				break;

			case MatchResultType::InProgress:
				contexts.push(lastResult.Context());
				lastResult = {};
				break;
			case MatchResultType::None:
				assert(false);
				break;
			}

			if (steps++ >= aMaxSteps)
				return {};
		}

		switch (lastResult.GetType())
		{
		case MatchResultType::Success:
			return lastResult.Success();

		case MatchResultType::Failure:
			return {};

		case MatchResultType::InProgress:
		case MatchResultType::None:
			assert(false);
			break;
		}

		std::unreachable();
	}

private:
	std::unordered_map<Key, std::unique_ptr<IPatternMatcherFragment<Key, TokenRange>>> myFragments;
};
