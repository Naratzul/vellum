#pragma once
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace vellum {
namespace common {
template <typename T>
using Unique = std::unique_ptr<T>;
template <typename T>
using Shared = std::shared_ptr<T>;
template <typename T>
using Opt = std::optional<T>;
template <typename T>
using Vec = std::vector<T>;
template <typename T>
using Set = std::unordered_set<T>;
template <typename K, typename V>
using Map = std::unordered_map<K, V>;

template <typename Type, typename... Args>
Unique<Type> makeUnique(Args&&... args) {
  return std::make_unique<Type>(std::forward<Args>(args)...);
}

template <typename Type, typename... Args>
Shared<Type> makeShared(Args&&... args) {
  return std::make_shared<Type>(std::forward<Args>(args)...);
}
}  // namespace common
}  // namespace vellum