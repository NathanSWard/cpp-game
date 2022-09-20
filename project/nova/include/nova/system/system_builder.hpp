#pragma once

#include <nova/label/builder.hpp>
#include <nova/label/label.hpp>
#include <range/v3/view/move.hpp>
#include <range/v3/view/zip.hpp>

#include "system.hpp"

namespace nova {

namespace {
struct system_tag_t {};
}  // namespace

class system_builder : public builder_base<system_builder> {
 private:
  System system_;
  Access access_;

  friend struct into_system_descriptors<system_builder>;

 public:
  template <concepts::system_like T>
  constexpr system_builder(system_tag_t, T&& system)
      : system_{detail::create_system(FWD(system))},
        access_(detail::get_system_access<T>()) {}
};

template <>
struct into_system_descriptors<system_builder> {
  template <typename TSystemBuilder>
  auto operator()(TSystemBuilder&& builder) {
    static_assert(
        std::is_same_v<system_builder, std::remove_cvref_t<TSystemBuilder>>);
    return SystemDescriptor{
        .system = std::exchange(builder.system_, System{}),
        .labels = std::exchange(builder.labels_, Labels{}),
        .access = std::exchange(builder.access_, Access{}),
        .ordering = std::exchange(builder.ordering_, Ordering{}),
    };
  }
};

template <concepts::system_like TSystem>
constexpr auto system(TSystem&& system) {
  return system_builder{system_tag_t{}, FWD(system)};
}

class system_set : public builder_base<system_set> {
 private:
  std::vector<System> systems_{};
  std::vector<Access> access_{};

  friend struct into_system_descriptors<system_set>;

 public:
  template <typename TSystem>
  auto with_system(TSystem&& system) & -> auto& {
    systems_.push_back(detail::create_system(FWD(system)));
    access_.push_back(detail::get_system_access<TSystem>());
    return *this;
  }
};

template <>
struct into_system_descriptors<system_set> {
  auto operator()(auto&& set) -> std::vector<SystemDescriptor> {
    static_assert(
        std::is_same_v<system_set, std::remove_cvref_t<decltype(set)>>);
    static_assert(std::is_rvalue_reference_v<decltype(set)>,
                  "SystemDescriptor can only be constructed from a "
                  "system_set&&. Consider using std::move(set).");

    auto descriptors =
        reserved<std::vector<SystemDescriptor>>(std::size(set.systems_));
    for (auto&& [system, access] :
         ranges::views::zip(set.systems_, set.access_) | ranges::views::move) {
      descriptors.push_back(SystemDescriptor{
          .system = MOV(system),
          .labels = set.labels_,
          .access = MOV(access),
          .ordering = set.ordering_,
      });
    }
    return descriptors;
  }
};

template <>
struct into_system_descriptors<SystemDescriptor> {
  auto operator()(SystemDescriptor&& descriptor)
      -> std::vector<SystemDescriptor> {
    auto descriptors = reserved<std::vector<SystemDescriptor>>(1u);
    descriptors.push_back(MOV(descriptor));
    return descriptors;
  }
};

}  // namespace nova
