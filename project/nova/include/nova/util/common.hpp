#pragma once

#include <algorithm>
#include <exception>
#include <format>
#include <range/v3/view/enumerate.hpp>
#include <string_view>
#include <type_traits>

#define UNUSED(x) static_cast<void>(x)
#define MOV(...) \
  static_cast<std::remove_reference_t<decltype(__VA_ARGS__)>&&>(__VA_ARGS__)
#define FWD(...) static_cast<decltype(__VA_ARGS__)&&>(__VA_ARGS__)

namespace nova {

template <typename TContainer>
  requires(requires(TContainer& container) {
             container.reserve(std::size_t{});
           })
[[nodiscard]] constexpr auto reserved(const std::size_t size) -> TContainer {
  auto container = TContainer{};
  container.reserve(size);
  return container;
}

[[nodiscard]] constexpr auto equals(const auto& lhs) {
  return [&](const auto& rhs) -> bool { return lhs == rhs; };
}

template <typename T>
struct range_formatter {
  T& t;
};

struct nova_exception : std::exception {
  std::string message{};

  explicit(false) nova_exception(std::string message) : message(MOV(message)) {}

  auto what() const noexcept -> const char* override {
    return std::data(message);
  }
};

}  // namespace nova

template <typename T, typename TChar>
struct std::formatter<nova::range_formatter<T>, TChar> {
  constexpr auto parse(std::format_parse_context& ctx) {
    return std::begin(ctx);
  }

  template <typename FmtCtx>
  constexpr auto format(const nova::range_formatter<T>& range, FmtCtx& ctx) {
    std::format_to(ctx.out(), "[");
    for (const auto& [index, value] : range.t | ::ranges::views::enumerate) {
      if (index != 0) [[likely]] {
        std::format_to(ctx.out(), ", ");
      }
      std::format_to(ctx.out(), "{}", value);
    }
    return std::format_to(ctx.out(), "]");
  }
};
