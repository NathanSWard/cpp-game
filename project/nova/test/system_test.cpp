#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
// clang-format off
#include <doctest/doctest.h>
// clang-format on

#include "nova/system/system.hpp"

#include <ranges>

#include "common.hpp"
#include "nova/system/system_builder.hpp"

TEST_CASE("systems run correctly") {
  using ref_t = std::reference_wrapper<int>;

  struct my_resource {
    ref_t ref;
  };

  struct my_component {
    ref_t ref;
  };

  int resource = 0;
  int component = 0;

  auto world = nova::World{};
  world.resources().set<my_resource>(std::ref(resource));

  auto& reg = world.registry();
  const auto e = reg.create();
  reg.emplace<my_component>(e, std::ref(component));

  auto func1 = [](nova::Resource<my_resource> res) { res->ref.get() += 1; };
  auto func2 = [](nova::View<nova::With<my_component>> view) {
    CHECK(1u == std::size(view));
    for (auto&& [_, component] : view.each()) {
      component.ref.get() += 1;
    }
  };

  auto system1 = nova::detail::create_system(func1);
  auto system2 = nova::detail::create_system(func2);

  auto* const world_ptr = static_cast<void*>(&world);

  system1.initialize(world_ptr);
  CHECK(resource == 0);
  system1.run(world_ptr);
  CHECK(resource == 1);
  system1.run(world_ptr);
  CHECK(resource == 2);

  system2.initialize(world_ptr);
  CHECK(component == 0);
  system2.run(world_ptr);
  CHECK(component == 1);
  system2.run(world_ptr);
  CHECK(component == 2);
}

TEST_CASE("throws when expecting a resource that doesn't exist") {
  auto func = [](nova::Resource<int>) {};
  auto system = nova::detail::create_system(func);

  auto world = nova::World{};
  system.initialize(&world);

  CHECK_THROWS_AS(system.run(&world), nova::missing_resource<int>);
}

TEST_CASE("Local<T> are not shared across systems") {
  auto func1 = [](nova::Local<int> i) {
    *i += 2;
    CHECK(*i % 2 == 0);
  };

  auto func2 = [](nova::Local<int> i) {
    *i += 3;
    CHECK(*i % 3 == 0);
  };

  auto system1 = nova::detail::create_system(func1);
  auto system2 = nova::detail::create_system(func2);

  auto world = nova::World{};
  system1.initialize(&world);
  system2.initialize(&world);

  for ([[maybe_unused]] const auto _ : std::ranges::views::iota(0, 3)) {
    system1.run(&world);
    system2.run(&world);
  }
}
