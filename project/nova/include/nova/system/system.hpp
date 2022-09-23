#pragma once

#include <algorithm>
#include <entt/entt.hpp>
#include <exception>
#include <format>
#include <functional>
#include <nova/debug/debug.hpp>
#include <nova/label/label.hpp>
#include <nova/resource/resource.hpp>
#include <nova/util/common.hpp>
#include <nova/util/meta.hpp>
#include <nova/util/type.hpp>
#include <nova/world.hpp>
#include <ranges>

#include "system_data.hpp"
#include "view.hpp"

namespace nova {

template <typename TComponent>
struct component_param {};

template <typename TResource>
struct resource_param {};

template <typename T>
struct system_param_fetch;

template <typename T>
struct system_param;

namespace concepts {
template <typename TParam>
concept system_param_access = requires {
  { nova::system_param<TParam>::access() } -> std::same_as<Access>;
};

template <typename TParam>
concept system_param_without_state = requires(SystemMeta const& meta,
                                              World& world) {
  { nova::system_param<TParam>::param(meta, world) } -> std::same_as<TParam>;
}
and system_param_access<TParam>;

template <typename TParam>
concept system_param_with_state =
    requires(typename nova::system_param<TParam>::state_t& state,
             SystemMeta const& meta, World& world) {
  {
    nova::system_param<TParam>::param(state, meta, world)
    } -> std::same_as<TParam>;

  {
    nova::system_param<TParam>::init(meta, world)
    } -> std::same_as<typename nova::system_param<TParam>::state_t>;
}
and system_param_access<TParam>;

template <typename TParam>
concept system_param =
    system_param_without_state<TParam> or system_param_with_state<TParam>;
}  // namespace concepts

template <typename T>
struct missing_resource : std::exception {
  std::string message{
      std::format("Could not find resource: `{}`", type_name<T>())};
  auto what() const noexcept -> const char* override {
    return std::data(message);
  }
};

// Implementation of system_param<T> for builtin types

template <typename T>
struct system_param<Resource<T>> {
  static auto param(SystemMeta const&, World& world) -> Resource<T> {
    if (auto resource = world.resources().get<T>(); not resource.has_value())
        [[unlikely]] {
      throw missing_resource<T>{};
    } else {
      return *std::move(resource);
    }
  }

  static constexpr auto access() -> Access {
    constexpr auto access = type_id<resource_param<std::remove_const_t<T>>>();
    if constexpr (std::is_const_v<T>) {
      return Access{
          .read_only = std::vector<TypeId>{access},
      };
    } else {
      return Access{
          .read_write = std::vector<TypeId>{access},
      };
    }
  }
};

template <typename T>
struct system_param<Optional<Resource<T>>> {
  static auto param(SystemMeta const&, World& world) -> Optional<Resource<T>> {
    return world.resources().get<T>();
  }

  static constexpr auto access() -> Access {
    constexpr auto access = type_id<resource_param<std::remove_const_t<T>>>();
    if constexpr (std::is_const_v<T>) {
      return Access{
          .read_only = std::vector<TypeId>{access},
      };
    } else {
      return Access{
          .read_write = std::vector<TypeId>{access},
      };
    }
  }
};

template <typename T>
struct system_param<Local<T>> {
  using state_t = T;

  static constexpr auto param(state_t& state, SystemMeta const&, World&)
      -> Local<T> {
    return Local{state};
  }

  static constexpr auto init(SystemMeta const&, World& world) -> state_t {
    if constexpr (concepts::from_world<T>) {
      return from_world<T>{}(world);
    } else {
      return T{};
    }
  }

  static constexpr auto access() -> Access {
    // Local<T> has no access since its local to the system.
    return {};
  }
};

template <typename... TWith, typename... TWithout>
struct system_param<View<With<TWith...>, Without<TWithout...>>> {
  using view_t = View<With<TWith...>, Without<TWithout...>>;

  static constexpr auto param(SystemMeta const&, World& world) -> view_t {
    return view_t{world.registry().view<TWith...>(entt::exclude<TWithout...>)};
  }

