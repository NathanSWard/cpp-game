#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
// clang-format off
#include <doctest/doctest.h>
// clang-format on

#include "nova/util/reflection.hpp"

struct Zero {};
struct One {
  int i{};
};
struct Two {
  int i{};
  float f{};
};
struct Three {
  int i{};
  Two two{};
  Zero zero{};
};

TEST_CASE("member count") {
  static_assert(0u == nova::reflect::member_count<Zero>);
  static_assert(1u == nova::reflect::member_count<One>);
  static_assert(2u == nova::reflect::member_count<Two>);
}

template <typename T, typename... Ts>
constexpr auto has_types() -> bool {
  return std::is_same_v<nova::type_list<Ts...>,
                        nova::reflect::member_type_list<T>>;
};

TEST_CASE("member_type_list") {
  static_assert(has_types<Zero>());
  static_assert(has_types<One, int>());
  static_assert(has_types<Two, int, float>());
  static_assert(has_types<Three, int, Two, Zero>());
}

TEST_CASE("get_member_references") {
  {
    const auto const_two = Two{
        .i = 42,
        .f = 3.14f,
    };

    auto [i, f] = nova::reflect::get_member_references(const_two);
    static_assert(std::is_same_v<decltype(i), const int&>);
    static_assert(std::is_same_v<decltype(f), const float&>);

    CHECK_EQ(i, const_two.i);
    CHECK_EQ(std::addressof(i), std::addressof(const_two.i));
    CHECK_EQ(f, const_two.f);
    CHECK_EQ(std::addressof(f), std::addressof(const_two.f));
  }

  {
    auto two = Two{
        .i = 42,
        .f = 3.14f,
    };

    auto [i, f] = nova::reflect::get_member_references(two);
    static_assert(std::is_same_v<decltype(i), int&>);
    static_assert(std::is_same_v<decltype(f), float&>);

    CHECK_EQ(i, two.i);
    CHECK_EQ(std::addressof(i), std::addressof(two.i));
    CHECK_EQ(f, two.f);
    CHECK_EQ(std::addressof(f), std::addressof(two.f));
  }
}

TEST_CASE("for_each") {
  nova::reflect::for_each(Two{.i = 42, .f = 3.14f}, [](auto&& value) -> void {
    static_assert(std::is_rvalue_reference_v<decltype(value)>);
  });

  const auto const_two = Two{.i = 42, .f = 3.14f};
  nova::reflect::for_each(const_two, [](auto&& value) -> void {
    static_assert(std::is_lvalue_reference_v<decltype(value)> and
                  std::is_const_v<std::remove_reference_t<decltype(value)>>);
  });

  auto two = Two{.i = 42, .f = 3.14f};
  nova::reflect::for_each(two, [](auto&& value) -> void {
    static_assert(
        std::is_lvalue_reference_v<decltype(value)> and
        not std::is_const_v<std::remove_reference_t<decltype(value)>>);
  });
}