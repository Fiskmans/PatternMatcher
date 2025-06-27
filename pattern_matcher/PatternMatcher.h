#pragma once

#include "PatternMatcherFragment.h"

#include <unordered_map>
#include <memory>
#include <string>

class PatternMatcher
{
public:

	PatternMatcher() = default;

	void AddFragment(std::unique_ptr<IPatternMatcherFragment>&& aFragment);

	Expect Resolve();

	std::optional<MatchSuccess> Match(std::string aRoot, CharRange aRange, size_t aMaxDepth = 2'048, size_t aMaxSteps = 4'294'967'296);

private:
	std::unordered_map<std::string, std::unique_ptr<IPatternMatcherFragment>> myFragments;
};

