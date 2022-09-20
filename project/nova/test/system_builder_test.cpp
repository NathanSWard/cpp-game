#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
// clang-format off
#include <doctest/doctest.h>
// clang-format on

#include "nova/system/system.hpp"

#include "common.hpp"
#include "nova/system/system_builder.hpp"

#define ALL_POSSILBE_PARAMS                                            \
  nova::View<nova::With<int, const float>>, nova::World &,             \
      nova::World const &, nova::Resources &, nova::Resources const &, \
      nova::Registry &, nova::Registry const &, nova::Resource<int>,   \
      nova::Resource<const int>, nova::Local<int>

auto func(ALL_POSSILBE_PARAMS) {}
constexpr auto lambda = [](ALL_POSSILBE_PARAMS) {};

TEST_CASE("system builder access") {
  auto from_func = nova::into_descriptor(nova::system(func));
  auto from_lambda = nova::into_descriptor(nova::system(lambda));

  CHECK(std::ranges::equal(from_func.access.read_only,
                           from_lambda.access.read_only));
  CHECK(std::ranges::equal(from_func.access.read_write,
                           from_lambda.access.read_write));
}

TEST_CASE("system builder labeling") {
  auto system = [](nova::World &) {};
  struct label_t {};
  auto descriptor = nova::into_descriptor(
      nova::system(system).label("hi").label<label_t>().after("after").before(
          "before"));

  CHECK(test::equals_unordered(
      descriptor.labels,
      std::array{nova::to_label("hi"), nova::to_label<label_t>()}));
  CHECK(test::equals_unordered(descriptor.ordering.after,
                               std::array{nova::to_label("after")}));
  CHECK(test::equals_unordered(descriptor.ordering.before,
                               std::array{nova::to_label("before")}));
}
