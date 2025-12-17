#pragma once

#include <vector>

#include "common/types.h"
#include "pex_property.h"
#include "pex_state.h"
#include "pex_string.h"
#include "pex_user_flag.h"
#include "pex_variable.h"

namespace vellum {
namespace pex {
using common::Vec;

class PexWriter;

class PexObject final {
 public:
  PexString getName() const { return name; }
  void setName(PexString name_) { name = name_; }

  PexString getParentName() const { return parentName; }
  void setParentName(PexString parentName_) { parentName = parentName_; }

  // TODO: add object size calc
  uint32_t getObjectSize() const { return 0; }
  PexUserFlags getUserFlags() const { return PexUserFlags(); }

  PexString getDocumentationString() const { return documentationString; }
  void setDocumentationString(PexString docString) {
    documentationString = docString;
  }

  PexString getAutoStateName() const { return autoStateName; }
  void setAutoStateName(PexString name) { autoStateName = name; }

  const Vec<PexVariable>& getVariables() const { return variables; }
  Vec<PexVariable>& getVariables() { return variables; }

  const Vec<PexProperty>& getProperties() const { return properties; }
  Vec<PexProperty>& getProperties() { return properties; }

  const Vec<PexState>& getStates() const { return states; }
  Vec<PexState>& getStates() { return states; }

  const PexState& getRootState() const {
    assert(!states.empty());
    return states[0];
  }

  PexState& getRootState() {
    assert(!states.empty());
    return states[0];
  }

 private:
  PexString name;
  PexString parentName;
  PexString documentationString;
  PexString autoStateName;

  Vec<PexVariable> variables;
  Vec<PexProperty> properties;
  Vec<PexState> states;
};

PexWriter& operator<<(PexWriter& writer, const PexObject& object);

}  // namespace pex
}  // namespace vellum