#pragma once

#include <concepts>

template<class T, class KeyType, class ValueType>
concept KeyedCollection = requires(T val) {
    { val.find(std::declval<KeyType>())->second } -> std::convertible_to<ValueType>;
};

template<class Range, class T>
concept RangeComparable = requires {
    { std::ranges::range<Range> };
    { std::equality_comparable_with<T, std::ranges::range_value_t<Range>> };
};