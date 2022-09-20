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

  auto run() {
    if (not runner) [[unlikely]] {
      throw nova_exception{
          "nova::App does not have a runner set. Set one "
          "via `App::set_runner`"};
    } else {
      runner(*this);
    }
  }

  template <typename R>
  auto insert_resource(auto&&... args) -> auto& {
    if constexpr (concepts::from_world<R>) {
      static_assert(
          sizeof...(args) == 0,
          "A resource that specializes `nova::from_world<>` cannot be "
          "inserted with arguments. Try `app.insert_resource<R>()` instead");
      world.resources().set<R>(from_world<R>{}(world));
    } else {
      world.resources().set<R>(FWD(args)...);
    }
    return *this;
  }

  template <typename TStage>
  auto add_stage(TStage&& stage = {}) -> auto& {
    scheduler.add_stage(FWD(stage));
    return *this;
  }

  template <typename TStage, typename TSystem>
  auto add_system_to_stage(TSystem&& system, TStage&& stage = {}) -> auto& {
    scheduler.add_system_to_stage(FWD(system), FWD(stage));
    return *this;
  }

  template <typename TSystem>
  auto add_startup_system(TSystem&& system) -> auto& {
    scheduler.add_startup_system(FWD(system));
    return *this;
  }

  template <typename TSystem>
  auto add_teardown_system(TSystem&& system) -> auto& {
    scheduler.add_teardown_system(FWD(system));
    return *this;
  }

  template <typename TSystem>
  auto add_system(TSystem&& system) -> auto& {
    add_system_to_stage<stages::Update>(FWD(system));
    return *this;
  }

  template <typename TPlugin>
  auto add_plugin(TPlugin = {}) -> auto& {
    TPlugin{}(*this);
    return *this;
  }

  auto set_runner(std::function<void(App&)> new_runner) -> auto& {
    runner = MOV(new_runner);
    return *this;
  }

  auto update() { scheduler.update(world); }

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
