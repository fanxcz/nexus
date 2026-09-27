#include "driver/driver.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic.hpp"
#include "codegen/llvm_ir.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdlib>
namespace nexus {
static std::string readFile(const std::filesystem::path&p){std::ifstream f(p);if(!f)throw std::runtime_error("cannot open "+p.string());return {std::istreambuf_iterator<char>(f),{}};}
static void usage(){std::cout<<"Nexus 0.1.0\nUsage: nexus <command> [options]\n\nCommands:\n  build <file.nx> [-o output] [--emit-ir]\n  run <file.nx>\n  check <file.nx>\n  version\n  help\n\nTargets: Linux x86_64 is the initial native target; the compiler frontend is target-independent.\n";}
static int compileFile(const std::string&in,const std::string&out,bool emitIR){auto src=readFile(in);Lexer lex(src);auto toks=lex.tokenize();Parser p(std::move(toks));auto prog=p.parseProgram();SemanticAnalyzer sem;sem.analyze(prog);LLVMIRGenerator gen;auto ir=gen.generate(prog);auto irPath=out+".ll";std::ofstream irf(irPath);irf<<ir;irf.close();if(emitIR){std::cout<<ir;return 0;}std::string cmd="clang "+irPath+" runtime/runtime.c -O2 -o "+out;int rc=std::system(cmd.c_str());if(rc!=0)throw std::runtime_error("LLVM/Clang backend failed; inspect "+irPath);return 0;}
int run(int argc,char**argv){try{if(argc<2){usage();return 1;}std::string cmd=argv[1];if(cmd=="help"||cmd=="--help"){usage();return 0;}if(cmd=="version"||cmd=="--version"){std::cout<<"Nexus 0.1.0 (C++20/LLVM IR backend)\n";return 0;}if(cmd=="check"){if(argc<3)throw std::runtime_error("check requires a .nx file");auto src=readFile(argv[2]);auto toks=Lexer(src).tokenize();auto prog=Parser(std::move(toks)).parseProgram();SemanticAnalyzer().analyze(prog);std::cout<<"check: OK\n";return 0;}if(cmd=="build"||cmd=="run"){if(argc<3)throw std::runtime_error(cmd+" requires a .nx file");std::string in=argv[2];std::string out=std::filesystem::path(in).stem().string();bool emitIR=false;for(int i=3;i<argc;++i){std::string a=argv[i];if(a=="--emit-ir")emitIR=true;else if(a=="-o"&&i+1<argc)out=argv[++i];}compileFile(in,out,emitIR);if(cmd=="run"){std::string exec="./"+out;return std::system(exec.c_str());}return 0;}usage();return 1;}catch(const std::exception&e){std::cerr<<"error[NX0000]: "<<e.what()<<"\n";return 2;}}
}
