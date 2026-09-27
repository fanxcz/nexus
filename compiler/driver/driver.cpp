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
static void usage(){std::cout<<"Nexus 0.7.0\nUsage: nexus <command> [options]\n\nCommands:\n  new <project> [--template cli|app|game|3d]\n  init\n  build [file.nx] [-o output] [--target TRIPLE] [--emit-ir] [--release]\n  run [file.nx] [--target TRIPLE]\n  check [file.nx]\n  targets\n  version\n  help\n\nExamples:\n  nexus new hello\n  nexus run examples/hello.nx\n  nexus build app.nx -o app --target x86_64-linux\n  nexus build app.nx --emit-ir\n\nPortable source is target-independent; target toolchains determine which native outputs can be linked on the current host.\n";}
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
        fs::path candidates[] = {bin/"../runtime/runtime.cpp", bin/"../share/nexus/runtime/runtime.cpp", bin/"runtime/runtime.cpp", bin/"../../runtime/runtime.cpp"};
        for(const auto& p:candidates){if(fs::exists(p))return fs::weakly_canonical(p);}
    }
    fs::path local=fs::current_path()/"runtime/runtime.cpp";if(fs::exists(local))return local;
#ifdef NEXUS_INSTALL_RUNTIME_DIR
    fs::path installed=fs::path(NEXUS_INSTALL_RUNTIME_DIR)/"runtime.cpp";if(fs::exists(installed))return installed;
#endif
    throw std::runtime_error("Nexus runtime not found; set NEXUS_RUNTIME_DIR or install the runtime files");
}
static int compileFile(const fs::path&in,const fs::path&out,const std::string&target,bool emitIR,int opt){std::set<fs::path> seen;auto prog=loadProgram(in,seen);SemanticAnalyzer().analyze(prog);CodegenOptions cg{targetTriple(target),opt};auto ir=LLVMIRGenerator(cg).generate(prog);fs::path irPath=out;irPath += ".ll";std::ofstream irf(irPath);if(!irf)throw std::runtime_error("cannot write "+irPath.string());irf<<ir;irf.close();if(emitIR){std::cout<<ir;return 0;}std::string clang="clang++";std::ostringstream cmd;cmd<<clang<<' '<<"-std=c++20 -Wno-override-module ";
#ifdef __linux__
    cmd<<"-ldl ";
#endif
    if(!target.empty()&&target!="native")cmd<<"--target="<<targetTriple(target)<<' ';cmd<<irPath.string()<<" "<<runtimeSource().string()<<" -O"<<opt<<" -o "<<out.string();int rc=std::system(cmd.str().c_str());if(rc!=0)throw std::runtime_error("native backend failed; the target toolchain or sysroot may be unavailable");return 0;}
