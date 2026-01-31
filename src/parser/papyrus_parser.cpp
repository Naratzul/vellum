#include "papyrus_parser.h"

#include "ast/decl/declaration.h"
#include "common/types.h"
#include "vellum/vellum_value.h"

namespace vellum {
using common::makeUnique;
using common::Opt;
using common::Shared;
using common::Unique;
using common::Vec;

namespace {
class ParseException : public std::runtime_error {
 public:
  ParseException(Token token, const std::string& message)
      : std::runtime_error(message), token(token) {}

  Token getToken() const { return token; }
  std::string getMessage() const { return what(); }

 private:
  Token token;
};
}  // namespace

PapyrusParser::PapyrusParser(Unique<ILexer> lexer,
                             Shared<CompilerErrorHandler> errorHandler)
    : lexer(std::move(lexer)), errorHandler(errorHandler) {}

ParserResult PapyrusParser::parse() {
  advance();

  ParserResult result;
  bool hasScript = false;

  while (!check(TokenType::END_OF_FILE)) {
    try {
      auto decl = topDeclaration();
      if (decl->getOrder() == ast::DeclarationOrder::Script) {
        if (hasScript) {
          errorHandler->errorAt(
              previous, CompilerErrorKind::MultipleScriptsDefinition,
              "Only one Scriptname declaration is allowed per file.");
          continue;
        }
        hasScript = true;
      }
      result.declarations.push_back(std::move(decl));
    } catch (const ParseException& e) {
      errorHandler->errorAt(e.getToken(), CompilerErrorKind::ExpectDeclaration,
                            e.getMessage());
      synchronize();
    }
  }

  return result;
}

void PapyrusParser::advance() {
  previous = current;
  for (;;) {
    current = lexer->scanToken();
    if (current.type != TokenType::ERROR) {
      break;
    }
    errorHandler->errorAt(current, current.lexeme);
  }
}

Unique<ast::Declaration> PapyrusParser::topDeclaration() {
  if (match(TokenType::IMPORT)) {
    return importDeclaration();
  } else if (match(TokenType::SCRIPTNAME)) {
    return scriptDeclaration();
  } else if (match(TokenType::FUNCTION)) {
    return functionDeclarationWithReturnType(VellumType::none());
  } else if (match(TokenType::EVENT)) {
    return eventDeclaration();
  } else if (check(TokenType::IDENTIFIER)) {
    // Could be: returnType Function Name, or Type Property Name
    // Parse the type first (it will be consumed), then check next token
    VellumType type = parseType();

    if (match(TokenType::FUNCTION)) {
      // It's a function: returnType Function Name
      // We've already parsed the return type, now parse the rest
      return functionDeclarationWithReturnType(type);
    } else if (match(TokenType::PROPERTY)) {
      // It's a property: Type Property Name
      // We've already parsed the type, now parse the rest
      return propertyDeclarationWithType(type);
    }

    // Neither Function nor Property - error
    throw ParseException(current,
                         "Expect 'Function' or 'Property' after type name.");
  }

  throw ParseException(previous,
                       "Expect a top-level declaration: Scriptname, import, "
                       "Function, Event, or Property.");
}

bool PapyrusParser::match(TokenType type) {
  if (!check(type)) {
    return false;
  }
  advance();
  return true;
}

bool PapyrusParser::check(TokenType type) const { return current.type == type; }

Unique<ast::Declaration> PapyrusParser::importDeclaration() {
  consume(TokenType::IDENTIFIER, CompilerErrorKind::ExpectDeclaration,
          "Import name expected.");
  std::string_view importName = previous.lexeme;
  return makeUnique<ast::ImportDeclaration>(importName, previous);
}

Unique<ast::Declaration> PapyrusParser::scriptDeclaration() {
  consume(TokenType::IDENTIFIER, CompilerErrorKind::ExpectDeclaration,
          "Expect a script name after 'Scriptname'.");
  Token scriptNameLocation = previous;
  auto scriptName = VellumType::identifier(previous.lexeme);

  // Skip optional 'Hidden' keyword (can appear before or after extends)
  match(TokenType::HIDDEN);

  auto parentScriptName = VellumType::none();
  Opt<Token> parentScriptNameLocation;
  if (match(TokenType::EXTENDS)) {
    consume(TokenType::IDENTIFIER, CompilerErrorKind::ExpectDeclaration,
            "Expect a parent script's name after 'extends'.");
    parentScriptName = VellumType::identifier(previous.lexeme);
    parentScriptNameLocation = previous;

    // Skip optional 'Hidden' keyword after extends
    match(TokenType::HIDDEN);
  }

  return makeUnique<ast::ScriptDeclaration>(
      scriptName, scriptNameLocation, parentScriptName,
      parentScriptNameLocation, Vec<Unique<ast::Declaration>>{});
}

Unique<ast::Declaration> PapyrusParser::functionDeclarationWithReturnType(
    VellumType returnTypeName) {
  consume(TokenType::IDENTIFIER, CompilerErrorKind::ExpectFunctionName,
          "Expect a function name.");
  const std::string_view name = previous.lexeme;

  consume(TokenType::LEFT_PAREN, CompilerErrorKind::ExpectLeftParen,
          "Expect '(' after function name.");

  Vec<ast::FunctionParameter> parameters;
  if (!check(TokenType::RIGHT_PAREN)) {
    parameters = parseParameters();
  }

  consume(TokenType::RIGHT_PAREN, CompilerErrorKind::ExpectRightParen,
          "Expect ')' after function parameters.");

  bool isNative = match(TokenType::NATIVE);
  bool isGlobal = match(TokenType::GLOBAL);

  // Skip function body
  if (!isNative) {
    skipUntilEndFunction();
  }

  return makeUnique<ast::FunctionDeclaration>(name, parameters, returnTypeName,
                                              ast::FunctionBody{}, isGlobal);
}

Unique<ast::Declaration> PapyrusParser::eventDeclaration() {
  consume(TokenType::IDENTIFIER, CompilerErrorKind::ExpectFunctionName,
          "Expect an event name.");
  const std::string_view name = previous.lexeme;

  consume(TokenType::LEFT_PAREN, CompilerErrorKind::ExpectLeftParen,
          "Expect '(' after event name.");

  Vec<ast::FunctionParameter> parameters;
  if (!check(TokenType::RIGHT_PAREN)) {
    parameters = parseParameters();
  }

  consume(TokenType::RIGHT_PAREN, CompilerErrorKind::ExpectRightParen,
          "Expect ')' after event parameters.");

  // Check if native (native events have no body)
  bool isNative = match(TokenType::NATIVE);

  // Skip event body
  if (isNative) {
    skipToEndOfStatement();
  } else {
    skipUntilEndEvent();
  }

  return makeUnique<ast::FunctionDeclaration>(
      name, parameters, VellumType::none(), ast::FunctionBody{}, false);
}

Unique<ast::Declaration> PapyrusParser::propertyDeclarationWithType(
    VellumType typeName) {
  consume(TokenType::IDENTIFIER, CompilerErrorKind::ExpectDeclaration,
          "Expect a property name.");
  const std::string_view name = previous.lexeme;

  if (match(TokenType::EQUAL)) {
    // Skip default value for now - just consume until Auto/AutoReadOnly
    while (!check(TokenType::AUTO) && !check(TokenType::AUTOREADONLY) &&
           !check(TokenType::END_OF_FILE) && !check(TokenType::SEMICOLON)) {
      advance();
    }
  }

  if (match(TokenType::AUTOREADONLY)) {
  } else if (match(TokenType::AUTO)) {
  } else {
    skipToEndOfStatement();
  }

  return makeUnique<ast::PropertyDeclaration>(name, typeName, "", std::nullopt,
                                              std::nullopt, std::nullopt);
}

Unique<ast::Declaration> PapyrusParser::propertyDeclaration() {
  // Parse type
  VellumType typeName = parseType();

  consume(TokenType::PROPERTY, CompilerErrorKind::ExpectDeclaration,
          "Expect 'Property' after type name.");

  consume(TokenType::IDENTIFIER, CompilerErrorKind::ExpectDeclaration,
          "Expect a property name.");
  const std::string_view name = previous.lexeme;

  if (match(TokenType::EQUAL)) {
    // Skip default value for now - just consume until Auto/AutoReadOnly
    while (!check(TokenType::AUTO) && !check(TokenType::AUTOREADONLY) &&
           !check(TokenType::END_OF_FILE) && !check(TokenType::SEMICOLON)) {
      advance();
    }
  }

  if (match(TokenType::AUTOREADONLY)) {
  } else if (match(TokenType::AUTO)) {
  } else {
    skipToEndOfStatement();
  }

  return makeUnique<ast::PropertyDeclaration>(name, typeName, "", std::nullopt,
                                              std::nullopt, std::nullopt);
}

Vec<ast::FunctionParameter> PapyrusParser::parseParameters() {
  Vec<ast::FunctionParameter> parameters;

  do {
    if (check(TokenType::COMMA)) {
      advance();
    }

    VellumType paramType = parseType();

    consume(TokenType::IDENTIFIER, CompilerErrorKind::ExpectParamName,
            "Expect a parameter name.");
    std::string_view paramName = previous.lexeme;

    if (match(TokenType::EQUAL)) {
      while (!check(TokenType::COMMA) && !check(TokenType::RIGHT_PAREN)) {
        advance();
      }
    }

    parameters.emplace_back(paramName, paramType);
  } while (match(TokenType::COMMA));

  return parameters;
}

VellumType PapyrusParser::parseType() {
  if (!check(TokenType::IDENTIFIER)) {
    throw ParseException(current, "Expect a type name.");
  }

  advance();
  std::string_view typeName = previous.lexeme;

  if (typeName == "bool") {
    return VellumType::literal(VellumLiteralType::Bool);
  } else if (typeName == "int") {
    return VellumType::literal(VellumLiteralType::Int);
  } else if (typeName == "float") {
    return VellumType::literal(VellumLiteralType::Float);
  } else if (typeName == "string") {
    return VellumType::literal(VellumLiteralType::String);
  } else if (typeName == "Var") {
    return VellumType::unresolved("Var");
  }

  if (match(TokenType::LEFT_BRACK)) {
    consume(TokenType::RIGHT_BRACK, CompilerErrorKind::ExpectRightBracket,
            "Expect ']' after array type.");
    VellumType elementType = VellumType::unresolved(typeName);
    return VellumType::array(std::move(elementType));
  }

  return VellumType::unresolved(typeName);
}

void PapyrusParser::skipUntilEndFunction() {
  while (!check(TokenType::ENDFUNCTION) && !check(TokenType::END_OF_FILE)) {
    advance();
  }
  if (match(TokenType::ENDFUNCTION)) {
  }
}

void PapyrusParser::skipUntilEndEvent() {
  while (!check(TokenType::ENDEVENT) && !check(TokenType::END_OF_FILE)) {
    advance();
  }
  if (match(TokenType::ENDEVENT)) {
  }
}

void PapyrusParser::skipToEndOfStatement() {
  while (!check(TokenType::END_OF_FILE) && !check(TokenType::SEMICOLON)) {
    if (current.lexeme.length() > 0 && current.lexeme[0] == '\n') {
      break;
    }
    advance();
  }
  if (check(TokenType::SEMICOLON)) {
    advance();
  }
}

void PapyrusParser::skipBlock() {
  int depth = 1;
  while (depth > 0 && !check(TokenType::END_OF_FILE)) {
    if (match(TokenType::LEFT_BRACE)) {
      depth++;
    } else if (match(TokenType::RIGHT_BRACE)) {
      depth--;
    } else {
      advance();
    }
  }
}

void PapyrusParser::synchronize() {
  while (!check(TokenType::END_OF_FILE)) {
    if (previous.type == TokenType::SEMICOLON) return;

    switch (current.type) {
      case TokenType::SCRIPTNAME:
      case TokenType::FUNCTION:
      case TokenType::EVENT:
      case TokenType::PROPERTY:
      case TokenType::IMPORT:
        return;
      default:
        break;
    }

    advance();
  }
}

}  // namespace vellum