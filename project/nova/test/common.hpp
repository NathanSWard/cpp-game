#pragma once

#include <algorithm>
#include <numeric>
#include <ranges>

namespace test {

template <std::ranges::range TRange>

constexpr auto size(const TRange& range) -> std::size_t {
  if constexpr (std::ranges::sized_range<TRange>) {
    return std::size(range);
  } else {
    return std::accumulate(std::begin(range), std::end(range), 0u,
                           [](auto count, auto&&) { return count + 1; });
  }
}

template <std::ranges::range TFirst, std::ranges::range TSecond>
constexpr auto equals_unordered(const TFirst& first, const TSecond& second)
    -> bool {
  if (size(first) != size(second)) {
    return false;
  }

  return std::ranges::all_of(first, [&](const auto& a) {
    return std::ranges::find_if(second, [&](const auto& b) { return a == b; }) !=
           std::end(second);
  });
}

}  // namespace test
