#pragma once

#include <entt/entt.hpp>

#include "bundle/bundle.hpp"
#include "debug/debug.hpp"
#include "util/common.hpp"
#include "util/reflection.hpp"

namespace nova {

struct Registry : entt::registry {
  template <typename TBundle>
  requires(concepts::bundle<std::remove_cvref_t<TBundle>>) auto emplace_bundle(
      const entt::entity e, TBundle&& bundle) -> void {
    reflect::for_each(FWD(bundle), [this, e](auto&& component) {
      using component_t = std::remove_cvref_t<decltype(component)>;
      if constexpr (concepts::bundle<component_t>) {
        this->emplace_bundle(e, FWD(component));
      } else {
        this->emplace<component_t>(e, FWD(component));
      }
    });
  }

  /// @brief Erases the bundle for a given entity.
  /// If the bundle does not exist for the entity, this is UB.
  /// Consider using `remove_bundle<TBundle>` if you are unsure if the
  /// entity has every component from the bundle.
  ///
  /// @tparam TBundle The bundle type to remove.
  /// @param e The entity to erase the bundle from.
  template <concepts::bundle TBundle>
  auto erase_bundle(const entt::entity e) -> void {
    reflect::for_each_type<TBundle>(
        [this, e]<typename T>(std::type_identity<T>) -> void {
          if constexpr (concepts::bundle<T>) {
            this->erase_bundle<T>(e);
          } else {
            this->erase<T>(e);
          }
        });
  }

  /// @brief Erases the bundle's components for a given entity, if
  /// and only if the components exist. Consider using `erase_bundle` if
  /// you are sure the Bundle exists for the given entity.
  ///
  /// @tparam TBundle The bundle type to remove.
  /// @param e The entity to erase the bundle from.
  template <concepts::bundle TBundle>
  auto remove_bundle(const entt::entity e) {
    reflect::for_each_type<TBundle>(
        [this, e]<typename T>(std::type_identity<T>) -> void {
          if constexpr (concepts::bundle<T>) {
            this->remove_bundle<T>(e);
          } else {
            this->remove<T>(e);
          }
        });
  }

  /// @brief Check if the given entity has each component from a bundle.
  ///
  /// @tparam TBundle The bundle type
  /// @param e The entity to erase to check.
  /// @return True if the entity has each component of the bundle.
  template <concepts::bundle TBundle>
  auto has_bundle(const entt::entity e) -> bool {
    return [this, e]<typename... Ts>(type_list<Ts...>) -> bool {
      const auto check_bundle = [this,
                                 e]<typename T>(std::type_identity<T>) -> bool {
        if constexpr (concepts::bundle<T>) {
          return this->has_bundle<T>(e);
        } else {
          return this->try_get<T>(e) != nullptr;
        }
      };

      return (... and check_bundle(std::type_identity<Ts>{}));
    }(reflect::member_type_list<TBundle>{});
  }
};

}  // namespace nova