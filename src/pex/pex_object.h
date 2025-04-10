#pragma once

#include <vector>

#include "pex_string.h"
#include "pex_writer.h"

namespace vellum {
namespace pex {

class PexVariable {};

class PexProperty {};

class PexState {};

class PexObject {
 public:
  PexString getName() const { return name; }
  void setName(PexString name_) { name = name_; }

  PexString getParentName() const { return parentName; }
  void setParentName(PexString parentName_) { parentName = parentName_; }

  // TODO: add object size calc
  uint32_t getObjectSize() const { return 0; }
  uint32_t getUserFlags() const { return 0; }

  PexString getDocumentationString() const { return documentationString; }
  void setDocumentationString(PexString docString) {
    documentationString = docString;
  }

  PexString getAutoStateName() const { return autoStateName; }
  void setAutoStateName(PexString name) { autoStateName = name; }

  const std::vector<PexVariable>& getVariables() const { return variables; }
  std::vector<PexVariable>& getVariables() { return variables; }

  const std::vector<PexProperty>& getProperties() const { return properties; }
  std::vector<PexProperty>& getProperties() { return properties; }

  const std::vector<PexState>& getStates() const { return states; }
  std::vector<PexState>& getStates() { return states; }

 private:
  PexString name;
  PexString parentName;
  PexString documentationString;
  PexString autoStateName;

  std::vector<PexVariable> variables;
  std::vector<PexProperty> properties;
  std::vector<PexState> states;
};

PexWriter& operator<<(PexWriter& writer, const PexObject& object);

}  // namespace pex
}  // namespace vellum