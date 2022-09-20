#pragma once

#include <entt/entt.hpp>
#include <tl/optional.hpp>

#include "resource/resource.hpp"

namespace nova {

using Registry = entt::registry;

class World {
  Resources resources_;
  Registry registry_;

 public:
  auto resources() -> Resources& { return resources_; }
  auto resources() const -> const Resources& { return resources_; }
  auto registry() -> Registry& { return registry_; }
  auto registry() const -> const Registry& { return registry_; }
};

template <typename T>
using Optional = tl::optional<T>;

template <typename T>
struct from_world;

namespace concepts {

template <typename T>
concept from_world = requires(World& world) {
                       { nova::from_world<T>{}(world) } -> std::same_as<T>;
                     };

}  // namespace concepts

}  // namespace nova
