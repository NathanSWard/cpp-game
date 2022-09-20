#pragma once

#include <type_traits>
#include <utility>

#include "common.hpp"

namespace nova {

namespace detail {

template <typename T>
constexpr void delete_data(void* const ptr) {
  delete static_cast<T*>(ptr);
}

}  // namespace detail

class void_ptr {
  using deleter_t = void (*)(void*);
  void* data_ = nullptr;
  deleter_t deleter_ = nullptr;

  constexpr void destroy() {
    if (data_) {
      deleter_(data_);
    }
  }

 public:
  constexpr void_ptr() noexcept = default;

  template <typename T, typename... Args>
  constexpr void_ptr(std::in_place_type_t<T>, Args&&... args)
      : data_(new T(FWD(args)...)), deleter_(detail::delete_data<T>) {}

  template <typename T, typename... Args>
  [[nodiscard]] static constexpr auto create(Args&&... args) noexcept
      -> void_ptr {
    return void_ptr(std::in_place_type<T>, FWD(args)...);
  }

  [[nodiscard]] constexpr auto take() noexcept -> void* {
    auto const ptr = std::exchange(data_, nullptr);
    deleter_ = nullptr;
    return ptr;
  }

  constexpr auto data() noexcept -> void* { return data_; }
  constexpr auto data() const noexcept -> void const* { return data_; }
  constexpr auto cdata() const noexcept -> void const* { return data_; }

  constexpr void_ptr(void_ptr&& other) noexcept
      : data_(std::exchange(other.data_, nullptr)),
        deleter_(std::exchange(other.deleter_, nullptr)) {}

  constexpr void_ptr& operator=(void_ptr&& other) noexcept {
    destroy();
    data_ = std::exchange(other.data_, nullptr);
    deleter_ = std::exchange(other.deleter_, nullptr);
    return *this;
  }

  void_ptr(void_ptr const&) = delete;
  void_ptr& operator=(void_ptr const&) = delete;

  ~void_ptr() { destroy(); }
};

}  // namespace nova
