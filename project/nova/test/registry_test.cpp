#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
// clang-format off
#include <doctest/doctest.h>
// clang-format on

#include "nova/registry.hpp"

struct NotABundle {
  int i{};
  constexpr auto operator==(NotABundle const&) const -> bool = default;
};

struct NestedBundle {
  using is_bundle = void;

  int i{};
  float f{};
};

struct BundleType {
  using is_bundle = void;

  double d{};
  NestedBundle nested{};
  NotABundle not_a_bundle{};
  unsigned u{};
};

TEST_CASE("emplace bundle") {
  auto bundle = BundleType{
      .d = 12.3,
      .nested =
          NestedBundle{
              .i = 42,
              .f = 3.14,
          },
      .not_a_bundle =
          NotABundle{
              .i = 100,
          },
      .u = 21,
  };

  auto reg = nova::Registry{};
  const auto e = reg.create();

  CHECK(not reg.has_bundle<BundleType>(e));

  reg.emplace_bundle(e, bundle);

  const auto check = [&]<typename T>(T t) {
    const auto component = reg.try_get<T>(e);
    REQUIRE_NE(component, nullptr);
    CHECK_EQ(t, *component);
  };

  check((double)12.3);
  check((int)42);
  check((float)3.14f);
  check(NotABundle{.i = 100});
  check((unsigned)21);

  CHECK(reg.has_bundle<BundleType>(e));
}

TEST_CASE("remove/erase a bundle") {
  struct Nested {
    using is_bundle = void;
    float f{};
  };
  struct MyBundle {
    using is_bundle = void;
    int i{};
    Nested nested{};
  };

  auto reg = nova::Registry{};
  const auto e = reg.create();

  reg.emplace_bundle<MyBundle>(e, MyBundle{});
  CHECK(reg.has_bundle<MyBundle>(e));
  CHECK(reg.has_bundle<Nested>(e));
  CHECK_NE(reg.try_get<int>(e), nullptr);
  CHECK_NE(reg.try_get<float>(e), nullptr);

  SUBCASE("remove_bundle") {
    reg.remove_bundle<MyBundle>(e);
    CHECK_FALSE(reg.has_bundle<MyBundle>(e));
    CHECK_FALSE(reg.has_bundle<Nested>(e));
    CHECK_EQ(reg.try_get<int>(e), nullptr);
    CHECK_EQ(reg.try_get<float>(e), nullptr);
  }

  SUBCASE("erase_bundle") {
    reg.erase_bundle<MyBundle>(e);
    CHECK_FALSE(reg.has_bundle<MyBundle>(e));
    CHECK_FALSE(reg.has_bundle<Nested>(e));
    CHECK_EQ(reg.try_get<int>(e), nullptr);
    CHECK_EQ(reg.try_get<float>(e), nullptr);
  }
}