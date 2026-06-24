#include "lsp_utf16.h"

#include <algorithm>
#include <cstdint>
#include <string_view>

#include "common/types.h"

namespace {
using vellum::common::Vec;

struct LineRange {
  size_t start;
  size_t end;
};

Vec<LineRange> splitLines(const std::string& text) {
  Vec<LineRange> lines;
  size_t start = 0;
  for (size_t i = 0; i < text.size(); ++i) {
    const char c = text[i];
    if (c == '\n') {
      lines.push_back({start, i});
      start = i + 1;
    } else if (c == '\r') {
      lines.push_back({start, i});
      if (i + 1 < text.size() && text[i + 1] == '\n') {
        start = i + 2;
        ++i;
      } else {
        start = i + 1;
      }
    }
  }
  lines.push_back({start, text.size()});
  return lines;
}

size_t utf8CodepointSize(unsigned char lead) {
  if ((lead & 0x80) == 0) {
    return 1;
  }
  if ((lead & 0xE0) == 0xC0) {
    return 2;
  }
  if ((lead & 0xF0) == 0xE0) {
    return 3;
  }
  if ((lead & 0xF8) == 0xF0) {
    return 4;
  }
  return 1;
}

uint32_t readUtf8Codepoint(std::string_view text, size_t& index) {
  if (index >= text.size()) {
    return 0;
  }

  const unsigned char b0 = static_cast<unsigned char>(text[index]);
  size_t len = utf8CodepointSize(b0);
  if (index + len > text.size()) {
    len = 1;
  }

  uint32_t codepoint = b0;
  if (len == 2) {
    codepoint = (b0 & 0x1F) << 6;
    codepoint |= static_cast<unsigned char>(text[index + 1]) & 0x3F;
  } else if (len == 3) {
    codepoint = (b0 & 0x0F) << 12;
    codepoint |= (static_cast<unsigned char>(text[index + 1]) & 0x3F) << 6;
    codepoint |= static_cast<unsigned char>(text[index + 2]) & 0x3F;
  } else if (len == 4) {
    codepoint = (b0 & 0x07) << 18;
    codepoint |= (static_cast<unsigned char>(text[index + 1]) & 0x3F) << 12;
    codepoint |= (static_cast<unsigned char>(text[index + 2]) & 0x3F) << 6;
    codepoint |= static_cast<unsigned char>(text[index + 3]) & 0x3F;
  }

  index += len;
  return codepoint;
}

lsp::uint utf16Length(std::string_view line) {
  size_t index = 0;
  lsp::uint length = 0;
  while (index < line.size()) {
    const uint32_t codepoint = readUtf8Codepoint(line, index);
    length += codepoint > 0xFFFF ? 2 : 1;
  }
  return length;
}

size_t utf8OffsetAtUtf16Position(std::string_view line, lsp::uint utf16Character) {
  size_t index = 0;
  lsp::uint utf16Count = 0;
  while (index < line.size() && utf16Count < utf16Character) {
    const uint32_t codepoint = readUtf8Codepoint(line, index);
    utf16Count += codepoint > 0xFFFF ? 2 : 1;
  }
  return index;
}

}  // namespace

namespace vellum {

size_t utf8ByteOffsetAtLspPosition(const std::string& text,
                                   const lsp::Position& position) {
  const Vec<LineRange> lines = splitLines(text);
  lsp::uint line = position.line;
  if (line > lines.size()) {
    line = static_cast<lsp::uint>(lines.size());
  }
  if (line >= lines.size()) {
    return text.size();
  }

  const LineRange& lineRange = lines[line];
  const std::string_view lineText(text.data() + lineRange.start,
                                  lineRange.end - lineRange.start);
  const lsp::uint character =
      std::min(position.character, utf16Length(lineText));
  return lineRange.start + utf8OffsetAtUtf16Position(lineText, character);
}

}  // namespace vellum
