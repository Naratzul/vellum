#pragma once

#include <optional>
#include <vector>

#include "common/types.h"
#include "vellum/vellum_variable.h"

namespace vellum {
using common::Opt;
using common::Vec;

class Scope {
 public:
  void pushVar(const VellumVariable var);
  void pushVar(const Vec<VellumVariable>& vars);

  void popVar();
  void popVar(int count);

  bool contains(VellumIdentifier name);
  Opt<VellumVariable> getVariable(VellumIdentifier name) const;

 private:
  Vec<VellumVariable> variables;
};
}  // namespace vellum