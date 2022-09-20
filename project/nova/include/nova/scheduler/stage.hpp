#pragma once

#include <nova/label/builder.hpp>
#include <nova/label/label.hpp>
#include <nova/system/system_data.hpp>
#include <nova/util/common.hpp>
#include <type_traits>
#include <utility>

namespace nova {

template <typename T>
struct into_stage;

struct StageMeta {
  Label primary_label{};
  Labels labels{};
  Ordering ordering{};
};

namespace concepts {

template <typename T>
concept into_stage = requires(T&& t) {
                       {
                         nova::into_stage<T>{}(FWD(t))
                         } -> std::same_as<StageMeta>;
                     };

}  // namespace concepts

template <concepts::into_stage TStage>
constexpr auto to_stage(TStage&& stage = {}) -> StageMeta {
  return into_stage<std::remove_cvref_t<TStage>>{}(FWD(stage));
}

namespace {
struct stage_builder_tag_t {
  constexpr stage_builder_tag_t() {}
};
}  // namespace

struct stage_builder : public builder_base<stage_builder> {
 private:
  Label primary_label_{};

  friend struct into_stage<stage_builder>;

 public:
  explicit(false) stage_builder(stage_builder_tag_t, Label primary_label)
      : primary_label_(MOV(primary_label)) {
    labels_.push_back(primary_label_);
  }
};

template <concepts::into_label TStageLabel>
constexpr auto stage(TStageLabel&& label = {}) -> stage_builder {
  return stage_builder{stage_builder_tag_t{}, to_label(FWD(label))};
}

template <>
struct into_stage<stage_builder> {
  constexpr auto operator()(stage_builder&& builder) -> StageMeta {
    return StageMeta{
        .primary_label = FWD(builder).primary_label_,
        .labels = FWD(builder).labels_,
        .ordering = FWD(builder).ordering_,
    };
  }
};

template <typename T>
  requires concepts::into_label<T>
struct into_stage<T> {
  constexpr auto operator()(auto&& label) -> StageMeta {
    static_assert(std::is_same_v<T, std::remove_cvref_t<decltype(label)>>);
    return into_stage<stage_builder>{}(nova::stage(FWD(label)));
  }
};

}  // namespace nova
