#pragma once

#include <nova/system/system_data.hpp>

#include "label.hpp"

namespace nova {

template <class TBase>
struct builder_base {
 protected:
  Labels labels_{};
  Ordering ordering_{};

 public:
  // lvalue-ref overloads
  template <concepts::into_label TLabel>
  constexpr auto before(TLabel&& label = {}) & -> auto& {
    this->ordering_.before.push_back(nova::to_label(FWD(label)));
    return static_cast<TBase&>(*this);
  }
  template <concepts::into_label TLabel>
  constexpr auto after(TLabel&& label = {}) & -> auto& {
    this->ordering_.after.push_back(nova::to_label(FWD(label)));
    return static_cast<TBase&>(*this);
  }
  template <concepts::into_label TLabel>
  constexpr auto label(TLabel&& label = {}) & -> auto& {
    this->labels_.push_back(nova::to_label(FWD(label)));
    return static_cast<TBase&>(*this);
  }

  // rvalue-ref overloads
  template <concepts::into_label TLabel>
  constexpr auto before(TLabel&& label = {}) && -> auto&& {
    this->ordering_.before.push_back(nova::to_label(FWD(label)));
    return static_cast<TBase&&>(*this);
  }
  template <concepts::into_label TLabel>
  constexpr auto after(TLabel&& label = {}) && -> auto&& {
    this->ordering_.after.push_back(nova::to_label(FWD(label)));
    return static_cast<TBase&&>(*this);
  }
  template <concepts::into_label TLabel>
  constexpr auto label(TLabel&& label = {}) && -> auto&& {
    this->labels_.push_back(nova::to_label(FWD(label)));
    return static_cast<TBase&&>(*this);
  }
};

}  // namespace nova