#pragma once

#include <stdint.h>

#include <cassert>

#include "pex_string.h"

namespace vellum {
namespace pex {

enum class PexValueType : uint8_t {
  None = 0,
  Identifier = 1,
  String = 2,
  Integer = 3,
  Float = 4,
  Bool = 5,
  Label = 20,
  TemporaryVar = 21,
  Invalid = 50
};

class PexValue {
 public:
  PexValue() = default;
  PexValue(PexString value, PexValueType type = PexValueType::String) : type(type), value(value) {}
  PexValue(int32_t value) : type(PexValueType::Integer), value(value) {}
  PexValue(float value) : type(PexValueType::Float), value(value) {}
  PexValue(bool value) : type(PexValueType::Bool), value(value) {}

  PexValueType getType() const { return type; }

  PexString asString() const {
    //assert(type == PexValueType::String);
    return value.string;
  }

  int32_t asInt() const {
    assert(type == PexValueType::Integer);
    return value.intNum;
  }

  float asFloat() const {
    assert(type == PexValueType::Float);
    return value.floatNum;
  }

  bool asBool() const {
    assert(type == PexValueType::Bool);
    return value.boolean;
  }

 private:
  PexValueType type = PexValueType::None;

  union Value {
    PexString string;
    int32_t intNum;
    float floatNum;
    bool boolean;

    Value(){};
    ~Value(){};

    Value(PexString value) : string(value) {}
    Value(int32_t value) : intNum(value) {}
    Value(float value) : floatNum(value) {}
  } value;
};
}  // namespace pex
}  // namespace vellum