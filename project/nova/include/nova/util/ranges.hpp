#pragma once

#include <range/v3/view/cache1.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/join.hpp>
#include <range/v3/view/transform.hpp>
#include <tl/optional.hpp>

#include "common.hpp"

namespace nova {

constexpr auto filter_transform(auto func) {
  return ranges::views::transform(MOV(func)) | ranges::views::cache1 |
         ranges::views::filter([](const auto& optional) -> bool {
           return static_cast<bool>(optional);
         }) |
         ranges::views::transform(
             [](auto&& optional) -> decltype(auto) { return *FWD(optional); });
}

constexpr auto join_transform(auto func) {
  return ranges::views::transform(MOV(func)) | ranges::views::join;
}

}  // namespace nova
