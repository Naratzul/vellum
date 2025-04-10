#include "pex_object.h"

namespace vellum {
namespace pex {

PexWriter& vellum::pex::operator<<(PexWriter& writer, const PexObject& object) {
  writer << object.getName() << object.getParentName();
  return writer;
}

}  // namespace pex
}  // namespace vellum