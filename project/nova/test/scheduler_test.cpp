#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
// clang-format off
#include <doctest/doctest.h>
// clang-format on

#include "nova/scheduler/scheduler.hpp"

#include <algorithm>
#include <ranges>

#include "common.hpp"
#include "nova/scheduler/stage.hpp"
#include "nova/system/system.hpp"
#include "nova/system/system_builder.hpp"

TEST_CASE("scheduler orders stages") {
  auto sched = nova::Scheduler{};

  sched.set_first_stage("first");
  sched.set_last_stage("last");
  sched.add_stage("2");
  sched.add_stage(nova::stage("3").after("2"));
  sched.add_stage(nova::stage("1").before("2"));

  auto world = nova::World{};
  sched.initialize_systems(world);

  const auto labels =
      sched.stages.meta | std::ranges::views::transform([
      ](const auto& meta) -> const auto& { return meta.primary_label; });

  CHECK(std::ranges::equal(
      labels, std::array{nova::to_label("first"), nova::to_label("1"),
                         nova::to_label("2"), nova::to_label("3"),
                         nova::to_label("last")}));

  CHECK(5u == sched.stage_count());
}

TEST_CASE("scheduler correctly assigns systems to stages") {
  auto sched = nova::Scheduler{};

  sched.add_stage("a");
  sched.add_stage("b");

  auto descriptor = nova::into_descriptor([] {});
  const auto system_meta = descriptor.system.meta;

  sched.add_system_to_stage(MOV(descriptor), "a");

  {
    const auto found = sched.get_stage("a");
    REQUIRE(found.has_value());

    const auto& [stage, meta] = *found;
    REQUIRE(1u == std::size(stage.systems.systems));

    CHECK(stage.systems.systems[0].meta.id == system_meta.id);
    CHECK(stage.systems.systems[0].meta.name == system_meta.name);
  }

  { const auto found = sched.get_stage("b");
    REQUIRE(found.has_value());
    const auto& [stage, _] = *found;
    CHECK(std::empty(stage.systems.systems));
    CHECK(std::empty(stage.systems.meta));
  }

  CHECK(2u == sched.stage_count());
  CHECK(1u == sched.system_count());
    
}
