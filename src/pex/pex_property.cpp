#include "pex_property.h"

#include "pex_writer.h"

namespace vellum {
namespace pex {

enum PexPropertyFlags : uint8_t { Readable = 1, Writable = 2, Auto = 4 };

PexWriter& operator<<(PexWriter& writer, const PexProperty& property) {
  writer << property.getName() << property.getTypeName()
         << property.getDocumentationString() << property.getUserFlags();

  uint8_t flags = 0;

  if (property.getBackedVariableName().has_value()) {
    flags = PexPropertyFlags::Readable | PexPropertyFlags::Writable |
            PexPropertyFlags::Auto;
  } else {
    if (property.getGetAccessor().has_value()) {
      flags |= PexPropertyFlags::Readable;
    }
    if (property.getSetAccessor().has_value()) {
      flags |= PexPropertyFlags::Writable;
    }
  }

  writer << flags;

  if (auto backedVariableName = property.getBackedVariableName()) {
    assert(backedVariableName.value().isValid());
    writer << backedVariableName.value();
  } else {
    if (auto getAccessor = property.getGetAccessor()) {
      writer << getAccessor.value();
    }
    if (auto setAccessor = property.getSetAccessor()) {
      writer << setAccessor.value();
    }
  }

  return writer;
}
}  // namespace pex
}  // namespace vellum