#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
// clang-format off
#include <doctest/doctest.h>
// clang-format on

#include "nova/util/bitset.hpp"

#include <algorithm>

TEST_CASE("bitset") {
  auto bitset = nova::FixedBitset(10);
  CHECK(10u == bitset.size());

  constexpr auto enabled_bits = std::array{0u, 3u, 5u, 6u, 9u};

  for (const auto bit : enabled_bits) {
    CHECK_NOTHROW(bitset.insert(bit));
  }

  CHECK(std::ranges::equal(enabled_bits, bitset.ones()));
}
