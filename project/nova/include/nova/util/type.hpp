#pragma once

#include <entt/entt.hpp>
#include <functional>

namespace nova {

using id_type = entt::id_type;

template <typename T>
[[nodiscard]] constexpr auto type_name() noexcept -> std::string_view {
  return entt::type_name<T>::value();
}

class TypeId {
  id_type id_{};
  std::string_view name_{"<unknown type>"};

 public:
  constexpr TypeId() noexcept = default;
  constexpr TypeId(id_type const id, std::string_view const name) noexcept
      : id_(id), name_(name) {}

  constexpr TypeId(TypeId&&) noexcept = default;
  constexpr TypeId(TypeId const&) noexcept = default;
  constexpr TypeId& operator=(TypeId&&) noexcept = default;
  constexpr TypeId& operator=(TypeId const&) noexcept = default;

  constexpr auto operator==(TypeId const& other) const noexcept -> bool {
    return id_ == other.id_ and name_ == other.name_;
  }

  constexpr auto operator<=>(TypeId const&) const = default;

  [[nodiscard]] constexpr auto id() const noexcept -> id_type { return id_; }
  [[nodiscard]] constexpr auto hash() const noexcept -> id_type { return id_; }
  [[nodiscard]] constexpr auto name() const noexcept -> std::string_view {
    return name_;
  }
};

template <typename T>
[[nodiscard]] constexpr auto type_id() noexcept -> TypeId {
  return TypeId{entt::type_hash<T>::value(), type_name<T>()};
}

}  // namespace nova

template <>
struct std::hash<nova::TypeId> {
  auto operator()(nova::TypeId const& id) const noexcept -> std::size_t {
    return static_cast<std::size_t>(id.hash());
  }
};
