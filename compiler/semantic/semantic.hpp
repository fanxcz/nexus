#pragma once
#include "ast/ast.hpp"
#include <unordered_map>
namespace nexus { class SemanticAnalyzer { std::unordered_map<std::string,Function*> funcs_; public: void analyze(Program& p); private: Type expr(Expr* e,std::unordered_map<std::string,Type>& env); void block(Block& b,std::unordered_map<std::string,Type>& env,Type ret); }; }
