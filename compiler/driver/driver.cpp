#include "driver/driver.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic.hpp"
#include "codegen/llvm_ir.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <set>

namespace nexus {
namespace fs=std::filesystem;
static fs::path gExecutable;
static std::string readFile(const fs::path&p){std::ifstream f(p);if(!f)throw std::runtime_error("cannot open "+p.string());return {std::istreambuf_iterator<char>(f),{}};}
static void usage(){std::cout<<"Nexus 0.4.0\nUsage: nexus <command> [options]\n\nCommands:\n  new <project> [--template cli|app|game]\n  init\n  build [file.nx] [-o output] [--target TRIPLE] [--emit-ir] [--release]\n  run [file.nx] [--target TRIPLE]\n  check [file.nx]\n  targets\n  version\n  help\n\nExamples:\n  nexus new hello\n  nexus run examples/hello.nx\n  nexus build app.nx -o app --target x86_64-linux\n  nexus build app.nx --emit-ir\n\nPortable source is target-independent; target toolchains determine which native outputs can be linked on the current host.\n";}
static std::string targetTriple(std::string t){if(t.empty()||t=="native")return {};if(t=="x86_64-linux")return "x86_64-unknown-linux-gnu";if(t=="aarch64-linux")return "aarch64-unknown-linux-gnu";if(t=="x86_64-windows")return "x86_64-pc-windows-gnu";if(t=="aarch64-windows")return "aarch64-pc-windows-msvc";if(t=="x86_64-macos")return "x86_64-apple-darwin";if(t=="aarch64-macos")return "arm64-apple-darwin";if(t=="aarch64-android")return "aarch64-linux-android";if(t=="wasm32-wasi")return "wasm32-wasi";return t;}
static std::string projectEntry(){fs::path p="nexus.toml";if(!fs::exists(p))return {};std::string s=readFile(p);auto pos=s.find("entry");if(pos==std::string::npos)return {};pos=s.find('=',pos);if(pos==std::string::npos)return {};auto q1=s.find('"',pos),q2=s.find('"',q1+1);if(q1==std::string::npos||q2==std::string::npos)return {};return s.substr(q1+1,q2-q1-1);}
static void appendProgram(Program&dst,Program src){dst.imports.insert(dst.imports.end(),src.imports.begin(),src.imports.end());for(auto&s:src.structs)dst.structs.push_back(std::move(s));for(auto&e:src.enums)dst.enums.push_back(std::move(e));for(auto&f:src.functions)dst.functions.push_back(std::move(f));}
static Program loadProgram(const fs::path&root,std::set<fs::path>&seen){auto canon=fs::weakly_canonical(root);if(seen.contains(canon))return {};seen.insert(canon);auto src=readFile(canon);auto prog=Parser(Lexer(src).tokenize()).parseProgram();Program all;for(auto&imp:prog.imports){if(imp.rfind("std.",0)==0)continue;fs::path child=imp; if(child.extension().empty())child += ".nx";if(child.is_relative())child=canon.parent_path()/child;appendProgram(all,loadProgram(child,seen));}appendProgram(all,std::move(prog));return all;}
static fs::path resolveInput(const std::string&arg){if(!arg.empty())return arg;auto e=projectEntry();if(e.empty())throw std::runtime_error("no input file; pass a .nx file or create nexus.toml");return e;}
static fs::path runtimeSource(){
    if(const char* env=std::getenv("NEXUS_RUNTIME_DIR")){fs::path p=fs::path(env)/"runtime.c";if(fs::exists(p))return p;}
    if(!gExecutable.empty()){
        fs::path exe=fs::weakly_canonical(gExecutable);
        fs::path bin=exe.parent_path();
        fs::path candidates[] = {bin/"../runtime/runtime.c", bin/"../share/nexus/runtime/runtime.c", bin/"runtime/runtime.c", bin/"../../runtime/runtime.c"};
        for(const auto& p:candidates){if(fs::exists(p))return fs::weakly_canonical(p);}
    }
    fs::path local=fs::current_path()/"runtime/runtime.c";if(fs::exists(local))return local;
#ifdef NEXUS_INSTALL_RUNTIME_DIR
    fs::path installed=fs::path(NEXUS_INSTALL_RUNTIME_DIR)/"runtime.c";if(fs::exists(installed))return installed;
#endif
    throw std::runtime_error("Nexus runtime not found; set NEXUS_RUNTIME_DIR or install the runtime files");
}
static int compileFile(const fs::path&in,const fs::path&out,const std::string&target,bool emitIR,int opt){std::set<fs::path> seen;auto prog=loadProgram(in,seen);SemanticAnalyzer().analyze(prog);CodegenOptions cg{targetTriple(target),opt};auto ir=LLVMIRGenerator(cg).generate(prog);fs::path irPath=out;irPath += ".ll";std::ofstream irf(irPath);if(!irf)throw std::runtime_error("cannot write "+irPath.string());irf<<ir;irf.close();if(emitIR){std::cout<<ir;return 0;}std::string clang="clang";std::ostringstream cmd;cmd<<clang<<' '<<"-Wno-override-module ";if(!target.empty()&&target!="native")cmd<<"--target="<<targetTriple(target)<<' ';cmd<<irPath.string()<<" "<<runtimeSource().string()<<" -O"<<opt<<" -o "<<out.string();int rc=std::system(cmd.str().c_str());if(rc!=0)throw std::runtime_error("native backend failed; the target toolchain or sysroot may be unavailable");return 0;}
static int initProject(const fs::path&dir,const std::string&name,const std::string&templ="cli"){
    fs::create_directories(dir/"src");
    fs::create_directories(dir/"assets");
    std::string source;
    if(templ=="game") source=R"NX(fn main() {
    screen_set_title("NEXUS Game")
    screen_begin()
    let mut x: i64 = 10
    let mut y: i64 = 5
    let mut score: i64 = 0
    let mut running = true

    while running {
        screen_clear()
        screen_put(0, 0, "NEXUS GAME — WASD move, Q quit")
        screen_put(x, y, "@")
        screen_put(0, 22, "Score: " + str_i64(score))
        screen_present()

        if key_pressed() {
            let key = read_key()
            if key == 119 { y = y - 1 }
            else if key == 115 { y = y + 1 }
            else if key == 97 { x = x - 1 }
            else if key == 100 { x = x + 1 }
            else if key == 113 { running = false }
            score = score + 1
        }
        sleep(30)
    }

    screen_end()
    print("Game over")
    print(score)
}
)NX";
    else if(templ=="app") source=R"NX(fn greet() {
    let name = input("Your name: ")
    print("Hello, " + name)
}

