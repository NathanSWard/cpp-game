#pragma once

#include <exception>
#include <format>
#include <nova/util/common.hpp>
#include <source_location>
#include <sstream>
#include <string_view>
#include <thread>
#include <tuple>

namespace nova {

struct panic_exception : std::exception {
  std::string message{};
  panic_exception(std::string message) noexcept : message(MOV(message)) {}
  auto what() const noexcept -> const char* override {
    return std::data(message);
  }
};

#define PANIC(fmt, ...)                                                      \
  [&, loc = std::source_location::current()] {                               \
    const auto message = std::format(fmt, __VA_ARGS__);                      \
    std::stringstream ss;                                                    \
    ss << std::this_thread::get_id();                                        \
    auto panic_message = std::format(                                        \
        "thread '{}' panicked at '{}', {}:{}:{}, '{}'\n", ss.str(), message, \
        loc.file_name(), loc.line(), loc.column(), loc.function_name());     \
    throw panic_exception{MOV(panic_message)};                               \
  }()

#define NOVA_ASSERT(x, fmt, ...)                 \
  [&] {                                          \
    if (not static_cast<bool>(x)) [[unlikely]] { \
      PANIC(fmt, __VA_ARGS__);                   \
    }                                            \
  }()

#ifndef NDEBUG
#define IS_DEBUG
#endif

// DEBUG_ASSERT
#ifdef IS_DEBUG
#define DEBUG_ASSERT(x, fmt, ...) NOVA_ASSERT(x, fmt, __VA_ARGS__)
#else
#define DEBUG_ASSERT(...)
#endif

}  // namespace nova
