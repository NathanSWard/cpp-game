#pragma once

#include <exception>
#include <format>
#include <functional>
#include <nova/label/label.hpp>
#include <nova/resource/resource.hpp>
#include <nova/system/system_data.hpp>
#include <nova/util/common.hpp>
#include <nova/world.hpp>
#include <range/v3/view/tail.hpp>
#include <range/v3/view/take.hpp>
#include <range/v3/view/zip.hpp>
#include <type_traits>
#include <vector>

#include "graph.hpp"
#include "stage.hpp"

namespace nova {

namespace detail {

struct SystemSchedulingData {
  Ordering ordering{};
  Access access{};
  Labels labels{};
};

struct SystemsContainer {
  std::vector<System> systems{};
  std::vector<SystemSchedulingData> meta{};
};

}  // namespace detail

struct Stage {
  detail::SystemsContainer systems{};
};

struct Stages {
  std::vector<Stage> stages{};
  std::vector<StageMeta> meta{};
};

struct Scheduler {
  detail::SystemsContainer startup_systems{};
  Stages stages{};
  detail::SystemsContainer teardown_systems{};

  tl::optional<Label> first_stage{};
  tl::optional<Label> last_stage{};

  auto stage_count() const -> std::size_t { return std::size(stages.stages); }
  auto system_count() const -> std::size_t {
    const auto n_startup = std::size(startup_systems.systems);
    const auto n_teardown = std::size(teardown_systems.systems);
    const auto n_systems = std::accumulate(
        std::begin(stages.stages), std::end(stages.stages), std::size_t{0},
        [](const auto count, const auto& stage) {
          return count + std::size(stage.systems.systems);
        });

    return n_startup + n_teardown + n_systems;
  }

  auto stage_exists(const LabelRef label_ref) const -> bool {
    return std::ranges::find_if(stages.meta, [&](const auto& meta) -> bool {
             return meta.primary_label == label_ref;
           }) != std::end(stages.meta);
  }

  auto throw_if_stage_exists(const LabelRef label_ref) -> void {
    if (stage_exists(label_ref)) {
      throw nova_exception{std::format(
          "scheduler: failed to add stage `{}`. As it already exists.",
          label_ref.name)};
    }
  }

  template <concepts::into_label_ref TLabel>
  auto get_stage(TLabel&& label = {}) const
      -> tl::optional<std::pair<const Stage&, const StageMeta&>> {
    auto label_ref = to_label_ref(FWD(label));
    if (const auto iter = std::ranges::find_if(
            stages.meta,
            [&](const auto& meta) { return meta.primary_label == label_ref; });
        iter == std::end(stages.meta)) {
      return {};
    } else {
      const auto index = static_cast<std::size_t>(
          std::distance(std::begin(stages.meta), iter));
      return tl::make_optional(std::pair<const Stage&, const StageMeta&>{
          stages.stages[index], stages.meta[index]});
    }
  }

  template <concepts::into_stage TStage>
  auto add_stage(TStage&& stage = {}) -> void {
    auto stage_meta = nova::to_stage(FWD(stage));
    throw_if_stage_exists(stage_meta.primary_label);

    if (first_stage.has_value()) {
      stage_meta.ordering.after.push_back(*first_stage);
    }
    if (last_stage.has_value()) {
      stage_meta.ordering.before.push_back(*last_stage);
    }
    stages.stages.emplace_back();
    stages.meta.push_back(MOV(stage_meta));
  }

  template <concepts::into_stage TStage>
  auto set_first_stage(TStage&& stage = {}) -> void {
    if (first_stage) {
      throw nova::nova_exception{"scheduler's first stage is already set."};
    }

    auto stage_meta = nova::to_stage(FWD(stage));
    throw_if_stage_exists(stage_meta.primary_label);

    for (auto& meta : stages.meta) {
      meta.ordering.after.push_back(stage_meta.primary_label);
    }

    first_stage.emplace(stage_meta.primary_label);
    stages.stages.emplace_back();
    stages.meta.push_back(MOV(stage_meta));
  }

  template <concepts::into_stage TStage>
  auto set_last_stage(TStage&& stage = {}) -> void {
    if (last_stage) {
      throw nova::nova_exception{"scheduler's last stage is already set."};
    }

    auto stage_meta = nova::to_stage(FWD(stage));
    throw_if_stage_exists(stage_meta.primary_label);

    for (auto& meta : stages.meta) {
      meta.ordering.before.push_back(stage_meta.primary_label);
    }

    last_stage.emplace(stage_meta.primary_label);
    stages.stages.emplace_back();
    stages.meta.push_back(MOV(stage_meta));
  }

