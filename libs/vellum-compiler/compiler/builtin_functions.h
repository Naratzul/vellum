#pragma once

#include "common/types.h"
#include "vellum/vellum_function.h"
#include "vellum/vellum_identifier.h"

namespace vellum {
using common::Opt;
using common::Vec;

class BuiltinFunctions {
 public:
  BuiltinFunctions();

  void addFunction(VellumFunction func);
  Opt<VellumFunction> getFunction(VellumIdentifier name) const;

 private:
  Vec<VellumFunction> functions;
};
}  // namespace vellum