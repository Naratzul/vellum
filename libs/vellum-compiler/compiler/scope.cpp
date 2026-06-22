#include "scope.h"

#include "common/types.h"

namespace vellum {
using common::Opt;
using common::Vec;

void Scope::pushVar(const VellumVariable var) { variables.push_back(var); }

void Scope::pushVar(const Vec<VellumVariable>& vars) {
  variables.insert(variables.end(), vars.begin(), vars.end());
}

void Scope::popVar() { variables.pop_back(); }

void Scope::popVar(int count) {
  variables.erase(variables.end() - count, variables.end());
}

bool Scope::contains(VellumIdentifier name) {
  for (const auto& var : variables) {
    if (var.getName() == name) {
      return true;
    }
  }
  return false;
}

Opt<VellumVariable> Scope::getVariable(VellumIdentifier name) const {
  for (const auto& var : variables) {
    if (var.getName() == name) {
      return var;
    }
  }
  return std::nullopt;
}

}  // namespace vellum