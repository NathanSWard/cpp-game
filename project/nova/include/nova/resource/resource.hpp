#pragma once

#include <functional>
#include <memory>
#include <nova/util/common.hpp>
#include <nova/util/type_map.hpp>
#include <numeric>
#include <tl/optional.hpp>

namespace nova {

namespace detail {

template <typename T>
class ResourceBase {
  static_assert(not std::is_reference_v<T>,
                "Resources cannot be a reference type");
  T* ptr_ = nullptr;

 public:
  constexpr ResourceBase(T& value) noexcept : ptr_(std::addressof(value)) {}

  constexpr ResourceBase(ResourceBase&&) noexcept = default;
  constexpr ResourceBase(ResourceBase const&) noexcept = default;
  constexpr ResourceBase& operator=(ResourceBase&&) noexcept = default;
  constexpr ResourceBase& operator=(ResourceBase const&) noexcept = default;

  [[nodiscard]] constexpr auto operator*() noexcept -> T& { return *ptr_; }

  [[nodiscard]] constexpr auto operator*() const noexcept
      -> std::add_const_t<T>& {
    return *ptr_;
  }

  [[nodiscard]] constexpr auto operator->() noexcept -> T* { return ptr_; }

  [[nodiscard]] constexpr auto operator->() const noexcept
      -> std::add_const_t<T>* {
    return ptr_;
  }
};

template <typename T>
ResourceBase(T&) -> ResourceBase<T>;

}  // namespace detail

template <typename T>
struct Resource : detail::ResourceBase<T> {
  using detail::ResourceBase<T>::ResourceBase;
};
template <typename T>
struct Local : detail::ResourceBase<T> {
  static_assert(not std::is_const_v<T>,
                "Local<T>: T cannot be const, as it's local to the system.");
  using detail::ResourceBase<T>::ResourceBase;
};

class Resources {
  TypeMap resources_;

 public:
  // Resources
  template <typename T, typename... Args>
  auto try_add(Args&&... args) -> std::pair<Resource<T>, bool> {
    static_assert(not std::is_reference_v<T>,
                  "resources cannot be reference types.");
    auto [resource, inserted] =
        resources_.try_add<std::remove_const_t<T>>(FWD(args)...);
    return std::pair{Resource{resource}, inserted};
  }

  template <typename T, typename... Args>
  auto set(Args&&... args) -> Resource<T> {
    static_assert(not std::is_reference_v<T>,
                  "resources cannot be reference types.");
    T& resource = resources_.set<std::remove_const_t<T>>(FWD(args)...);
    return Resource{resource};
  }

  template <typename T>
  auto remove() -> tl::optional<T> {
    return resources_.remove<T>();
  }

  template <typename T>
  [[nodiscard]] auto contains() const -> bool {
    return resources_.contains<std::remove_cvref_t<T>>();
  }

  template <typename T>
  [[nodiscard]] auto get() -> tl::optional<Resource<T>> {
    static_assert(not std::is_reference_v<T>,
                  "resources cannot be reference types.");
    return resources_.get<std::remove_const_t<T>>().map(
        [](T& value) { return Resource{value}; });
  }

  template <typename T>
  [[nodiscard]] auto get() const
      -> tl::optional<Resource<std::add_const_t<T>>> {
    static_assert(not std::is_reference_v<T>,
                  "resources cannot be reference types.");
    return resources_.get<std::remove_const_t<T>>().map(
        [](T const& value) { return Resource(value); });
  }

  template <typename T>
  [[nodiscard]] auto cget() const -> tl::optional<Resource<T const>> {
    return get<T>();
  }

  auto clear() -> void { resources_.clear(); }

  [[nodiscard]] auto size() const noexcept -> std::size_t {
    return std::size(resources_);
  }
};

}  // namespace nova
