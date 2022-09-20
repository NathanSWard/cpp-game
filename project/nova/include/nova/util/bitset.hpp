#pragma once

#include <nova/debug/debug.hpp>
#include <range/v3/view/enumerate.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/iota.hpp>
#include <range/v3/view/join.hpp>
#include <range/v3/view/map.hpp>
#include <range/v3/view/transform.hpp>
#include <ranges>
#include <utility>
#include <vector>

#include "ranges.hpp"

namespace nova {

namespace detail {

template <typename T>
constexpr auto div_rem(T first, T second) -> std::pair<T, T> {
  const auto div = first / second;
  const auto rem = first % second;
  return std::pair{div, rem};
}

}  // namespace detail

class FixedBitset {
 public:
  using block_t = std::uint32_t;
  inline static constexpr std::size_t BITS = sizeof(block_t) * CHAR_BIT;

  constexpr FixedBitset() = default;
  constexpr FixedBitset(std::size_t n_bits)
      : blocks_(
            [=] {
              auto [blocks, rem] = detail::div_rem(n_bits, BITS);
              blocks += static_cast<std::size_t>(rem > 0);
              return blocks;
            }(),
            0u),
        size_(n_bits) {}

  /// @brief  Clears all bits.
  constexpr auto clear() -> void {
    for (auto& block : blocks_) {
      block = 0;
    }
  }

  /// @brief Enable `bit`.
  /// @param bit The bit to set. Panics if bit is out of range.
  constexpr auto insert(const std::size_t bit) -> void {
    NOVA_ASSERT(bit < size_, "insert at index {} exceeds FixedBitset size {}",
                bit, size_);

    const auto [block, i] = detail::div_rem(bit, BITS);
    blocks_[block] |= 1 << i;
  }

  /// @brief Iterate over all enabled bits.
  /// @return An iterator over all enabled bits where the iterator element is
  /// the index of the bit.
  auto ones() const {
    return blocks_ | ranges::views::enumerate |
           nova::join_transform([](const auto pair) {
             const auto [index, block] = pair;
             const auto block_start_index = index * BITS;
             return ranges::views::iota(std::size_t{0}, BITS) |
                    nova::filter_transform(
                        [=](const auto bit) -> tl::optional<std::size_t> {
                          if ((block >> bit) & 1) {
                            return block_start_index + bit;
                          }
                          return {};
                        });
           });
  }

  constexpr auto size() -> std::size_t { return size_; }

 private:
  std::vector<block_t> blocks_{};
  std::size_t size_{};
};

}  // namespace nova