static int initProject(const fs::path&dir,const std::string&name,const std::string&templ="cli"){
    fs::create_directories(dir/"src");
    fs::create_directories(dir/"assets");
    std::string source;
    if(templ=="game") source=R"NX(fn main() {
    let ok = gfx_init(960, 540, "NEXUS Game")
    if !ok {
        print("Graphics backend unavailable: " + gfx_error())
        return
    }

    let mut playerX: f64 = 460.0
    let mut playerY: f64 = 250.0
    let mut score: i64 = 0
    let mut enemyX: f64 = 720.0
    let mut enemyY: f64 = 250.0

    while !gfx_should_close() {
        gfx_poll()
        let dt = gfx_dt()
        let speed = 240.0 * dt

        if gfx_key_down("W") { playerY = playerY - speed }
        if gfx_key_down("S") { playerY = playerY + speed }
        if gfx_key_down("A") { playerX = playerX - speed }
        if gfx_key_down("D") { playerX = playerX + speed }
        if gfx_key_down("ESC") { break }

        let mx = gfx_mouse_x()
        let my = gfx_mouse_y()
        if gfx_mouse_down(1) { enemyX = mx; enemyY = my; score = score + 1 }

        if playerX < 20.0 { playerX = 20.0 }
        if playerY < 20.0 { playerY = 20.0 }
        if playerX > 920.0 { playerX = 920.0 }
        if playerY > 500.0 { playerY = 500.0 }

        gfx_begin()
        gfx_clear(0.03, 0.04, 0.08, 1.0)
        gfx_rect(0.0, 0.0, 960.0, 64.0, 0.06, 0.08, 0.15, 1.0)
        gfx_text(24.0, 20.0, 2.0, "NEXUS GAME", 0.5, 0.85, 1.0, 1.0)
        gfx_text(240.0, 22.0, 1.5, "WASD MOVE   LEFT MOUSE TELEPORT ENEMY   ESC QUIT", 0.85, 0.85, 0.9, 1.0)

        gfx_circle(playerX, playerY, 24.0, 0.2, 0.8, 1.0, 1.0)
        gfx_circle(enemyX, enemyY, 18.0, 1.0, 0.25, 0.2, 1.0)
        gfx_line(playerX, playerY, enemyX, enemyY, 2.0, 0.9, 0.9, 0.3, 0.5)

        gfx_text(24.0, 500.0, 1.5, "SCORE", 0.6, 0.7, 0.9, 1.0)
        gfx_text(95.0, 500.0, 1.5, str_i64(score), 1.0, 1.0, 1.0, 1.0)
        gfx_text(720.0, 500.0, 1.2, "NEXUS 0.7 GAME RUNTIME", 0.5, 0.6, 0.8, 1.0)
        gfx_end()
    }

    gfx_shutdown()
    print("Game closed")
}
)NX";
    else if(templ=="app") source=R"NX(fn main() {
    let ok = gfx_init(900, 560, "NEXUS App")
    if !ok { print("Graphics backend unavailable: " + gfx_error()); return }
    let mut clicks: i64 = 0
    while !gfx_should_close() {
        gfx_poll()
        gfx_begin()
        gfx_clear(0.04, 0.05, 0.09, 1.0)
        gfx_rect(40.0, 40.0, 820.0, 90.0, 0.08, 0.12, 0.22, 1.0)
        gfx_text(70.0, 65.0, 3.0, "NEXUS APP", 0.5, 0.9, 1.0, 1.0)
        gfx_text(70.0, 115.0, 1.5, "NATIVE WINDOW / MOUSE / EVENTS", 0.75, 0.8, 0.9, 1.0)
        gfx_rect(70.0, 190.0, 260.0, 100.0, 0.12, 0.5, 0.9, 1.0)
        gfx_text(110.0, 225.0, 2.0, "CLICK ME", 1.0, 1.0, 1.0, 1.0)
        gfx_text(70.0, 360.0, 1.5, "CLICKS:", 0.6, 0.7, 0.85, 1.0)
        gfx_text(170.0, 360.0, 1.5, str_i64(clicks), 1.0, 1.0, 1.0, 1.0)
        if gfx_mouse_down(1) { clicks = clicks + 1 }
        gfx_end()
    }
    gfx_shutdown()
}
)NX";
    else if(templ=="3d") source=R"NX(fn main() {
    let ok = gfx_init(960, 540, "NEXUS 3D")
    if !ok { print("Graphics backend unavailable: " + gfx_error()); return }
    while !gfx_should_close() {
        gfx_poll()
        gfx3d_begin(70.0, 0.1, 1000.0, 0.0, 2.4, 8.0, 20.0, 0.0, 0.0)
        gfx_clear(0.02, 0.025, 0.05, 1.0)
        gfx3d_grid(12, 1.0, 0.16, 0.22, 0.32, 1.0)
        gfx3d_cube(0.0, 1.0, 0.0, 1.5, 1.5, 1.5, 0.2, 0.75, 1.0, 1.0)
        gfx3d_cube(3.0, 0.5, -2.0, 1.0, 1.0, 1.0, 1.0, 0.25, 0.2, 1.0)
        gfx3d_end()
    }
    gfx_shutdown()
}
)NX";
    else source=R"NX(fn main() {
    print("Hello from NEXUS!")
    let name = input("Name: ")
    print("Welcome, " + name)
}
)NX";
    std::ofstream(dir/"src/main.nx")<<source;
    std::ofstream(dir/"nexus.toml")<<"[package]\nname = \""<<(name.empty()?dir.filename().string():name)<<"\"\nversion = \"0.7.0\"\nedition = \"2026\"\n\n[build]\nentry = \"src/main.nx\"\n\n[dependencies]\n";
    std::ofstream(dir/".gitignore")<<"build/\n*.ll\n*.o\n";
    std::ofstream(dir/"README.md")<<"# "<<(name.empty()?dir.filename().string():name)<<"\n\nGenerated by NEXUS 0.7.0 (`"<<templ<<"` template).\n\nBuild: `nexus build`\nRun: `nexus run`\n";
    return 0;
}
int run(int argc,char**argv){try{gExecutable=argc>0?fs::absolute(argv[0]):fs::path();if(argc<2){usage();return 1;}std::string cmd=argv[1];if(cmd=="help"||cmd=="--help"){usage();return 0;}if(cmd=="version"||cmd=="--version"){std::cout<<"Nexus 0.7.0 (C++20/LLVM IR + Native Graphics Runtime)\n";return 0;}if(cmd=="targets"){std::cout<<"native\nx86_64-linux\naarch64-linux\nx86_64-windows\naarch64-windows\nx86_64-macos\naarch64-macos\naarch64-android\nwasm32-wasi\n";return 0;}if(cmd=="new"){if(argc<3)throw std::runtime_error("new requires a project name");std::string templ="cli";for(int i=3;i<argc;++i){std::string a=argv[i];if(a=="--template"&&i+1<argc)templ=argv[++i];else if(a.rfind("--template=",0)==0)templ=a.substr(11);else throw std::runtime_error("unknown option: "+a);}if(templ!="cli"&&templ!="app"&&templ!="game"&&templ!="3d")throw std::runtime_error("unknown template: "+templ);return initProject(argv[2],argv[2],templ);}if(cmd=="init"){return initProject(".",fs::current_path().filename().string(),"cli");}if(cmd=="check"){auto in=resolveInput(argc>=3?argv[2]:"");std::set<fs::path> seen;auto prog=loadProgram(in,seen);SemanticAnalyzer().analyze(prog);std::cout<<"check: OK\n";return 0;}if(cmd=="build"||cmd=="run"){int argi=2;std::string input;if(argi<argc&&argv[argi][0]!='-')input=argv[argi++];auto in=resolveInput(input);std::string out=fs::path(in).stem().string(),target="native";bool emitIR=false;int opt=2;for(int i=argi;i<argc;++i){std::string a=argv[i];if(a=="--emit-ir")emitIR=true;else if(a=="--release")opt=3;else if(a=="-o"&&i+1<argc)out=argv[++i];else if((a=="--target"||a.rfind("--target=",0)==0)){if(a=="--target")target=argv[++i];else target=a.substr(9);}else throw std::runtime_error("unknown option: "+a);}compileFile(in,out,target,emitIR,opt);if(cmd=="run"){fs::path outPath(out);std::string exec=(outPath.is_absolute()?outPath.string():"./"+outPath.string());return std::system(exec.c_str());}return 0;}usage();return 1;}catch(const std::exception&e){std::cerr<<"error[NX0000]: "<<e.what()<<"\n";return 2;}}
}
