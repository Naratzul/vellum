#include "papyrus_parser.h"

#include "ast/decl/declaration.h"
#include "common/types.h"
#include "vellum/vellum_literal.h"

namespace vellum {
using common::makeUnique;
using common::Opt;
using common::Shared;
using common::Unique;
using common::Vec;

PapyrusParser::PapyrusParser(Unique<ILexer> lexer,
                             Shared<CompilerErrorHandler> errorHandler)
    : lexer(std::move(lexer)), errorHandler(errorHandler) {}

ParserResult PapyrusParser::parse() {
  advance();

  ParserResult result;
  bool hasScript = false;

  while (!check(TokenType::END_OF_FILE)) {
    try {
      Token before = current;
      auto decl = topDeclaration();
      if (current.type == before.type &&
          current.location.start.line == before.location.start.line &&
          current.location.start.position == before.location.start.position) {
        errorHandler->errorAt(
            current, CompilerErrorKind::ExpectDeclaration,
            "Could not advance while parsing top-level declarations.");
        advance();
        continue;
      }
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
      errorHandler->errorAt(e.getToken(), e.getErrorKind(), e.what());
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
  switch (current.type) {
    case TokenType::IMPORT:
      advance();
      return importDeclaration();
    case TokenType::SCRIPTNAME:
      advance();
      return scriptDeclaration();
    case TokenType::FUNCTION:
      advance();
      return functionDeclarationWithReturnType(VellumType::none());
    case TokenType::EVENT:
      advance();
      return eventDeclaration();
    case TokenType::STATE:
      advance();
      return stateDeclaration(false);
    case TokenType::AUTO:
      advance();
      consume(TokenType::STATE, CompilerErrorKind::ExpectDeclaration,
              "State expected.");
      return stateDeclaration(true);
    case TokenType::IDENTIFIER: {
      VellumType type = parseType();
      Token typeLocation = previous;
      if (match(TokenType::FUNCTION)) {
        return functionDeclarationWithReturnType(type);
      }
      if (match(TokenType::PROPERTY)) {
        return propertyDeclarationWithType(type, typeLocation);
      }
      return variableDeclaration(type);
    }
    default:
      break;
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

  match(TokenType::HIDDEN);
  match(TokenType::CONDITIONAL);

  auto parentScriptName = VellumType::none();
  Opt<Token> parentScriptNameLocation;
  if (match(TokenType::EXTENDS)) {
    consume(TokenType::IDENTIFIER, CompilerErrorKind::ExpectDeclaration,
            "Expect a parent script's name after 'extends'.");
    parentScriptName = VellumType::identifier(previous.lexeme);
    parentScriptNameLocation = previous;

    match(TokenType::HIDDEN);
    match(TokenType::CONDITIONAL);
  }

  return makeUnique<ast::ScriptDeclaration>(
      scriptName, scriptNameLocation, parentScriptName,
      parentScriptNameLocation, Vec<Unique<ast::Declaration>>{});
}

Unique<ast::Declaration> PapyrusParser::stateDeclaration(bool isAuto) {
  consume(TokenType::IDENTIFIER, CompilerErrorKind::ExpectDeclaration,
          "Expect a state's name.");
  auto stateName = previous.lexeme;
  auto nameLocation = previous;

  skipUntilEndState();

  return makeUnique<ast::StateDeclaration>(stateName, nameLocation, isAuto,
                                           Vec<Unique<ast::Declaration>>{});
}

Unique<ast::Declaration> PapyrusParser::functionDeclarationWithReturnType(
    VellumType returnTypeName) {
  consume(TokenType::IDENTIFIER, CompilerErrorKind::ExpectFunctionName,
          "Expect a function name.");
  const std::string_view name = previous.lexeme;
  Token nameLocation = previous;

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

  if (!isNative) {
    skipUntilEndFunction();
  }

  Opt<Token> returnTypeLocation = std::nullopt;
  return makeUnique<ast::FunctionDeclaration>(
      name, parameters, returnTypeName,
      makeUnique<ast::BlockStatement>(ast::FunctionBody{}), isGlobal,
      nameLocation, returnTypeLocation);
}

Unique<ast::Declaration> PapyrusParser::eventDeclaration() {
  consume(TokenType::IDENTIFIER, CompilerErrorKind::ExpectFunctionName,
          "Expect an event name.");
  const std::string_view name = previous.lexeme;
  Token nameLocation = previous;

  consume(TokenType::LEFT_PAREN, CompilerErrorKind::ExpectLeftParen,
          "Expect '(' after event name.");

  Vec<ast::FunctionParameter> parameters;
  if (!check(TokenType::RIGHT_PAREN)) {
    parameters = parseParameters();
  }

  consume(TokenType::RIGHT_PAREN, CompilerErrorKind::ExpectRightParen,
          "Expect ')' after event parameters.");

  bool isNative = match(TokenType::NATIVE);

  if (isNative) {
    skipToEndOfStatement();
  } else {
    skipUntilEndEvent();
  }

  Opt<Token> returnTypeLocation = std::nullopt;
  return makeUnique<ast::FunctionDeclaration>(
      name, parameters, VellumType::none(),
      makeUnique<ast::BlockStatement>(ast::FunctionBody{}), false, nameLocation,
      returnTypeLocation);
}

void PapyrusParser::consumePropertyUserFlags(bool& isAuto,
                                             bool& isAutoReadOnly) {
  while (match(TokenType::HIDDEN) || match(TokenType::CONDITIONAL)) {
  }

  if (match(TokenType::AUTOREADONLY)) {
    isAutoReadOnly = true;
    while (match(TokenType::HIDDEN) || match(TokenType::CONDITIONAL)) {
    }
    return;
  }

  if (match(TokenType::AUTO)) {
    isAuto = true;
    while (match(TokenType::HIDDEN) || match(TokenType::CONDITIONAL)) {
    }
  }
}

Unique<ast::Declaration> PapyrusParser::propertyDeclarationWithType(
    VellumType typeName, Token typeLocation) {
  consume(TokenType::IDENTIFIER, CompilerErrorKind::ExpectDeclaration,
          "Expect a property name.");
  const std::string_view name = previous.lexeme;
  Token nameLocation = previous;

  parseDefaultArgument();

  bool isAuto = false;
  bool isAutoReadOnly = false;
  consumePropertyUserFlags(isAuto, isAutoReadOnly);

  if (!isAuto && !isAutoReadOnly) {
    skipUntilEndProperty();
  }

  return makeUnique<ast::PropertyDeclaration>(name, typeName, "", std::nullopt,
                                              std::nullopt, std::nullopt,
                                              nameLocation, typeLocation);
}

Unique<ast::Declaration> PapyrusParser::variableDeclaration(VellumType type) {
  consume(TokenType::IDENTIFIER, CompilerErrorKind::ExpectVarName,
          "Expect a variable name.");

  auto name = previous.lexeme;
  auto nameLocation = previous;

  parseDefaultArgument();

  return makeUnique<ast::GlobalVariableDeclaration>(name, type, nullptr,
                                                    nameLocation);
}

Vec<ast::FunctionParameter> PapyrusParser::parseParameters() {
  Vec<ast::FunctionParameter> parameters;

  do {
    if (check(TokenType::COMMA)) {
      advance();
    }

    VellumType paramType = parseType();
    Token paramTypeLocation = previous;

    consume(TokenType::IDENTIFIER, CompilerErrorKind::ExpectParamName,
            "Expect a parameter name.");
    std::string_view paramName = previous.lexeme;
    Token paramNameLocation = previous;

    Opt<VellumLiteral> defaultValue = parseDefaultArgument();

    parameters.emplace_back(paramName, paramType, std::move(defaultValue),
                            paramNameLocation, paramTypeLocation);
  } while (match(TokenType::COMMA));

  return parameters;
}

Opt<VellumLiteral> PapyrusParser::parseDefaultArgument() {
  if (!match(TokenType::EQUAL)) return std::nullopt;
  bool negative = match(TokenType::MINUS);
  if (check(TokenType::INT)) {
    advance();
    if (!previous.value)
      throw ParseException(previous,
                           "Expect a literal value for default argument.");
    int32_t v = previous.value->asInt();
    return VellumLiteral(negative ? -v : v);
  }
  if (check(TokenType::FLOAT)) {
    advance();
    if (!previous.value)
      throw ParseException(previous,
                           "Expect a literal value for default argument.");
    float v = previous.value->asFloat();
    return VellumLiteral(negative ? -v : v);
  }
  if (negative)
    throw ParseException(current,
                         "Expect a literal value for default argument.");
  if (match(TokenType::STRING)) {
    if (!previous.value)
      throw ParseException(previous,
                           "Expect a literal value for default argument.");
    return *previous.value;
  }
  if (match(TokenType::TRUE)) return VellumLiteral(true);
  if (match(TokenType::FALSE)) return VellumLiteral(false);
  if (match(TokenType::NONE)) return VellumLiteral();
  throw ParseException(current, "Expect a literal value for default argument.");
}

VellumType PapyrusParser::parseType() {
  if (!check(TokenType::IDENTIFIER)) {
    throw ParseException(current, "Expect a type name.");
  }

  advance();
  std::string_view typeName = previous.lexeme;

  VellumType subtype = VellumType::unresolved(typeName);

  if (typeName == "bool") {
    subtype = VellumType::literal(VellumLiteralType::Bool);
  } else if (typeName == "int") {
    subtype = VellumType::literal(VellumLiteralType::Int);
  } else if (typeName == "float") {
    subtype = VellumType::literal(VellumLiteralType::Float);
  } else if (typeName == "string") {
    subtype = VellumType::literal(VellumLiteralType::String);
  } else if (typeName == "Var") {
    subtype = VellumType::unresolved("Var");
  }

  if (match(TokenType::LEFT_BRACK)) {
    consume(TokenType::RIGHT_BRACK, CompilerErrorKind::ExpectRightBracket,
            "Expect ']' after array type.");
    return VellumType::array(std::move(subtype));
  }

  return subtype;
}

void PapyrusParser::skipUntilEndFunction() {
  while (!check(TokenType::END_OF_FILE)) {
    if (match(TokenType::ENDFUNCTION)) {
      return;
    }
    if (match(TokenType::FUNCTION)) {
      skipUntilEndFunction();
    } else if (match(TokenType::EVENT)) {
      skipUntilEndEvent();
    } else if (match(TokenType::IF)) {
      while (!check(TokenType::END_OF_FILE) && !check(TokenType::ENDIF)) {
        if (match(TokenType::FUNCTION)) {
          skipUntilEndFunction();
        } else if (match(TokenType::EVENT)) {
          skipUntilEndEvent();
        } else {
          advance();
        }
      }
      match(TokenType::ENDIF);
    } else if (match(TokenType::WHILE)) {
      while (!check(TokenType::END_OF_FILE) && !check(TokenType::ENDWHILE)) {
        if (match(TokenType::FUNCTION)) {
          skipUntilEndFunction();
        } else if (match(TokenType::EVENT)) {
          skipUntilEndEvent();
        } else {
          advance();
        }
      }
      match(TokenType::ENDWHILE);
    } else {
      advance();
    }
  }
}

void PapyrusParser::skipUntilEndEvent() {
  while (!check(TokenType::END_OF_FILE)) {
    if (match(TokenType::ENDEVENT)) {
      return;
    }
    if (match(TokenType::FUNCTION)) {
      skipUntilEndFunction();
    } else if (match(TokenType::EVENT)) {
      skipUntilEndEvent();
    } else if (match(TokenType::IF)) {
      while (!check(TokenType::END_OF_FILE) && !check(TokenType::ENDIF)) {
        if (match(TokenType::FUNCTION)) {
          skipUntilEndFunction();
        } else if (match(TokenType::EVENT)) {
          skipUntilEndEvent();
        } else {
          advance();
        }
      }
      match(TokenType::ENDIF);
    } else if (match(TokenType::WHILE)) {
      while (!check(TokenType::END_OF_FILE) && !check(TokenType::ENDWHILE)) {
        if (match(TokenType::FUNCTION)) {
          skipUntilEndFunction();
        } else if (match(TokenType::EVENT)) {
          skipUntilEndEvent();
        } else {
          advance();
        }
      }
      match(TokenType::ENDWHILE);
    } else {
      advance();
    }
  }
}

void PapyrusParser::skipUntilEndProperty() {
  while (!check(TokenType::END_OF_FILE)) {
    if (match(TokenType::ENDPROPERTY)) {
      return;
    }
    if (match(TokenType::FUNCTION)) {
      skipUntilEndFunction();
    } else if (match(TokenType::EVENT)) {
      skipUntilEndEvent();
    } else if (match(TokenType::IF)) {
      while (!check(TokenType::END_OF_FILE) && !check(TokenType::ENDIF)) {
        if (match(TokenType::FUNCTION)) {
          skipUntilEndFunction();
        } else if (match(TokenType::EVENT)) {
          skipUntilEndEvent();
        } else {
          advance();
        }
      }
      match(TokenType::ENDIF);
    } else if (match(TokenType::WHILE)) {
      while (!check(TokenType::END_OF_FILE) && !check(TokenType::ENDWHILE)) {
        if (match(TokenType::FUNCTION)) {
          skipUntilEndFunction();
        } else if (match(TokenType::EVENT)) {
          skipUntilEndEvent();
        } else {
          advance();
        }
      }
      match(TokenType::ENDWHILE);
    } else {
      advance();
    }
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

void PapyrusParser::skipUntilEndState() {
  while (!check(TokenType::END_OF_FILE)) {
    if (match(TokenType::ENDSTATE)) {
      return;
    }
    if (match(TokenType::FUNCTION)) {
      skipUntilEndFunction();
    } else if (match(TokenType::EVENT)) {
      skipUntilEndEvent();
    } else {
      advance();
    }
  }
}

void PapyrusParser::synchronize() {
  while (!check(TokenType::END_OF_FILE)) {
    switch (current.type) {
      case TokenType::SCRIPTNAME:
      case TokenType::IMPORT:
      case TokenType::FUNCTION:
      case TokenType::EVENT:
      case TokenType::STATE:
      case TokenType::AUTO:
      case TokenType::IDENTIFIER:
        return;
      default:
        break;
    }
    advance();
  }
}

}  // namespace vellum
