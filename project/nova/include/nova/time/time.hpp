#pragma once

#include <chrono>
#include <tl/optional.hpp>

namespace nova {

class Time {
 public:
  using clock_t = std::chrono::steady_clock;
  using duration_t = std::chrono::duration<double>;
  using time_point_t = std::chrono::time_point<clock_t, duration_t>;

 private:
  duration_t delta_{};
  tl::optional<time_point_t> last_update_{};
  duration_t time_since_startup_{};
  time_point_t startup_{clock_t::now()};

 public:
  constexpr explicit(true) Time(time_point_t startup = clock_t::now()) noexcept
      : startup_(startup) {}

  auto update_with_instant(time_point_t now) -> void {
    if (last_update_.has_value()) [[likely]] {
      delta_ = now - *last_update_;
    }

    time_since_startup_ = now - startup_;
    last_update_ = now;
  }

  auto update() -> void { update_with_instant(clock_t::now()); }

  [[nodiscard]] constexpr auto delta() const -> duration_t { return delta_; }

  template <typename T = double>
    requires(std::convertible_to<typename duration_t::rep, T>)
  [[nodiscard]] constexpr auto delta_seconds() const -> T {
    return static_cast<T>(delta_.count());
  }

  [[nodiscard]] constexpr auto time_since_startup() const -> duration_t {
    return time_since_startup_;
  }

  [[nodiscard]] constexpr auto last_update() const
      -> tl::optional<time_point_t> {
    return last_update_;
  }
};

}  // namespace nova
