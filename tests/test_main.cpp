#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic.hpp"
#include <iostream>
int main(){
  try {
    auto toks=nexus::Lexer("fn main(){ let x = 2 + 3; print(x) }").tokenize();
    if(toks.size()<10) return 1;
    auto p=nexus::Parser(std::move(toks)).parseProgram();
    nexus::SemanticAnalyzer().analyze(p);
    std::cout<<"Nexus tests: PASS\n";
    return 0;
  } catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}
}
