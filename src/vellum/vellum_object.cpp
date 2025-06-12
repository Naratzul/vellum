#include "vellum_object.h"
namespace vellum {

std::optional<VellumValue> VellumObject::findIdentifier(
    VellumIdentifier identifier) const {
  if (auto member = findVariable(identifier)) {
    return member;
  }

  return findProperty(identifier);
}

std::optional<VellumValue> VellumObject::findProperty(
    VellumIdentifier identifier) const {
  for (const auto& member : properties) {
    if (member.getName() == identifier) {
      return member;
    }
  }

  return findFunction(identifier);
}

std::optional<VellumFunction> VellumObject::findFunction(
    VellumIdentifier identifier) const {
  for (const auto& member : functions) {
    if (member.getName() == identifier) {
      return member;
    }
  }

  return std::nullopt;
}

std::optional<VellumVariable> VellumObject::findVariable(
    VellumIdentifier identifier) const {
  for (const auto& member : variables) {
    if (member.getName() == identifier) {
      return member;
    }
  }

  return std::nullopt;
}

}  // namespace vellum