  template <typename TSystem>
  auto add_system_impl(TSystem&& system, auto& inner) -> void {
    const auto add_descriptor = [&](auto&& descriptor) {
      inner.systems.push_back(FWD(descriptor).system);
      inner.meta.push_back(detail::SystemSchedulingData{
          .ordering = FWD(descriptor).ordering,
          .access = FWD(descriptor).access,
          .labels = FWD(descriptor).labels,
      });
    };

    auto descriptors = to_descriptors(FWD(system));
    if constexpr (std::ranges::range<decltype(descriptors)>) {
      for (auto&& descriptor : descriptors) {
        add_descriptor(MOV(descriptor));
      }
    } else {
      add_descriptor(MOV(descriptors));
    }
  }

  template <concepts::into_label_ref TStageLabel, typename TSystem>
  auto add_system_to_stage(TSystem&& system, TStageLabel&& label = {}) -> void {
    const auto label_ref = to_label_ref(FWD(label));
    if (const auto stage = std::ranges::find(
            std::as_const(stages.meta), label_ref,
            [](const auto& meta) -> const auto& { return meta.primary_label; });
        stage != std::cend(stages.meta)) {
      const auto index = static_cast<std::size_t>(
          std::distance(std::cbegin(stages.meta), stage));
      add_system_impl(FWD(system), stages.stages[index].systems);
    } else {
      throw nova_exception{std::format(
          "add_system_to_stage: could not find stage with primary label: `{}`",
          label_ref.name)};
    }
  }

  template <typename TSystem>
  auto add_startup_system(TSystem&& system) -> void {
    add_system_impl(FWD(system), startup_systems);
  }

  template <typename TSystem>
  auto add_teardown_system(TSystem&& system) -> void {
    add_system_impl(FWD(system), teardown_systems);
  }

  auto initialize_systems(World& world) {
    constexpr auto unwrap_dependency_cycle_error = [](const auto& name,
                                                      auto&& result,
                                                      auto get_name_fn) {
      if (result.has_value()) {
        return *FWD(result);
      } else {
        const auto& cycle = result.error().cycle;
        auto message = std::format("Found a dependnecy cycle in {}:\n", name);
        for (const auto& index : cycle | ranges::views::tail) {
          std::format_to(std::back_inserter(message),
                         "- `{}`\n wants to be after\n", get_name_fn(index));
        }
        for (const auto& index : cycle | ranges::views::take(1)) {
          std::format_to(std::back_inserter(message), "- `{}`\n",
                         get_name_fn(index));
        }
        throw nova_exception{std::move(message)};
      }
    };

    const auto sort = [&]<typename TMeta, typename T>(
                          const auto& name, std::vector<TMeta>& meta,
                          std::vector<T>& repr, auto get_name_fn) {
      const auto graph = build_dependency_graph(meta);
      auto sorted_order = unwrap_dependency_cycle_error(
          name, topological_order(graph), get_name_fn);

      const auto n = std::size(repr);
      auto sorted_repr = reserved<std::vector<T>>(n);
      auto sorted_meta = reserved<std::vector<TMeta>>(n);

      for (const auto index : sorted_order) {
        sorted_repr.push_back(MOV(repr[index]));
        sorted_meta.push_back(MOV(meta[index]));
      }

      repr.swap(sorted_repr);
      meta.swap(sorted_meta);
    };

    const auto get_system_name = [](const auto& systems) {
      return [&](const auto index) -> std::string_view {
        return systems[index].meta.id.name();
      };
    };

    const auto get_stage_name = [](const auto& stage_meta) {
      return [&](const auto index) -> const auto& {
        return stage_meta[index].primary_label.name;
      };
    };

    sort("startup_systems", startup_systems.meta, startup_systems.systems,
         get_system_name(startup_systems.systems));
    sort("teardown_systems", teardown_systems.meta, teardown_systems.systems,
         get_system_name(teardown_systems.systems));

    for (auto&& [stage, meta] :
         ranges::views::zip(stages.stages, stages.meta)) {
      sort(std::format("stage:`{}` - systems", meta.primary_label.name),
           stage.systems.meta, stage.systems.systems,
           get_system_name(stage.systems.systems));
    }

    sort("stages", stages.meta, stages.stages, get_stage_name(stages.meta));

    auto* const world_ptr = static_cast<void*>(&world);

    // initialize systems
    for (auto& startup_system : startup_systems.systems) {
      startup_system.initialize(world_ptr);
    }
    for (auto& stage : stages.stages) {
      for (auto& system : stage.systems.systems) {
        system.initialize(world_ptr);
      }
    }
    for (auto& teardown_system : teardown_systems.systems) {
      teardown_system.initialize(world_ptr);
    }
  }

  auto startup(World& world) {
    for (auto& system : startup_systems.systems) {
      system.run(static_cast<void*>(std::addressof(world)));
    }
  }

  auto update(World& world) {
    for (auto& stage : stages.stages) {
      for (auto& system : stage.systems.systems) {
        system.run(static_cast<void*>(std::addressof(world)));
      }
    }
  }

  auto teardown(World& world) {
    for (auto& system : teardown_systems.systems) {
      system.run(static_cast<void*>(std::addressof(world)));
    }
  }
};

}  // namespace nova
