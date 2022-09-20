#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
// clang-format off
#include <doctest/doctest.h>
// clang-format on

#include "nova/app/app.hpp"

#include <algorithm>

#include "nova/system/system.hpp"
#include "nova/system/system_builder.hpp"

auto test_runner(const std::size_t n_updates) {
  return [=](nova::App& app) {
    app.scheduler.initialize_systems(app.world);
    app.scheduler.startup(app.world);

    for (auto n = 0u; n < n_updates; ++n) {
      app.scheduler.update(app.world);
    }

    app.scheduler.teardown(app.world);
  };
}

TEST_CASE("app order core stages correctly") {
  using namespace nova;

  auto app = nova::App{};

  using res_t = std::reference_wrapper<std::vector<std::string>>;

  auto startup = [](Resource<res_t> res) { res->get().push_back("startup"); };
  auto first = [](Resource<res_t> res) { res->get().push_back("first"); };
  auto preupdate = [](Resource<res_t> res) {
    res->get().push_back("preupdate");
  };
  auto update = [](Resource<res_t> res) { res->get().push_back("update"); };
  auto postupdate = [](Resource<res_t> res) {
    res->get().push_back("postupdate");
  };
  auto last = [](Resource<res_t> res) { res->get().push_back("last"); };
  auto teardown = [](Resource<res_t> res) { res->get().push_back("teardown"); };

  auto resource = std::vector<std::string>{};

  app.add_default_stages()
      .insert_resource<res_t>(std::ref(resource))
      .add_startup_system(startup)
      .add_system_to_stage<stages::First>(first)
      .add_system_to_stage<stages::PreUpdate>(preupdate)
      .add_system_to_stage<stages::Update>(update)
      .add_system_to_stage<stages::PostUpdate>(postupdate)
      .add_system_to_stage<stages::Last>(last)
      .add_teardown_system(teardown)
      .set_runner(std::function{test_runner(2)})
      .run();

  CHECK(12u == std::size(resource));
  CHECK(std::ranges::equal(
      resource, std::array{"startup", "first", "preupdate", "update",
                           "postupdate", "last", "first", "preupdate", "update",
                           "postupdate", "last", "teardown"}));
}

TEST_CASE("app will throw is runner is not set") {
  auto app = nova::App{};

  CHECK_THROWS_WITH(
      app.run(),
      "nova::App does not have a runner set. Set one via `App::set_runner`");
}

TEST_CASE("custom stage cannot come before the `First` stage") {
  auto app = nova::App{};
  app.add_default_stages()
      .add_stage(nova::stage("first").before<nova::stages::First>())
      .set_runner(test_runner(0));

  CHECK_THROWS(app.run());
}

TEST_CASE("custom stage cannot come after the `Last` stage") {
  auto app = nova::App{};
  app.add_default_stages()
      .add_stage(nova::stage("last").after<nova::stages::Last>())
      .set_runner(test_runner(0));

  CHECK_THROWS(app.run());
}
