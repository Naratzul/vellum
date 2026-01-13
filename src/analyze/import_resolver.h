#pragma once

#include "common/types.h"
#include "vellum/vellum_object.h"

namespace vellum {
using common::Opt;
using common::Vec;

class ImportResolver {
 public:
  bool hasObject(VellumIdentifier name) const;

  const VellumObject& getObject(VellumIdentifier name) const;

  VellumObject* importObject(VellumIdentifier name);

 private:
  Vec<VellumObject> objects;
};
}  // namespace vellum