#pragma once
#include "ast/ast.hpp"
#include <vector>
namespace nexus { class Parser { public: explicit Parser(std::vector<Token> t):tokens_(std::move(t)){} Program parseProgram(); private: std::vector<Token> tokens_; size_t i_=0; const Token& cur()const; bool accept(TokenKind); Token expect(TokenKind); Function parseFunction(); std::unique_ptr<Block> parseBlock(); Stmt::Ptr parseStmt(); Expr::Ptr parseExpr(int minPrec=0); Expr::Ptr parsePrimary(); int precedence(TokenKind) const; Type parseType();}; }
