#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
// clang-format off
#include <doctest/doctest.h>
// clang-format on

#include "nova/system/system_builder.hpp"

#include "common.hpp"
#include "nova/system/system.hpp"

#define ALL_POSSILBE_PARAMS                                            \
  nova::View<nova::With<int, const float>>, nova::World &,             \
      nova::World const &, nova::Resources &, nova::Resources const &, \
      nova::Registry &, nova::Registry const &, nova::Resource<int>,   \
      nova::Resource<const int>, nova::Local<int>

auto func(ALL_POSSILBE_PARAMS) {}
constexpr auto lambda = [](ALL_POSSILBE_PARAMS) {};

constexpr auto make_labels = [](auto &&...values) -> nova::Labels {
  auto labels = nova::Labels{};
  (labels.push_back(nova::to_label(FWD(values))), ...);
  return labels;
};

constexpr auto check_all_labels = [](const auto &descriptor, const auto &labels,
                                     const auto &ordering) {
  CHECK(test::equals_unordered(descriptor.labels, labels));
  CHECK(test::equals_unordered(descriptor.ordering.before, ordering.before));
  CHECK(test::equals_unordered(descriptor.ordering.after, ordering.after));
};

TEST_CASE("system builder access") {
  auto from_func = nova::to_descriptors(nova::system(func));
  auto from_lambda = nova::to_descriptors(nova::system(lambda));

  CHECK(test::equals_unordered(from_func.access.read_only,
                               from_lambda.access.read_only));
  CHECK(test::equals_unordered(from_func.access.read_write,
                               from_lambda.access.read_write));
}

TEST_CASE("labeling") {
  SUBCASE("const char*") {
    const auto descriptor = nova::to_descriptors(
        nova::system(func).label("a").before("b").after("c"));

    check_all_labels(descriptor, make_labels(func, "a"),
                     nova::Ordering{
                         .before = make_labels("b"),
                         .after = make_labels("c"),
                     });
  }

  SUBCASE("std::string") {
    const auto descriptor = nova::to_descriptors(nova::system(func)
                                                     .label(std::string{"a"})
                                                     .before(std::string{"b"})
                                                     .after(std::string{"c"}));

    check_all_labels(descriptor, make_labels(func, std::string{"a"}),
                     nova::Ordering{
                         .before = make_labels(std::string{"b"}),
                         .after = make_labels(std::string{"c"}),
                     });
  }

  SUBCASE("std::string_view") {
    const auto descriptor =
        nova::to_descriptors(nova::system(func)
                                 .label(std::string_view{"a"})
                                 .before(std::string_view{"b"})
                                 .after(std::string_view{"c"}));

    check_all_labels(descriptor, make_labels(func, std::string_view{"a"}),
                     nova::Ordering{
                         .before = make_labels(std::string_view{"b"}),
                         .after = make_labels(std::string_view{"c"}),
                     });
  }

  SUBCASE("tag types") {
    struct Label {};
    struct Before {};
    struct After {};

    const auto descriptor1 = nova::to_descriptors(
        nova::system(func).label<Label>().before<Before>().after<After>());

    const auto descriptor2 = nova::to_descriptors(
        nova::system(func).label(Label{}).before(Before{}).after(After{}));

    const auto check = [](const auto &descriptor) {
      check_all_labels(descriptor, make_labels(func, Label{}),
                       nova::Ordering{
                           .before = make_labels(Before{}),
                           .after = make_labels(After{}),
                       });
    };

    check(descriptor1);
    check(descriptor2);
  }

  SUBCASE("systems") {
    SUBCASE("function") {
      const auto descriptor = nova::to_descriptors(
          nova::system(lambda).label(func).before(func).after(func));

      check_all_labels(descriptor, make_labels(lambda, func),
                       nova::Ordering{
                           .before = make_labels(func),
                           .after = make_labels(func),
                       });
    }

    SUBCASE("lambda") {
      const auto descriptor = nova::to_descriptors(
          nova::system(func).label(lambda).before(lambda).after(lambda));

      check_all_labels(descriptor, make_labels(func, lambda),
                       nova::Ordering{
                           .before = make_labels(lambda),
                           .after = make_labels(lambda),
                       });
    }
  }
}

TEST_CASE("system set") {
  const auto descriptors = nova::to_descriptors(nova::system_set()
                                                    .with_system(func)
                                                    .with_system(lambda)
                                                    .label("a")
                                                    .before("b")
                                                    .after("c"));

  REQUIRE(2u == std::size(descriptors));
  const auto check = [](const auto &descriptor, auto &&system) {
    check_all_labels(descriptor, make_labels(FWD(system), "a"),
                     nova::Ordering{
                         .before = make_labels("b"),
                         .after = make_labels("c"),
                     });
  };

  check(descriptors[0], func);
  check(descriptors[1], lambda);
}
