#pragma once

#include <type_traits>

namespace nova {

namespace concepts {

template <typename T>
concept bundle = requires {
  typename T::is_bundle;
}
and std::is_aggregate_v<T>;

}  // namespace concepts

}  // namespace nova