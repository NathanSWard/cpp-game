#pragma once

#include <algorithm>
#include <entt/entt.hpp>
#include <nova/label/label.hpp>
#include <nova/util/algorithm.hpp>
#include <nova/util/common.hpp>
#include <nova/util/type.hpp>
#include <nova/util/void_ptr.hpp>
#include <string>
#include <type_traits>
#include <vector>

namespace nova {

struct SystemId {
  TypeId id{};

  template <typename T>
  constexpr static auto create() noexcept -> SystemId {
    return SystemId{type_id<std::remove_cvref_t<T>>()};
  }

  constexpr auto operator==(SystemId const &rhs) const noexcept -> bool {
    return id == rhs.id;
  }
  constexpr auto operator<=>(SystemId const &) const = default;
};

struct SystemMeta {
  SystemId id;
  std::string_view name{};
};

struct System {
  using func_t = void (*)(SystemMeta const &, void *, void *);

  func_t run_func;
  func_t initialize_func;
  void_ptr data;
  SystemMeta meta;

  constexpr auto run(void *world_ptr) -> void {
    run_func(meta, data.data(), world_ptr);
  }

  constexpr auto initialize(void *world_ptr) -> void {
    initialize_func(meta, data.data(), world_ptr);
  }
};

struct Ordering {
  Labels before{};
  Labels after{};
};

struct Access {
  std::vector<TypeId> read_only{};
  std::vector<TypeId> read_write{};

  template <class T>
  static constexpr auto single() -> Access {
    constexpr auto is_const = std::is_const_v<std::remove_reference_t<T>>;
    constexpr auto id = type_id<std::remove_cvref_t<T>>();
    if constexpr (is_const) {
      return Access{
          .read_only = std::vector{id},
      };
    } else {
      return Access{
          .read_write = std::vector{id},
      };
    }
  }

  constexpr auto merge(Access &&other) -> void {
    const auto merge_vectors = [](auto &to, auto &&from) {
      to.reserve(std::size(to) + std::size(from));
      to.insert(std::end(to), std::make_move_iterator(std::begin(from)),
                std::make_move_iterator(std::end(from)));
      std::sort(std::begin(to), std::end(to));
      to.erase(std::unique(std::begin(to), std::end(to)), std::end(to));
    };

    merge_vectors(read_only, MOV(other).read_only);
    merge_vectors(read_write, MOV(other).read_write);

    // remove read_only TypeId's that exist in read_write.
    std::erase_if(read_only, [&](const auto &ro_tid) -> bool {
      return nova::contains(read_write, nova::equals(ro_tid));
    });
  }
};

template <typename>
struct into_system_descriptors;

}  // namespace nova

template <>
struct std::hash<nova::SystemId> {
  auto operator()(nova::SystemId const &id) const noexcept -> std::size_t {
    return std::hash<nova::TypeId>{}(id.id);
  }
};

template <>
struct std::hash<nova::Label> {
  constexpr auto operator()(nova::Label const &label) const noexcept
      -> std::size_t {
    return static_cast<std::size_t>(label.id);
  }
};
