#pragma once

#include <concepts>

template<class T, class KeyType, class ValueType>
concept KeyedCollection = requires(T val) {
    { val.find(std::declval<KeyType>())->second } -> std::convertible_to<ValueType>;
};

template<class LeftRange, class RightRange>
concept RangeComparable = requires {
    { std::ranges::range<LeftRange> };
    { std::ranges::range<RightRange> };
    { std::equality_comparable_with<std::ranges::range_value_t<LeftRange>, std::ranges::range_value_t<RightRange>> };
};