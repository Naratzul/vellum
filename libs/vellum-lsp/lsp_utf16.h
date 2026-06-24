#pragma once

#include <lsp/types.h>

#include <cstddef>
#include <string>

namespace vellum {

size_t utf8ByteOffsetAtLspPosition(const std::string& text,
                                   const lsp::Position& position);

}  // namespace vellum
