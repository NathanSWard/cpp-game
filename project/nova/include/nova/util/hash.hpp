#pragma once

#include <unordered_map>
#include <unordered_set>

namespace nova::hash {

template <typename K, typename V>
using hash_map_t = std::unordered_map<K, V>;

template <typename T>
using hash_set_t = std::unordered_set<T>;

}  // namespace nova::hash