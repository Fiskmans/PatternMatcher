
#pragma once

#include <limits>

struct RepeatCount
{
	RepeatCount() = default;
	RepeatCount(size_t aFixed);
	RepeatCount(size_t aMin, size_t aMax);

	static constexpr size_t Unbounded = std::numeric_limits<size_t>::max();

	size_t myMin;
	size_t myMax;
};