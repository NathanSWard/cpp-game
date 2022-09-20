#pragma once

#include <nova/app/app.hpp>
#include <nova/app/core_stages.hpp>
#include <nova/system/system.hpp>
#include <nova/system/system_builder.hpp>

#include "time.hpp"

namespace nova {

struct TimeSystem {};

auto time_system(Resource<Time> time) -> void { time->update(); }

struct TimePlugin {
  auto operator()(App& app) -> void {
    app.insert_resource<Time>().add_system_to_stage<stages::First>(
        system(time_system).label<TimeSystem>());
  }
};

}  // namespace nova
