#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
// clang-format off
#include <doctest/doctest.h>
// clang-format on

#include <algorithm>

#include "nova/scheduler/graph.hpp"

#include "common.hpp"
#include "nova/system/system.hpp"
#include "nova/system/system_builder.hpp"

TEST_CASE("topological order") {
  auto a = nova::into_descriptor(nova::system([] {}).label("a").after("b"));
  auto b = nova::into_descriptor(nova::system([] {}).label("b"));
  auto c = nova::into_descriptor(nova::system([] {}).label("c").before("b"));

  const auto systems = std::array{MOV(a), MOV(b), MOV(c)};

  const auto dependencies = nova::build_dependency_graph(systems);
  const auto order = nova::topological_order(dependencies);

  // expected order c -> b -> a
  REQUIRE(order.has_value());
  CHECK(std::ranges::equal(*order, std::array{2, 1, 0}));
}

TEST_CASE("topological order w/ cycle") {
  auto a = nova::into_descriptor(nova::system([] {}).label("a").after("c"));
  auto b = nova::into_descriptor(nova::system([] {}).label("b").after("a"));
  auto c = nova::into_descriptor(nova::system([] {}).label("c").after("b"));

  const auto systems = std::array{MOV(a), MOV(b), MOV(c)};

  const auto dependencies = nova::build_dependency_graph(systems);
  const auto order = nova::topological_order(dependencies);

  CHECK(not order.has_value());
}
