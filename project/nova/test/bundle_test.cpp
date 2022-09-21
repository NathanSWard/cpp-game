#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
// clang-format off
#include <doctest/doctest.h>
// clang-format on

#include "nova/bundle/bundle.hpp"

#include <string>

TEST_CASE("bundle concept") {
  struct IsBundle {
    using is_bundle = void;
    std::string s{};
    int i{};
  };
  static_assert(nova::concepts::bundle<IsBundle>);

  struct NonAggregate {
    int i{};

   private:
    float f{};
  };
  static_assert(not nova::concepts::bundle<NonAggregate>);

  struct MissingIsBundleTypedef {
    int i{};
  };
  static_assert(not nova::concepts::bundle<MissingIsBundleTypedef>);

  struct NonAggregateWithIsBundle {
    using is_bundle = void;
    int i{};

   private:
    float f{};
  };
  static_assert(not nova::concepts::bundle<NonAggregateWithIsBundle>);
}