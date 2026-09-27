#pragma once
#include "lexer/token.hpp"
#include <memory>
#include <string>
#include <vector>
#include <variant>

namespace nexus {
enum class TypeKind { I64, Bool, String, Void, Unknown };
struct Type { TypeKind kind=TypeKind::Unknown; static Type i64(){return {TypeKind::I64};} static Type boolean(){return {TypeKind::Bool};} static Type string(){return {TypeKind::String};} static Type void_(){return {TypeKind::Void};} };
struct Expr { virtual ~Expr()=default; using Ptr=std::unique_ptr<Expr>; };
struct IntExpr:Expr{long long value;explicit IntExpr(long long v):value(v){}}; struct BoolExpr:Expr{bool value;explicit BoolExpr(bool v):value(v){}}; struct StringExpr:Expr{std::string value;explicit StringExpr(std::string v):value(std::move(v)){}}; struct VarExpr:Expr{std::string name;explicit VarExpr(std::string n):name(std::move(n)){}}; struct UnaryExpr:Expr{TokenKind op;Expr::Ptr rhs;UnaryExpr(TokenKind o,Expr::Ptr r):op(o),rhs(std::move(r)){}}; struct BinaryExpr:Expr{TokenKind op;Expr::Ptr lhs,rhs;BinaryExpr(TokenKind o,Expr::Ptr l,Expr::Ptr r):op(o),lhs(std::move(l)),rhs(std::move(r)){}}; struct CallExpr:Expr{std::string callee;std::vector<Expr::Ptr> args;CallExpr(std::string c,std::vector<Expr::Ptr>a):callee(std::move(c)),args(std::move(a)){}};
struct Stmt { virtual ~Stmt()=default; using Ptr=std::unique_ptr<Stmt>; }; struct Block:Stmt{std::vector<Stmt::Ptr> statements;}; struct LetStmt:Stmt{std::string name;bool mut=false;Expr::Ptr init;Type type{};}; struct ExprStmt:Stmt{Expr::Ptr expr;}; struct ReturnStmt:Stmt{Expr::Ptr expr;}; struct IfStmt:Stmt{Expr::Ptr cond;std::unique_ptr<Block> thenBlock;std::unique_ptr<Block> elseBlock;}; struct WhileStmt:Stmt{Expr::Ptr cond;std::unique_ptr<Block> body;};
struct Param{std::string name;Type type;}; struct Function{std::string name;std::vector<Param> params;Type ret;std::unique_ptr<Block> body;}; struct Program{std::vector<Function> functions;};
}
