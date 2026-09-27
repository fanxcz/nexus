#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic.hpp"
#include <iostream>
#include <stdexcept>

static nexus::Program parse(const char* source) {
    return nexus::Parser(nexus::Lexer(source).tokenize()).parseProgram();
}

int main(){
    try {
        {
            auto p=parse("fn main(){ let mut x: i64 = 2 + 3; x = x + 1; print(x) }");
            nexus::SemanticAnalyzer().analyze(p);
        }
        {
            auto p=parse("struct Player { id: i64 health: i64 speed: f64 } fn main(){ let mut p = Player{id:1,health:100,speed:4.5}; p.health = p.health - 25; print(p.health) }");
            nexus::SemanticAnalyzer().analyze(p);
        }
        {
            auto p=parse("fn inc(x: i64)->i64 { return x + 1 } fn main(){ print(inc(41)) }");
            nexus::SemanticAnalyzer().analyze(p);
        }
        {
            auto p=parse("fn main(){ let name = \"Nexus\"; print(\"Hello, \" + name); print(name == \"Nexus\"); print(name != \"Other\") }");
            nexus::SemanticAnalyzer().analyze(p);
        }
        {
            auto p=parse("enum Color { Red, Green, Blue } fn main(){ let c = Color::Green; match c { Color::Red => { print(\"red\") } Color::Green => { print(\"green\") } _ => { print(\"other\") } } }");
            nexus::SemanticAnalyzer().analyze(p);
        }
        {
            auto p=parse("fn main(){ let mut a = [1, 2, 3]; a[1] = 99; let i: i64 = 2; print(a[i]) }");
            nexus::SemanticAnalyzer().analyze(p);
        }
        {
            bool failed=false;
            try {
                auto p=parse("fn main(){ let x = 1; x = 2 }");
                nexus::SemanticAnalyzer().analyze(p);
            } catch(const std::exception&) { failed=true; }
            if(!failed) return 1;
        }
        {
            bool failed=false;
            try { auto p=parse("fn main(){ let a = [1, 2, 3]; print(a[3]) }"); nexus::SemanticAnalyzer().analyze(p); } catch(const std::exception&) { failed=true; }
            if(!failed) return 2;
        }
        std::cout<<"Nexus tests: PASS\n";
        return 0;
    } catch(const std::exception&e){
        std::cerr<<e.what()<<"\n";
        return 1;
    }
}