fn calculator() {
    let a = input_i64("A: ")
    let b = input_i64("B: ")
    print("Sum: " + str_i64(a + b))
    print("Product: " + str_i64(a * b))
}

fn main() {
    let mut running = true
    while running {
        print("=== NEXUS APP ===")
        print("1 - Greeting")
        print("2 - Calculator")
        print("3 - Random number")
        print("0 - Exit")
        let choice = input_i64("> ")
        if choice == 1 { greet() }
        else if choice == 2 { calculator() }
        else if choice == 3 { print(random_i64(1, 100)) }
        else if choice == 0 { running = false }
        else { print("Unknown command") }
    }
}
)NX";
    else source=R"NX(fn main() {
    print("Hello from NEXUS!")
    let name = input("Name: ")
    print("Welcome, " + name)
}
)NX";
    std::ofstream(dir/"src/main.nx")<<source;
    std::ofstream(dir/"nexus.toml")<<"[package]\nname = \""<<(name.empty()?dir.filename().string():name)<<"\"\nversion = \"0.4.0\"\nedition = \"2026\"\n\n[build]\nentry = \"src/main.nx\"\n\n[dependencies]\n";
    std::ofstream(dir/".gitignore")<<"build/\n*.ll\n*.o\n";
    std::ofstream(dir/"README.md")<<"# "<<(name.empty()?dir.filename().string():name)<<"\n\nGenerated by NEXUS 0.4.0 (`"<<templ<<"` template).\n\nBuild: `nexus build`\nRun: `nexus run`\n";
    return 0;
}
int run(int argc,char**argv){try{gExecutable=argc>0?fs::absolute(argv[0]):fs::path();if(argc<2){usage();return 1;}std::string cmd=argv[1];if(cmd=="help"||cmd=="--help"){usage();return 0;}if(cmd=="version"||cmd=="--version"){std::cout<<"Nexus 0.4.0 (C++20/LLVM IR + Interactive Runtime)\n";return 0;}if(cmd=="targets"){std::cout<<"native\nx86_64-linux\naarch64-linux\nx86_64-windows\naarch64-windows\nx86_64-macos\naarch64-macos\naarch64-android\nwasm32-wasi\n";return 0;}if(cmd=="new"){if(argc<3)throw std::runtime_error("new requires a project name");std::string templ="cli";for(int i=3;i<argc;++i){std::string a=argv[i];if(a=="--template"&&i+1<argc)templ=argv[++i];else if(a.rfind("--template=",0)==0)templ=a.substr(11);else throw std::runtime_error("unknown option: "+a);}if(templ!="cli"&&templ!="app"&&templ!="game")throw std::runtime_error("unknown template: "+templ);return initProject(argv[2],argv[2],templ);}if(cmd=="init"){return initProject(".",fs::current_path().filename().string(),"cli");}if(cmd=="check"){auto in=resolveInput(argc>=3?argv[2]:"");std::set<fs::path> seen;auto prog=loadProgram(in,seen);SemanticAnalyzer().analyze(prog);std::cout<<"check: OK\n";return 0;}if(cmd=="build"||cmd=="run"){int argi=2;std::string input;if(argi<argc&&argv[argi][0]!='-')input=argv[argi++];auto in=resolveInput(input);std::string out=fs::path(in).stem().string(),target="native";bool emitIR=false;int opt=2;for(int i=argi;i<argc;++i){std::string a=argv[i];if(a=="--emit-ir")emitIR=true;else if(a=="--release")opt=3;else if(a=="-o"&&i+1<argc)out=argv[++i];else if((a=="--target"||a.rfind("--target=",0)==0)){if(a=="--target")target=argv[++i];else target=a.substr(9);}else throw std::runtime_error("unknown option: "+a);}compileFile(in,out,target,emitIR,opt);if(cmd=="run"){fs::path outPath(out);std::string exec=(outPath.is_absolute()?outPath.string():"./"+outPath.string());return std::system(exec.c_str());}return 0;}usage();return 1;}catch(const std::exception&e){std::cerr<<"error[NX0000]: "<<e.what()<<"\n";return 2;}}
}
