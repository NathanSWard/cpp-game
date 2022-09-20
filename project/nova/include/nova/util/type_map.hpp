#pragma once

#include <entt/core/type_info.hpp>
#include <tl/optional.hpp>
#include <type_traits>

#include "common.hpp"
#include "hash.hpp"
#include "type.hpp"
#include "void_ptr.hpp"

namespace nova {

class TypeMap {
  hash::hash_map_t<TypeId, void_ptr> map_;

 public:
  [[nodiscard]] auto size() const noexcept -> std::size_t {
    return std::size(map_);
  }

  [[nodiscard]] auto empty() const noexcept -> bool { return std::empty(map_); }

  auto clear() noexcept -> void { map_.clear(); }

  template <typename T, typename... TArgs>
  auto try_add(TArgs &&...args) -> std::pair<T &, bool> {
    auto const [iter, inserted] =
        map_.try_emplace(type_id<T>(), std::in_place_type<T>, FWD(args)...);
    return std::pair<T &, bool>{*static_cast<T *>(iter->second.data()),
                                inserted};
  }

  template <typename T, typename... Args>
  auto set(Args &&...args) -> T & {
    auto const [iter, _] =
        map_.insert_or_assign(type_id<T>(), void_ptr::create<T>(FWD(args)...));
    return *static_cast<T *>(iter->second.data());
  }

  template <typename T>
  [[nodiscard]] auto contains() const -> bool {
    return map_.contains(type_id<T>());
  }

  template <typename T>
  auto remove() -> tl::optional<T> {
    if (auto const iter = map_.find(type_id<T>()); iter == std::end(map_)) {
      return {};
    } else {
      auto const ptr =
          std::unique_ptr<T>{static_cast<T *>(iter->second.take())};
      map_.erase(iter);
      return tl::make_optional<T>(std::move(*ptr));
    }
  }

  template <typename T>
  [[nodiscard]] auto get() -> tl::optional<T &> {
    if (auto const iter = map_.find(type_id<T>()); iter == std::end(map_)) {
      return {};
    } else {
      const auto ptr = static_cast<T *>(iter->second.data());
      return tl::make_optional<T &>(*ptr);
    }
  }

  template <typename T>
  [[nodiscard]] auto get() const -> tl::optional<T const &> {
    if (auto const iter = map_.find(type_id<T>()); iter == std::end(map_)) {
      return {};
    } else {
      const auto ptr = static_cast<T const *>(iter->second.data());
      return tl::make_optional<T const &>(*ptr);
    }
  }

  template <typename T>
  [[nodiscard]] auto cget() const -> tl::optional<T const &> {
    return get<T>();
  }
};

}  // namespace nova
