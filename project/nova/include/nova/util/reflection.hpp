#pragma once

#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <type_traits>
#include <utility>

#include "common.hpp"
#include "meta.hpp"

namespace nova {

namespace concepts {
template <typename T>
concept aggregate = std::is_aggregate_v<T>;
}

namespace reflect {

namespace detail {

template <typename T, typename... TArgs>
auto is_aggregate_constructable_impl(std::tuple<TArgs...>)
    -> decltype(T{std::declval<TArgs>()...});

template <typename T, typename TArgs, typename = void>
struct is_aggregate_constructable : std::false_type {};

template <typename T, typename TArgs>
struct is_aggregate_constructable<
    T, TArgs,
    std::void_t<decltype(is_aggregate_constructable_impl<T>(
        std::declval<TArgs>()))>> : std::true_type {};

// Checks if type can be initialized from braced-init-list.
template <typename T, typename TArgs>
constexpr auto is_aggregate_constructable_v =
    is_aggregate_constructable<T, TArgs>::value;

// Class is convertible to anything.
struct any_type {
  template <typename T>
  operator T() const;
};

template <class T, typename... TArgs>
constexpr auto num_bindings_impl_recurse() -> std::size_t {
  if constexpr (detail::is_aggregate_constructable<T, std::tuple<TArgs...>>()) {
    return num_bindings_impl_recurse<T, any_type, TArgs...>();
  } else {
    return sizeof...(TArgs) - 1u;
  }
}

template <class T>
constexpr auto num_bindings_impl() -> std::size_t {
  if constexpr (std::is_empty_v<T>) {
    return 0;
  } else {
    return num_bindings_impl_recurse<T, any_type>();
  }
}

template <concepts::aggregate T>
inline constexpr auto struct_bind_num_v = num_bindings_impl<T>();

#ifndef NOVA_MAX_REFLECTION_DEPTH
#define NOVA_MAX_REFLECTION_DEPTH 16
#endif

#define VALUE_NAME(N) val_##N
#define VALUES(Z, N, _) BOOST_PP_COMMA_IF(N) VALUE_NAME(N)
#define DECLTYPE_VALUES(Z, N, _) \
  BOOST_PP_COMMA_IF(N) std::remove_cvref_t<decltype(VALUE_NAME(N))>
#define EXPAND_STRUCTURED_BINDING(Z, N, value) \
  [[maybe_unused]] auto&& [BOOST_PP_REPEAT_##Z(N, VALUES, _)] = FWD(value);
#define ELSE_BRANCH(T)                                                 \
  else {                                                               \
    static_assert(                                                     \
        always_false<T>{},                                             \
        "nova max reflection depth exceeded. Consider increasing the " \
        "limit by `#define NOVA_MAX_REFLECTION_DEPTH`");               \
  }

template <concepts::aggregate TReflect>
constexpr auto member_type_list_impl([[maybe_unused]] const TReflect& value) {
  constexpr auto n_bindings = struct_bind_num_v<TReflect>;
  if constexpr (n_bindings == 0) {
    return nova::type_list{};
  }

  /* clang-format off */
#define MAKE_TYPE_LIST(Z, N, _)                                \
  else if constexpr (n_bindings == BOOST_PP_INC(N)) {          \
    EXPAND_STRUCTURED_BINDING(Z, BOOST_PP_INC(N), value)       \
    return nova::type_list<                                    \
      BOOST_PP_REPEAT_##Z(BOOST_PP_INC(N), DECLTYPE_VALUES, _) \
    >{};                                                       \
  }
  /* clang-format on */

  BOOST_PP_REPEAT(NOVA_MAX_REFLECTION_DEPTH, MAKE_TYPE_LIST, _)

#undef MAKE_TYPE_LIST

  ELSE_BRANCH(TReflect)
}

}  // namespace detail

template <typename T>
using member_type_list = decltype(detail::member_type_list_impl(
    std::declval<std::remove_cvref_t<T>>()));

template <typename TReflect>
inline constexpr auto member_count = detail::struct_bind_num_v<TReflect>;

template <concepts::aggregate TReflect>
[[nodiscard]] constexpr auto get_member_references(
    [[maybe_unused]] TReflect& value) {
  constexpr auto n_bindings = member_count<std::remove_const_t<TReflect>>;
  if constexpr (n_bindings == 0) {
    return std::tuple{};
  }

  /* clang-format off */
#define MAKE_TUPLE_REF(Z, N, _)                          \
  else if constexpr (n_bindings == BOOST_PP_INC(N)) {    \
    EXPAND_STRUCTURED_BINDING(Z, BOOST_PP_INC(N), value) \
    return std::forward_as_tuple(                        \
        BOOST_PP_REPEAT_##Z(BOOST_PP_INC(N), VALUES, _)  \
    );                                                   \
  }
  /* clang-format on */

  BOOST_PP_REPEAT(NOVA_MAX_REFLECTION_DEPTH, MAKE_TUPLE_REF, _)

#undef MAKE_TUPLE_REF

  ELSE_BRANCH(TReflect)
}

template <typename TReflect>
requires(concepts::aggregate<std::remove_cvref_t<
             TReflect>>) constexpr auto for_each(TReflect&& value, auto func)
    -> void {
  constexpr auto n_bindings = member_count<std::remove_cvref_t<TReflect>>;
  constexpr auto should_move = std::is_rvalue_reference_v<decltype(value)>;
  if constexpr (n_bindings == 0) {
    return;
  }

#define INVOKE_FOR_EACH(_, N, func) \
  if constexpr (should_move) {      \
    func(MOV(VALUE_NAME(N)));       \
  } else {                          \
    func(VALUE_NAME(N));            \
  }

  /* clang-format off */
#define CALL_FUNC_ON_VALUES(Z, N, _)                            \
  else if constexpr (n_bindings == BOOST_PP_INC(N)) {           \
    EXPAND_STRUCTURED_BINDING(Z, BOOST_PP_INC(N), value)        \
    BOOST_PP_REPEAT(BOOST_PP_INC(N), INVOKE_FOR_EACH, func)     \
    return;                                                     \
  }
  /* clang-format on */

  BOOST_PP_REPEAT(NOVA_MAX_REFLECTION_DEPTH, CALL_FUNC_ON_VALUES, _)

#undef CALL_FUNC_ON_VALUES
#undef INVOKE_FOR_EACH

  ELSE_BRANCH(TReflect)
}

template <concepts::aggregate TReflect>
constexpr auto for_each_type(auto func) -> void {
  [&]<typename... TMembers>(type_list<TMembers...>) {
    (func(std::type_identity<TMembers>{}), ...);
  }(member_type_list<TReflect>{});
}

}  // namespace reflect
}  // namespace nova

#undef ELSE_BRANCH
#undef EXPAND_STRUCTURED_BINDING
#undef DECLTYPE_VALUES
#undef VALUES
#undef VALUE_NAME