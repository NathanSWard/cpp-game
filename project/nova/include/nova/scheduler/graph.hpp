#pragma once

#include <algorithm>
#include <nova/system/system_data.hpp>
#include <nova/util/bitset.hpp>
#include <nova/util/common.hpp>
#include <nova/util/hash.hpp>
#include <nova/util/ranges.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/drop.hpp>
#include <range/v3/view/enumerate.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/generate.hpp>
#include <range/v3/view/join.hpp>
#include <range/v3/view/move.hpp>
#include <range/v3/view/single.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/view/zip.hpp>
#include <ranges>
#include <sstream>
#include <tl/expected.hpp>
#include <tl/optional.hpp>
#include <vector>

namespace nova {

namespace concepts {

template <typename T>
concept node = requires(T t) {
  {t.labels};
  {t.ordering};
};
}  // namespace concepts

using graph_t =
    hash::hash_map_t<std::size_t,
                     hash::hash_map_t<std::size_t, hash::hash_set_t<Label>>>;

template <std::ranges::sized_range TNodes>
requires(concepts::node<std::ranges::range_value_t<
             TNodes>>) auto build_dependency_graph(TNodes const& nodes)
    -> graph_t {
  namespace views = ranges::views;

  auto labels = hash::hash_map_t<Label, FixedBitset>{};
  for (const auto& [index, label] :
       nodes | views::enumerate | nova::join_transform([&](const auto& pair) {
         const auto& [index, node] = pair;
         return views::zip(views::generate([=] { return index; }), node.labels);
       })) {
    if (const auto iter = labels.find(label); iter == std::end(labels)) {
      const auto [emplaced, _] =
          labels.try_emplace(label, FixedBitset(std::size(nodes)));
      emplaced->second.insert(index);
    } else {
      iter->second.insert(index);
    }
  }

  auto graph = reserved<graph_t>(std::size(nodes));
  for (const auto& [index, node] : nodes | views::enumerate) {
    auto& dependencies = graph[index];
    for (const auto& label : node.ordering.after) {
      if (const auto iter = labels.find(label); iter != std::end(labels)) {
        const auto& new_dependency = iter->second;
        for (const auto& dependency : new_dependency.ones()) {
          dependencies[dependency].insert(label);
        }
      } else {
        throw nova_exception{std::format(
            "unable to find label `{}` while building dependency graph",
            label.name)};
      }
    }

    for (const auto& label : node.ordering.before) {
      if (const auto iter = labels.find(label); iter != std::end(labels)) {
        const auto& dependants = iter->second;
        for (const auto& dependant : dependants.ones()) {
          graph[dependant][index].insert(label);
        }
      } else {
        throw nova_exception{std::format(
            "unable to find label `{}` while building dependency graph",
            label.name)};
      }
    }
  }

  return graph;
}

struct GraphCyclesError {
  std::vector<std::size_t> cycle{};
};

namespace detail {

inline auto check_if_cycles_and_visit(const std::size_t node,
                                      const graph_t& graph,
                                      std::vector<std::size_t>& sorted,
                                      hash::hash_set_t<std::size_t>& unvisited,
                                      std::vector<std::size_t>& current)
    -> bool {
  if (std::ranges::any_of(current, equals(node))) {
    return true;
  } else if (unvisited.erase(node) == 0u) {
    return false;
  }
  current.push_back(node);
  for (const auto& dependency : graph.at(node) | std::ranges::views::keys) {
    if (check_if_cycles_and_visit(dependency, graph, sorted, unvisited,
                                  current)) {
      return true;
    }
  }
  sorted.push_back(node);
  current.pop_back();
  return false;
};

}  // namespace detail

inline auto topological_order(const graph_t& graph)
    -> tl::expected<std::vector<std::size_t>, GraphCyclesError> {
  namespace views = ::ranges::views;
  using vec_t = std::vector<std::size_t>;

  auto sorted = reserved<vec_t>(std::size(graph));
  auto current = reserved<vec_t>(std::size(graph));
  auto unvisited = graph | std::ranges::views::keys |
                   ranges::to<hash::hash_set_t<std::size_t>>;

  while (not std::empty(unvisited)) {
    const auto node = *std::begin(unvisited);
    if (detail::check_if_cycles_and_visit(node, graph, sorted, unvisited,
                                          current)) {
      /*
      // TODO: chain this to the end of the view
      // make this an iterator instead of adding another element
      //  current.push_back(current.at(0));
      auto cycle = reserved<typename
      GraphCyclesError::cycles_t>(std::size(current) / 2u); for (const auto
      [dependant, dependency] : views::zip(current, current | views::drop(1))) {
          cycle.push_back(std::pair { dependant,
      graph.at(dependant).at(dependency) | ranges::to<std::vector> });
      }

      const auto dependant = current.back();
      const auto dependency = current.front();
      auto first = dependant;
      auto second = [&] {
          const auto& a = graph.at(dependant);
          const auto& b = a.at(dependency);
          return b | ranges::to<std::vector>;
      }();
      cycle.push_back(std::pair { first, MOV(second) });

      return tl::make_unexpected(
          GraphCyclesError { .cycles = MOV(cycle) });
      */
      if (not std::empty(current)) [[likely]] {
        current.push_back(current.front());
      }
      return tl::make_unexpected(GraphCyclesError{.cycle = MOV(current)});
    }
  }

  return sorted;
}

}  // namespace nova
