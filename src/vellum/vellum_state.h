#pragma once

#include "common/types.h"
#include "vellum/vellum_identifier.h"
#include "vellum_function.h"

namespace vellum {
using common::Vec;

class VellumState {
 public:
  explicit VellumState(VellumIdentifier name) : name(name) {}

  VellumIdentifier getName() const { return name; }

  const Vec<VellumFunction>& getFunctions() const { return functions; }
  void addFunction(const VellumFunction& func) { functions.push_back(func); }

  Opt<VellumFunction> getFunction(VellumIdentifier name) const {
    for (const auto& func : functions) {
      if (func.getName() == name) {
        return func;
      }
    }
    return std::nullopt;
  }

 private:
  VellumIdentifier name;
  Vec<VellumFunction> functions;
};
}  // namespace vellum