#include "PatternMatcher.h"

#include <cassert>
#include <stack>

void PatternMatcher::AddFragment(std::unique_ptr<IPatternMatcherFragment>&& aFragment)
{
	assert(myFragments.find(aFragment->myName) == myFragments.end());

	myFragments[aFragment->myName] = std::move(aFragment);
}

Expect PatternMatcher::Resolve()
{
	for (auto& [ name, fragment ] : myFragments)
	{
		Expect result = fragment->Resolve(myFragments);
		if (!result)
			return result;
	}

	return {};
}

std::optional<MatchSuccess> PatternMatcher::Match(std::string aRoot, CharRange aRange, size_t aMaxDepth)
{
	auto it = myFragments.find(aRoot);
	if (it == myFragments.end())
		return {};

	std::stack<MatchContext> contexts;

	contexts.push(it->second->Match(aRange));

	Result lastResult;

	while (!contexts.empty())
	{
		MatchContext& ctx = contexts.top();

		lastResult = ctx.myPattern->Resume(ctx, lastResult);

		if (contexts.size() >= aMaxDepth)
		{
			lastResult = MatchFailure{};
		}

		switch (lastResult.GetType())
		{
		case Result::Type::Success:
		case Result::Type::Failure:
			contexts.pop();
			break;

		case Result::Type::InProgress:
			contexts.push(lastResult.Context());
			lastResult = {};
			break;
		case Result::Type::None:
			assert(false);
			break;
		}
	}

	switch (lastResult.GetType())
	{
		case Result::Type::Success:
			return lastResult.Success();

		case Result::Type::Failure:
			return {};

		case Result::Type::InProgress:
		case Result::Type::None:
			assert(false);
			break;
	}

	std::unreachable();
}