  static constexpr auto access() -> Access {
    namespace views = std::ranges::views;

    constexpr auto ids = std::array{
        std::tuple{type_id<component_param<std::remove_const_t<TWith>>>(),
                   std::is_const_v<TWith>}...};

    constexpr auto is_const = [](const auto& data) {
      return std::get<1>(data);
    };

    auto read_only = ids | views::filter(is_const) | views::elements<0>;
    auto read_write =
        ids | views::filter(std::not_fn(is_const)) | views::elements<0>;

    return Access{
        .read_only =
            std::vector<TypeId>{std::begin(read_only), std::end(read_only)},
        .read_write =
            std::vector<TypeId>{std::begin(read_write), std::end(read_write)},
    };
  }
};

template <typename TWorld>
requires(std::is_same_v<std::remove_cvref_t<TWorld>, World>and
             std::is_lvalue_reference_v<TWorld>) struct system_param<TWorld> {
  static constexpr auto param(SystemMeta const&, World& world) -> TWorld {
    return world;
  }

  static constexpr auto access() -> Access {
    // TODO: this is exclusive, so not too sure exactly what to do
    if constexpr (std::is_const_v<std::remove_reference_t<TWorld>>) {
      return Access{
          .read_only = std::vector<TypeId>{type_id<World>()},
      };
    } else {
      return Access{
          .read_write = std::vector<TypeId>{type_id<World>()},
      };
    }
  }
};

template <typename TResources>
requires(std::is_same_v<std::remove_cvref_t<TResources>, Resources>and
             std::is_lvalue_reference_v<
                 TResources>) struct system_param<TResources> {
  static constexpr auto param(SystemMeta const&, World& world) -> TResources {
    return world.resources();
  }

  static constexpr auto access() -> Access {
    if constexpr (std::is_const_v<std::remove_reference_t<TResources>>) {
      return Access{
          .read_only = std::vector<TypeId>{type_id<Resources>()},
      };
    } else {
      return Access{
          .read_write = std::vector<TypeId>{type_id<Resources>()},
      };
    }
  }
};

template <typename TRegistry>
requires(
    std::is_same_v<std::remove_cvref_t<TRegistry>, Registry>and
        std::is_lvalue_reference_v<TRegistry>) struct system_param<TRegistry> {
  static constexpr auto param(SystemMeta const&, World& world) -> TRegistry {
    return world.registry();
  }

  static constexpr auto access() -> Access {
    if constexpr (std::is_const_v<std::remove_reference_t<TRegistry>>) {
      return Access{
          .read_only = std::vector<TypeId>{type_id<Registry>()},
      };
    } else {
      return Access{
          .read_write = std::vector<TypeId>{type_id<Registry>()},
      };
    }
  }
};

namespace detail {
template <typename TSystemParam>
struct system_param_impl;

template <typename TSystemParam>
requires nova::concepts::system_param_with_state<TSystemParam>
struct system_param_impl<TSystemParam> : nova::system_param<TSystemParam> {
};

template <typename TSystemParam>
requires nova::concepts::system_param_without_state<TSystemParam>
struct system_param_impl<TSystemParam> : nova::system_param<TSystemParam> {
  struct state_t {};

  static constexpr auto init(SystemMeta const&, World&) -> state_t {
    return {};
  }

