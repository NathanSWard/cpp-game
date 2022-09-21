#pragma once

#include <functional>
#include <nova/scheduler/scheduler.hpp>
#include <nova/world.hpp>

#include "core_stages.hpp"

namespace nova {

struct AppExit {
  bool should_exit{};
};

struct App {
  World world{};
  Scheduler scheduler{};
  std::function<void(App&)> runner{};

  /// @brief run the application with the current runner.
  /// Throws an exception if no runner is set.
  auto run() {
    if (not runner) [[unlikely]] {
      throw nova_exception{
          "nova::App does not have a runner set. Set one "
          "via `App::set_runner` or consider using `DefaultPlugins`"};
    } else {
      runner(*this);
    }
  }

  /// @brief Inserts a resource.
  ///
  /// @tparam TResource The resource type
  /// @param args The arguments used to construct the resource.
  template <typename TResource>
  auto insert_resource(auto&&... args) -> auto& {
    if constexpr (concepts::from_world<TResource>) {
      static_assert(
          sizeof...(args) == 0,
          "A resource that specializes `nova::from_world<>` cannot be "
          "inserted with arguments. Try `app.insert_resource<R>()` instead");
      world.resources().set<TResource>(from_world<R>{}(world));
    } else {
      world.resources().set<TResource>(FWD(args)...);
    }
    return *this;
  }

  /// @brief Adds a new stage to the scheduler.
  ///
  /// @tparam TStage The stage type.
  /// @param stage The stage to add.
  template <typename TStage>
  auto add_stage(TStage&& stage = {}) -> auto& {
    scheduler.add_stage(FWD(stage));
    return *this;
  }

  /// @brief Adds a new system to the specified stage.
  ///
  /// @tparam TStage The stage type.
  /// @param system The system to add.
  /// @param stage The stage the systems should to added to.
  template <typename TStage, typename TSystem>
  auto add_system_to_stage(TSystem&& system, TStage&& stage = {}) -> auto& {
    scheduler.add_system_to_stage(FWD(system), FWD(stage));
    return *this;
  }

  /// @brief Adds a new startup system (only runs once at the beginning).
  ///
  /// @param system The startup system to add.
  template <typename TSystem>
  auto add_startup_system(TSystem&& system) -> auto& {
    scheduler.add_startup_system(FWD(system));
    return *this;
  }

  /// @brief Adds a new startup system (only runs once at the end).
  ///
  /// @param system The teardown system to add.
  template <typename TSystem>
  auto add_teardown_system(TSystem&& system) -> auto& {
    scheduler.add_teardown_system(FWD(system));
    return *this;
  }

  /// @brief Adds a new system to the Update stage.
  ///
  /// @param system The system to add.
  template <typename TSystem>
  auto add_system(TSystem&& system) -> auto& {
    add_system_to_stage<stages::Update>(FWD(system));
    return *this;
  }

  /// @brief Adds a plugin to this application.
  ///
  /// @tparam The Plugin type.
  template <typename TPlugin>
  auto add_plugin(TPlugin = {}) -> auto& {
    TPlugin{}(*this);
    return *this;
  }

  /// @brief Set the current application's runner.
  ///
  /// @param new_runner The runner.
  auto set_runner(std::function<void(App&)> new_runner) -> auto& {
    runner = MOV(new_runner);
    return *this;
  }

  /// @brief Update each system in the scheduler.
  /// NOTE: Ensure the scheduler/systems are initialized first.
  auto update() { scheduler.update(world); }

  /// @brief Adds the default nova stages to the scheduler.
  /// NOTE: Generally prefer to add the DefaultPlugins instead of manually
  /// calling this function.
  auto add_default_stages() -> auto& {
    scheduler.set_first_stage<stages::First>();
    scheduler.set_last_stage<stages::Last>();

    add_stage(nova::stage<stages::PreUpdate>());
    add_stage(nova::stage<stages::Update>().after<stages::PreUpdate>());
    add_stage(nova::stage<stages::PostUpdate>().after<stages::Update>());

    return *this;
  }
};

}  // namespace nova
