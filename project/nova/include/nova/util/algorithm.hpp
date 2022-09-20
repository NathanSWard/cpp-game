#pragma once

#include <algorithm>
#include <ranges>

#include "common.hpp"

namespace nova {

template <std::ranges::range TRange>
constexpr auto contains(const TRange& range, auto&& pred) -> bool {
  return std::ranges::find_if(range, FWD(pred)) != std::end(range);
}

}  // namespace nova
