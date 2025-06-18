#pragma once

#include "PatternMatcherFragment.h"

#include <unordered_map>
#include <memory>
#include <string>

class PatternMatcher
{
public:

	void AddFragment(std::unique_ptr<IPatternMatcherFragment>&& aFragment);

	Expect Resolve();

	std::optional<PatternMatch> Match(std::string aRoot, CharRange aRange);

private:
	std::unordered_map<std::string, std::unique_ptr<IPatternMatcherFragment>> myFragments;
};

