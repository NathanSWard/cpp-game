#pragma once

#include <nova/time/time_plugin.hpp>

#include "app.hpp"

namespace nova {

inline auto default_runner(App& app) {
  app.scheduler.initialize_systems(app.world);
  app.scheduler.startup(app.world);
  for (;;) {
    const auto should_exit = std::as_const(app.world)
                                 .resources()
                                 .get<AppExit>()
                                 .map([](const auto& app_exit) -> bool {
                                   return app_exit->should_exit;
                                 })
                                 .value_or(false);

    if (should_exit) [[unlikely]] {
      break;
    }
    app.update();
  }
  app.scheduler.teardown(app.world);
}

struct DefaultPlugins {
  auto operator()(App& app) -> void {
    app.add_default_stages()
        .add_plugin(TimePlugin{})
        .insert_resource<AppExit>(AppExit{.should_exit = false})
        .set_runner(std::function<void(App&)>{&default_runner});
  }
};
}  // namespace nova
