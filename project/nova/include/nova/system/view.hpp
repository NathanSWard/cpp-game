#pragma once
#include <entt/entt.hpp>
#include <nova/util/common.hpp>
#include <type_traits>

namespace nova {

template <typename... TComponents>
struct With {};

template <typename... TComponentss>
struct Without {};

namespace detail {

template <typename TWith, typename TWithout>
struct entt_view_t;

template <typename... TWith, typename... TWithout>
struct entt_view_t<With<TWith...>, Without<TWithout...>>
    : std::remove_cvref_t<
          decltype(std::declval<entt::registry&>().view<TWith...>(
              entt::exclude<TWithout...>))> {};

}  // namespace detail

template <typename W, typename WO = Without<>>
class View;

template <typename... TWith, typename... TWithout>
struct View<With<TWith...>, Without<TWithout...>>
    : detail::entt_view_t<With<TWith...>, Without<TWithout...>> {
  using base_t = detail::entt_view_t<With<TWith...>, Without<TWithout...>>;

  template <typename T>
  requires(not std::is_same_v<std::remove_cvref_t<T>, View>) explicit(
      true) constexpr View(T&& repr) noexcept
      : base_t(FWD(repr)) {}
};

}  // namespace nova
