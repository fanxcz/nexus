#pragma once
#include <cstddef>
#include <string>

namespace nexus {

enum class TokenKind {
    End,
    Identifier,
    Integer,
    String,
    KwFn,
    KwLet,
    KwMut,
    KwReturn,
    KwIf,
    KwElse,
    KwWhile,
    KwTrue,
    KwFalse,
    KwPrint,
    Plus, Minus, Star, Slash, Percent,
    Equal, EqualEqual, BangEqual,
    Less, LessEqual, Greater, GreaterEqual,
    Bang, AndAnd, OrOr,
    LParen, RParen, LBrace, RBrace,
    Comma, Colon, Arrow, Semicolon,
};

struct SourcePos { std::size_t line=1, column=1, offset=0; };
struct Token { TokenKind kind; std::string text; SourcePos pos; };
const char* tokenName(TokenKind kind);

}
