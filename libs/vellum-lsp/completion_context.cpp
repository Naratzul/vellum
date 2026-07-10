#include "completion_context.h"

#include "ast_locator.h"
#include "common/string_utils.h"
#include "document_analyzer.h"

namespace vellum {
namespace {

bool isIdentifierChar(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') || c == '_';
}

bool isHorizontalSpace(char c) { return c == ' ' || c == '\t'; }

}  // namespace

std::string_view lineAt(std::string_view source, unsigned lineIndex) {
  unsigned currentLine = 0;
  size_t start = 0;
  for (size_t i = 0; i < source.size(); ++i) {
    if (source[i] == '\n') {
      if (currentLine == lineIndex) {
        return source.substr(start, i - start);
      }
      currentLine++;
      start = i + 1;
    }
  }
  if (currentLine == lineIndex) {
    return source.substr(start);
  }
  return {};
}

PrefixAtCursor prefixAtPosition(std::string_view source, lsp::Position pos) {
  PrefixAtCursor result;
  const std::string_view line = lineAt(source, pos.line);
  if (line.empty() || pos.character > line.size()) {
    result.replaceRange = lsp::Range{.start = pos, .end = pos};
    return result;
  }

  size_t cursor = pos.character;
  if (cursor > line.size()) {
    cursor = line.size();
  }

  if (cursor > 0 && line[cursor - 1] == '.') {
    result.afterDot = true;
    result.replaceRange = lsp::Range{.start = pos, .end = pos};
    return result;
  }
  if (cursor < line.size() && line[cursor] == '.') {
    result.afterDot = true;
    result.replaceRange = lsp::Range{.start = pos, .end = pos};
    return result;
  }

  // Walk back over spaces so "adv |" still completes the identifier "adv".
  size_t end = cursor;
  while (end > 0 && isHorizontalSpace(line[end - 1])) {
    end--;
  }

  if (end == 0 || !isIdentifierChar(line[end - 1])) {
    result.replaceRange = lsp::Range{.start = pos, .end = pos};
    return result;
  }

  size_t start = end;
  while (start > 0 && isIdentifierChar(line[start - 1])) {
    start--;
  }
  size_t endIdent = end;
  while (endIdent < line.size() && isIdentifierChar(line[endIdent])) {
    endIdent++;
  }

  size_t rangeEnd = endIdent;
  if (cursor > rangeEnd) {
    rangeEnd = cursor;
  }

  result.replaceRange = lsp::Range{
      .start = lsp::Position{.line = pos.line,
                             .character = static_cast<unsigned int>(start)},
      .end = lsp::Position{.line = pos.line,
                           .character = static_cast<unsigned int>(rangeEnd)}};
  result.prefix = std::string(
      common::normalizeToLower(line.substr(start, endIdent - start)));

  if (start > 0 && line[start - 1] == '.') {
    result.afterDot = true;
  } else if (cursor > 0 && line[cursor - 1] == '.') {
    result.afterDot = true;
  }

  return result;
}

CompletionContext detectContext(
    std::string_view source,
    const lsp::requests::TextDocument_Completion::Params& params,
    const CachedAnalysis& cache, const PrefixAtCursor& prefix) {
  CompletionContext ctx;
  const bool parseOk = cache.navigation && cache.navigation->parseOk;

  const bool triggerDot =
      params.context &&
      params.context->triggerKind ==
          lsp::CompletionTriggerKind::TriggerCharacter &&
      params.context->triggerCharacter == ".";

  if (prefix.afterDot || triggerDot) {
    ctx.kind = CompletionContextKind::MemberAccess;
    return ctx;
  }

  if (cache.navigation && cache.navigation->parseOk) {
    const Opt<AstLocatorTarget> target =
        AstLocator::locate(cache.navigation->parseResult, params.position);
    if (target) {
      switch (target->kind) {
        case AstLocatorTargetKind::PropertyUse:
          ctx.kind = CompletionContextKind::MemberAccess;
          return ctx;
        case AstLocatorTargetKind::ImportName:
          ctx.kind = CompletionContextKind::ImportModule;
          return ctx;
        case AstLocatorTargetKind::ScriptName:
          ctx.kind = CompletionContextKind::ScriptName;
          return ctx;
        case AstLocatorTargetKind::ParentScriptName:
          ctx.kind = CompletionContextKind::ParentType;
          return ctx;
        case AstLocatorTargetKind::TypeReference:
          ctx.kind = CompletionContextKind::TypeAnnotation;
          return ctx;
        case AstLocatorTargetKind::IdentifierUse:
        case AstLocatorTargetKind::CallCallee:
          ctx.kind = CompletionContextKind::Expression;
          return ctx;
        case AstLocatorTargetKind::DeclName:
          ctx.kind = CompletionContextKind::Keyword;
          return ctx;
      }
    }
  }

  const std::string_view line = lineAt(source, params.position.line);
  const size_t col = params.position.character;
  if (!line.empty() && col > 0) {
    size_t i = col;
    if (i > line.size()) {
      i = line.size();
    }
    while (i > 0 && (line[i - 1] == ' ' || line[i - 1] == '\t')) {
      i--;
    }
    if (i > 0 && line[i - 1] == ':') {
      ctx.kind = CompletionContextKind::TypeAnnotation;
      return ctx;
    }
    if (i > 0 && line[i - 1] == '[') {
      size_t bracket = i - 1;
      while (bracket > 0 && isHorizontalSpace(line[bracket - 1])) {
        bracket--;
      }
      if (bracket > 0) {
        const char beforeBracket = line[bracket - 1];
        if (beforeBracket == '=' || beforeBracket == ':' ||
            beforeBracket == '(' || beforeBracket == ',') {
          ctx.kind = CompletionContextKind::TypeAnnotation;
          return ctx;
        }
      }
    }
    if (line.find("import") != std::string_view::npos && col > 6) {
      const size_t importPos = line.find("import");
      if (importPos != std::string_view::npos && col > importPos + 6) {
        const size_t quotePos = line.find('"', importPos);
        if (quotePos != std::string_view::npos && col > quotePos) {
          ctx.kind = CompletionContextKind::ImportModule;
          return ctx;
        }
      }
    }
  }

  if (parseOk && cache.navigation && cache.navigation->resolver) {
    ctx.kind = CompletionContextKind::Expression;
  } else {
    ctx.kind = CompletionContextKind::Keyword;
  }
  return ctx;
}

}  // namespace vellum
