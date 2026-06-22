#include "pex_object.h"

#include "pex_writer.h"

namespace vellum {
namespace pex {

PexWriter& operator<<(PexWriter& writer, const PexObject& object) {
  writer << object.getName() << object.getObjectSize() << object.getParentName()
         << object.getDocumentationString() << object.getUserFlags()
         << object.getAutoStateName();

  writer << object.getVariables();
  writer << object.getProperties();
  writer << object.getStates();

  return writer;
}

}  // namespace pex
}  // namespace vellum