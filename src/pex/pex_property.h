#pragma once

#include <optional>

#include "pex_function.h"
#include "pex_string.h"
#include "pex_user_flag.h"

namespace vellum {
namespace pex {

class PexWriter;

class PexProperty {
 public:
  PexProperty(PexString name, PexString typeName, PexString documentationString,
              PexUserFlags userFlags, std::optional<PexFunction> getAccessor,
              std::optional<PexFunction> setAccessor,
              std::optional<PexString> backedVariableName)
      : name(name),
        typeName(typeName),
        documentationString(documentationString),
        userFlags(userFlags),
        getAccessor(getAccessor),
        setAccessor(setAccessor),
        backedVariableName(backedVariableName) {}

  PexString getName() const { return name; }
  PexString getTypeName() const { return typeName; }
  PexString getDocumentationString() const { return documentationString; }
  PexUserFlags getUserFlags() const { return userFlags; }

  std::optional<PexFunction> getGetAccessor() const { return getAccessor; }
  std::optional<PexFunction> getSetAccessor() const { return setAccessor; }

  std::optional<PexString> getBackedVariableName() const {
    return backedVariableName;
  }

 private:
  PexString name;
  PexString typeName;
  PexString documentationString;
  PexUserFlags userFlags;

  std::optional<PexFunction> getAccessor;
  std::optional<PexFunction> setAccessor;

  std::optional<PexString> backedVariableName;
};

PexWriter& operator<<(PexWriter& writer, const PexProperty& property);
}  // namespace pex
}  // namespace vellum