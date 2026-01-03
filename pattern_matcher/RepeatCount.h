
#pragma once

#include <cstddef>
#include <limits>

namespace pattern_matcher
{
    struct RepeatCount
    {
        RepeatCount() = default;
        RepeatCount(std::size_t aFixed);
        RepeatCount(std::size_t aMin, std::size_t aMax);

        static constexpr std::size_t Unbounded = std::numeric_limits<std::size_t>::max();

        std::size_t myMin;
        std::size_t myMax;
    };
}  // namespace pattern_matcher