  static constexpr auto param(state_t&, SystemMeta const& meta, World& world)
      -> decltype(nova::system_param<TSystemParam>::param(meta, world)) {
    return nova::system_param<TSystemParam>::param(meta, world);
  }
};

template <typename T>
struct function_wrapper {
  template <class TFunc>
  constexpr explicit(true) function_wrapper(std::in_place_t, TFunc&& func)
      : func(FWD(func)) {}
  T func{};
};

template <typename T>
requires(std::is_function_v<T>) struct function_wrapper<T> {
  template <class TFunc>
  constexpr explicit(true) function_wrapper(std::in_place_t, TFunc& func)
      : func(std::addressof(func)) {}
  T* func{};
};

template <typename Args>
struct SystemState;
template <typename... TArgs>
struct SystemState<args<TArgs...>>
    : std::tuple<typename system_param_impl<TArgs>::state_t...> {
  using std::tuple<typename system_param_impl<TArgs>::state_t...>::tuple;
};

template <typename TSystem>
struct SystemData {
  tl::optional<SystemState<args_t<TSystem>>> state;
  function_wrapper<TSystem> system;
};

template <typename TSystem>
constexpr auto get_system_access() -> Access {
  using func_traits = function_traits<std::remove_cvref_t<TSystem>>;
  return []<typename... TArgs>(args<TArgs...>) -> Access {
    auto access = Access{};
    (access.merge(system_param<TArgs>::access()), ...);
    return access;
  }(typename func_traits::args_t{});
}

// helper function that aggregate all the system arguments, check them, and then
// invoke the original function
template <typename TSystem, typename... Args>
auto type_erased_runc_func_impl(SystemMeta const& meta, void* const data,
                                World& world, args<Args...>) -> void {
  static_assert((nova::concepts::system_param<Args> and ...),
                "System has invalid argument types");

  auto* const system_data = reinterpret_cast<SystemData<TSystem>*>(data);

  [&]<auto... Is, typename... TSystemParam>(std::index_sequence<Is...>,
                                            type_list<TSystemParam...>) {
    DEBUG_ASSERT(system_data->state.has_value(),
                 "system `{}` is not initialized!", meta.id.name());

    std::invoke(
        system_data->system.func,
        TSystemParam::param(std::get<Is>(*system_data->state), meta, world)...);
  }
  (std::index_sequence_for<Args...>{}, type_list<system_param_impl<Args>...>{});
}

// type erased system function that reinterprets the function pointer to the
// original type
template <typename TSystem>
auto type_erased_runc_func(SystemMeta const& meta, void* const data,
                           void* const world_ptr) -> void {
  using func_traits = function_traits<TSystem>;
  static_assert(std::is_same_v<void, typename func_traits::result_t>,
                "Systems must return `void`");

  auto& world = *reinterpret_cast<World*>(world_ptr);
  type_erased_runc_func_impl<TSystem>(meta, data, world,
                                      typename func_traits::args_t{});
}

template <typename TSystem, typename... Args>
auto type_erased_initialize_func_impl(SystemMeta const& meta, void* const data,
                                      World& world, args<Args...>) -> void {
  auto* const system_data = reinterpret_cast<SystemData<TSystem>*>(data);
  [&]<auto... Is, typename... TSystemParam>(std::index_sequence<Is...>,
                                            type_list<TSystemParam...>) {
    DEBUG_ASSERT(not system_data->state.has_value(),
                 "system `{}`'s state is being initialize more than once!",
                 meta.id.name());

    system_data->state.emplace(TSystemParam::init(meta, world)...);
  }
  (std::index_sequence_for<Args...>{}, type_list<system_param_impl<Args>...>{});
}

template <typename TSystem>
auto type_erased_initialize_func(SystemMeta const& meta, void* const data,
                                 void* const world_ptr) -> void {
  using func_traits = function_traits<TSystem>;

  auto& world = *reinterpret_cast<World*>(world_ptr);
  type_erased_initialize_func_impl<TSystem>(meta, data, world,
                                            typename func_traits::args_t{});
}

template <typename TSystem>
auto create_system(TSystem&& system) -> System {
  using system_t = std::remove_cvref_t<TSystem>;
  using system_data_t = detail::SystemData<system_t>;
  return System{
      .run_func = detail::type_erased_runc_func<system_t>,
      .initialize_func = detail::type_erased_initialize_func<system_t>,
      .data = void_ptr::create<system_data_t>(
          tl::nullopt,
          detail::function_wrapper<system_t>{std::in_place, FWD(system)}),
      .meta = SystemMeta{
          .id = type_id<system_t>(),
      }};
}

}  // namespace detail

namespace concepts {

template <typename T>
concept system = any_callable<T>;

}  // namespace concepts

template <concepts::system TSystem>
struct into_label<TSystem> {
  template <typename T>
  constexpr auto operator()(T&& system) const noexcept -> Label {
    static_assert(
        std::is_same_v<TSystem, std::remove_cvref_t<decltype(system)>>);
    constexpr auto name = type_name<TSystem>();
    return Label{
        .id = entt::hashed_string{std::data(name), std::size(name)},
        .name = std::string{name},
    };
  }
};

struct SystemDescriptor {
  System system;
  Labels labels{};
  Access access{};
  Ordering ordering{};
};

template <concepts::system TSystem>
struct into_system_descriptors<TSystem> {
  template <typename T>
  auto operator()(T&& system) -> SystemDescriptor {
    static_assert(std::is_same_v<std::remove_cvref_t<T>, TSystem>);
    auto labels = Labels{};
    labels.push_back(to_label(system));
    return SystemDescriptor{
        .system = detail::create_system(FWD(system)),
        .labels = MOV(labels),
        .access = detail::get_system_access<TSystem>(),
        .ordering = {},
    };
  }
};

}  // namespace nova
