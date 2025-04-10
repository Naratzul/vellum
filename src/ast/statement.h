#pragma once

#include <optional>
#include <string_view>
#include <vector>

namespace vellum {

namespace ast {

class StatementVisitor;

class Statement {
 public:
  virtual ~Statement() = default;
  virtual void accept(StatementVisitor& visitor) = 0;
};

class ScriptStatement : public Statement {
 public:
  ScriptStatement(std::string_view scriptName,
                  std::optional<std::string_view> parentScriptName)
      : scriptName_(scriptName), parentScriptName_(parentScriptName) {}

  std::string_view scriptName() const { return scriptName_; }
  std::optional<std::string_view> parentScriptName() const {
    return parentScriptName_;
  }

  void accept(StatementVisitor& visitor) override;

 private:
  std::string_view scriptName_;
  std::optional<std::string_view> parentScriptName_;
};
}  // namespace ast
}  // namespace vellum