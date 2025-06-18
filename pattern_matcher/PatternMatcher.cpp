#include "PatternMatcher.h"

#include <cassert>

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

std::optional<PatternMatch> PatternMatcher::Match(std::string aRoot, CharRange aRange)
{
	auto it = myFragments.find(aRoot);
	if (it == myFragments.end())
		return {};

	return it->second->Match(aRange);
}
