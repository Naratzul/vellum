#pragma once

#include <optional>
#include <vector>

#include "vellum/vellum_variable.h"

namespace vellum {

class Scope {
 public:
  void pushVar(const VellumVariable var);
  void pushVar(const std::vector<VellumVariable>& vars);

  void popVar();
  void popVar(int count);

  bool contains(VellumIdentifier name);
  std::optional<VellumVariable> getVariable(VellumIdentifier name) const;

 private:
  std::vector<VellumVariable> variables;
};
}  // namespace vellum