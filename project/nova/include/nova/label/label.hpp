#pragma once

#include <concepts>
#include <entt/entt.hpp>
#include <nova/util/common.hpp>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace nova {

using label_id_t = entt::id_type;

struct LabelRef {
  label_id_t id;
  std::string_view name;

  constexpr auto operator==(const LabelRef&) const noexcept -> bool = default;
  constexpr auto operator<=>(const LabelRef&) const noexcept = default;
};

struct Label {
  label_id_t id{};
  std::string name{};

  constexpr explicit(false) operator LabelRef() const noexcept {
    return LabelRef{
        .id = id,
        .name = name,
    };
  }

  constexpr auto operator==(const Label&) const noexcept -> bool = default;
  constexpr auto operator<=>(const Label&) const noexcept = default;
};

using Labels = std::vector<Label>;

template <typename T>
struct into_label;
template <typename T>
struct into_label_ref;

namespace concepts {
template <typename T>
concept into_label = requires(T&& t) {
  { nova::into_label<T>{}(FWD(t)) } -> std::same_as<Label>;
};

template <typename T>
concept into_label_ref = requires(T&& t) {
  { nova::into_label_ref<T>{}(FWD(t)) } -> std::same_as<LabelRef>;
};
}  // namespace concepts

template <concepts::into_label_ref TLhs, concepts::into_label_ref TRhs>
constexpr auto operator==(const TLhs& lhs, const TRhs& rhs) -> bool {
  const auto lhs_ref = into_label_ref<TLhs>{}(lhs);
  const auto rhs_ref = into_label_ref<TRhs>{}(rhs);
  return lhs_ref == rhs_ref;
}

template <concepts::into_label_ref TLhs, concepts::into_label_ref TRhs>
constexpr auto operator<=>(const TLhs& lhs, const TRhs& rhs) {
  const auto lhs_ref = into_label_ref<TLhs>{}(lhs);
  const auto rhs_ref = into_label_ref<TRhs>{}(rhs);
  return lhs_ref <=> rhs_ref;
}

template <>
struct into_label_ref<Label> {
  constexpr auto operator()(auto&& label) -> LabelRef {
    static_assert(std::is_same_v<Label, std::remove_cvref_t<decltype(label)>>);
    static_assert(not std::is_rvalue_reference_v<decltype(label)>,
                  "LabelRef cannot be created from a Label&&");
    return LabelRef{
        .id = FWD(label).id,
        .name = FWD(label).name,
    };
  }
};

template <>
struct into_label_ref<LabelRef> {
  constexpr auto operator()(LabelRef ref) -> LabelRef { return ref; }
};

template <std::convertible_to<std::string_view> T>
struct into_label_ref<T> {
  constexpr auto operator()(auto&& value) -> LabelRef {
    static_assert(std::is_same_v<T, std::remove_cvref_t<decltype(value)>>);
    const auto name = static_cast<std::string_view>(FWD(value));
    return LabelRef{
        .id = entt::hashed_string{std::data(name), std::size(name)},
        .name = name,
    };
  }
};

template <typename T>
requires std::is_empty_v<T>
struct into_label_ref<T> {
  constexpr auto operator()(T) const noexcept -> LabelRef {
    constexpr auto name = type_name<T>();
    return LabelRef{
        .id = entt::hashed_string{std::data(name), std::size(name)},
        .name = name,
    };
  }
};

template <typename T>
requires std::constructible_from<std::string, T>
struct into_label<T> {
  constexpr auto operator()(auto&& value) const noexcept -> Label {
    static_assert(std::is_same_v<T, std::remove_cvref_t<decltype(value)>>);
    auto name = std::string(FWD(value));
    return Label{
        .id = entt::hashed_string{std::data(name), std::size(name)},
        .name = MOV(name),
    };
  }
};

template <typename T>
requires(std::is_empty_v<T>) struct into_label<T> {
  constexpr auto operator()(T) const noexcept -> Label {
    constexpr auto name = type_name<T>();
    return Label{
        .id = entt::hashed_string{std::data(name), std::size(name)},
        .name = std::string{name},
    };
  }
};

template <concepts::into_label TLabel>
constexpr auto to_label(TLabel&& label = {}) -> Label {
  return into_label<std::remove_cvref_t<TLabel>>{}(FWD(label));
}

template <concepts::into_label_ref TLabelRef>
constexpr auto to_label_ref(TLabelRef&& ref = {}) -> LabelRef {
  return into_label_ref<std::remove_cvref_t<TLabelRef>>{}(FWD(ref));
}

}  // namespace nova
