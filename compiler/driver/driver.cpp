#include "driver/driver.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic.hpp"
#include "codegen/llvm_ir.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>

namespace nexus {
namespace fs = std::filesystem;

namespace {
constexpr const char* kVersion = "1.0.0";
fs::path gExecutable;

std::string readFile(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open " + p.string());
    return {std::istreambuf_iterator<char>(f), {}};
}

std::string trim(std::string s) {
    const auto first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

std::string tomlString(const std::string& data, const std::string& section, const std::string& key) {
    std::istringstream in(data);
    std::string line;
    std::string current;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        if (line.front() == '[' && line.back() == ']') {
            current = trim(line.substr(1, line.size() - 2));
            continue;
        }
        if (current != section) continue;
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        const auto lhs = trim(line.substr(0, eq));
        if (lhs != key) continue;
        const auto rhs = trim(line.substr(eq + 1));
        if (rhs.size() >= 2 && rhs.front() == '"' && rhs.back() == '"') return rhs.substr(1, rhs.size() - 2);
        return rhs;
    }
    return {};
}

int64_t tomlInt(const std::string& data, const std::string& section, const std::string& key, int64_t fallback) {
    const std::string value = tomlString(data, section, key);
    if (value.empty()) return fallback;
    try { return std::stoll(value); } catch (...) { return fallback; }
}

struct ProjectConfig {
    fs::path root;
    std::string name;
    std::string version;
    std::string entry;
    std::string artifact;
    std::string target;
    int64_t windowWidth = 1280;
    int64_t windowHeight = 720;
    std::string windowTitle;
};

fs::path findProjectRoot(fs::path start) {
    std::error_code ec;
    start = fs::weakly_canonical(start, ec);
    if (ec) start = fs::absolute(start, ec);
    if (fs::is_regular_file(start, ec)) start = start.parent_path();
    for (;;) {
        if (fs::exists(start / "nexus.toml")) return start;
        const auto parent = start.parent_path();
        if (parent == start) break;
        start = parent;
    }
    return {};
}

ProjectConfig loadProject(const fs::path& requestedRoot = fs::current_path()) {
    const fs::path root = findProjectRoot(requestedRoot);
    if (root.empty()) throw std::runtime_error("nexus.toml not found; run `nexus init` or pass a .nx file");
    const std::string data = readFile(root / "nexus.toml");
    ProjectConfig cfg;
    cfg.root = root;
    cfg.name = tomlString(data, "package", "name");
    cfg.version = tomlString(data, "package", "version");
    cfg.entry = tomlString(data, "build", "entry");
    cfg.artifact = tomlString(data, "build", "artifact");
    cfg.target = tomlString(data, "build", "target");
    cfg.windowWidth = tomlInt(data, "window", "width", 1280);
    cfg.windowHeight = tomlInt(data, "window", "height", 720);
    cfg.windowTitle = tomlString(data, "window", "title");
    if (cfg.name.empty()) cfg.name = root.filename().string();
    if (cfg.version.empty()) cfg.version = kVersion;
    if (cfg.entry.empty()) cfg.entry = "src/main.nx";
    if (cfg.artifact.empty()) cfg.artifact = cfg.name;
    if (cfg.windowTitle.empty()) cfg.windowTitle = cfg.name;
    return cfg;
}

std::string shellQuote(const std::string& s) {
#ifdef _WIN32
    std::string out = "\"";
    for (const char c : s) { if (c == '"') out += '\\'; out += c; }
    out += '"';
    return out;
#else
    std::string out = "'";
    for (const char c : s) { if (c == '\'') out += "'\\''"; else out += c; }
    out += '\'';
    return out;
#endif
}

void usage() {
    std::cout
        << "Nexus " << kVersion << "\n"
        << "Usage: nexus <command> [options]\n\n"
        << "Project commands:\n"
        << "  new <project> [--template cli|app|game|3d|server|library]\n"
        << "  init [--template cli|app|game|3d|server|library]\n"
        << "  info\n"
        << "  assets\n"
        << "  clean\n"
        << "  doctor\n"
        << "  editor [--gui]\n\n"
        << "Build commands:\n"
        << "  build [file.nx] [-o output] [--target TRIPLE] [--emit-ir] [--release]\n"
        << "  run [file.nx] [--target TRIPLE] [--release]\n"
        << "  check [file.nx]\n"
        << "  targets\n"
        << "  fmt [file.nx]\n"
        << "  lint [file.nx]\n"
        << "  doc\n"
        << "  bench [file.nx] [--runs N]\n"
        << "  test\n"
        << "  pkg <init|add|remove|list|update|build|publish> [args]\n"
        << "  lsp\n"
        << "  version\n"
        << "  help\n\n"
        << "Examples:\n"
        << "  nexus new NeonGame --template game\n"
        << "  cd NeonGame && nexus build && nexus run\n"
        << "  nexus check src/main.nx\n"
        << "  nexus build src/main.nx -o app --target x86_64-linux\n";
}

std::string targetTriple(const std::string& t) {
    if (t.empty() || t == "native") return {};
    if (t == "x86_64-linux") return "x86_64-unknown-linux-gnu";
    if (t == "aarch64-linux") return "aarch64-unknown-linux-gnu";
    if (t == "x86_64-windows") return "x86_64-pc-windows-gnu";
    if (t == "aarch64-windows") return "aarch64-pc-windows-msvc";
    if (t == "x86_64-macos") return "x86_64-apple-darwin";
    if (t == "aarch64-macos") return "arm64-apple-darwin";
    if (t == "aarch64-android") return "aarch64-linux-android";
    if (t == "wasm32-wasi") return "wasm32-wasi";
    return t;
}

void appendProgram(Program& dst, Program src) {
    dst.imports.insert(dst.imports.end(), src.imports.begin(), src.imports.end());
    for (auto& s : src.structs) dst.structs.push_back(std::move(s));
    for (auto& e : src.enums) dst.enums.push_back(std::move(e));
    for (auto& f : src.functions) dst.functions.push_back(std::move(f));
}

Program loadProgram(const fs::path& root, std::set<fs::path>& seen) {
    const auto canon = fs::weakly_canonical(root);
    if (seen.contains(canon)) return {};
    seen.insert(canon);
    const auto src = readFile(canon);
    auto prog = Parser(Lexer(src).tokenize()).parseProgram();
    Program all;
    for (const auto& imp : prog.imports) {
        fs::path child;
        if (imp.rfind("std.", 0) == 0) {
            const std::string mod = imp.substr(4);
            std::vector<fs::path> candidates;
            candidates.push_back(canon.parent_path() / "std" / (mod + ".nx"));
#ifdef NEXUS_SOURCE_ROOT
            candidates.push_back(fs::path(NEXUS_SOURCE_ROOT) / "std" / (mod + ".nx"));
#endif
            if (!gExecutable.empty()) {
                const auto bin = fs::weakly_canonical(gExecutable).parent_path();
                candidates.push_back(bin / "../share/nexus/std" / (mod + ".nx"));
                candidates.push_back(bin / "../../std" / (mod + ".nx"));
            }
            for (const auto& c : candidates) if (fs::exists(c)) { child = c; break; }
            if (child.empty()) throw std::runtime_error("standard module not found: " + imp);
        } else {
            child = imp;
            if (child.extension().empty()) child += ".nx";
            if (child.is_relative()) {
                const auto local = canon.parent_path() / child;
                const auto vendorName = child.stem();
                std::vector<fs::path> candidates;
                candidates.push_back(local);
                const auto pkgRoot = findProjectRoot(canon);
                if (!pkgRoot.empty()) {
                    candidates.push_back(pkgRoot / "vendor" / vendorName / "src/main.nx");
                    candidates.push_back(pkgRoot / "vendor" / child);
                }
                candidates.push_back(canon.parent_path() / "vendor" / vendorName / "src/main.nx");
                candidates.push_back(canon.parent_path() / "vendor" / child);
                for (const auto& c : candidates) if (fs::exists(c)) { child = c; break; }
            }
        }
        appendProgram(all, loadProgram(child, seen));
    }
    appendProgram(all, std::move(prog));
    return all;
}

fs::path resolveInput(const std::string& arg) {
    if (!arg.empty()) {
        fs::path p = arg;
        if (p.is_relative()) p = fs::absolute(p);
        return p;
    }
    const ProjectConfig cfg = loadProject();
    return cfg.root / cfg.entry;
}

fs::path templateSource(const std::string& templ) {
    std::vector<fs::path> candidates = {
        fs::current_path() / "templates" / (templ + ".nx"),
        fs::current_path() / "examples" / (templ == "game" ? "neon_survivor.nx" : templ + ".nx")
    };
    if (!gExecutable.empty()) {
        const fs::path bin = fs::weakly_canonical(gExecutable).parent_path();
#ifdef NEXUS_SOURCE_ROOT
        candidates.push_back(fs::path(NEXUS_SOURCE_ROOT) / "templates" / (templ + ".nx"));
        candidates.push_back(fs::path(NEXUS_SOURCE_ROOT) / "examples" / (templ == "game" ? "neon_survivor.nx" : templ + ".nx"));
#endif
        candidates.push_back(bin / "../share/nexus/templates" / (templ + ".nx"));
        candidates.push_back(bin / "../share/nexus/examples" / (templ == "game" ? "neon_survivor.nx" : templ + ".nx"));
        candidates.push_back(bin / "../../templates" / (templ + ".nx"));
        candidates.push_back(bin / "../../examples" / (templ == "game" ? "neon_survivor.nx" : templ + ".nx"));
    }
    for (const auto& p : candidates) if (fs::exists(p)) return fs::weakly_canonical(p);
    return {};
}

fs::path runtimeSource() {
    if (const char* env = std::getenv("NEXUS_RUNTIME_DIR")) {
        fs::path p = fs::path(env) / "runtime.cpp";
        if (fs::exists(p)) return p;
        p = fs::path(env) / "runtime.c";
        if (fs::exists(p)) return p;
    }
    if (!gExecutable.empty()) {
        const auto bin = fs::weakly_canonical(gExecutable).parent_path();
        const std::vector<fs::path> candidates = {
#ifdef NEXUS_SOURCE_ROOT
            fs::path(NEXUS_SOURCE_ROOT) / "runtime/runtime.cpp",
#endif
            bin / "../runtime/runtime.cpp", bin / "../share/nexus/runtime/runtime.cpp",
            bin / "runtime/runtime.cpp", bin / "../../runtime/runtime.cpp", bin / "../runtime/runtime.c"
        };
        for (const auto& p : candidates) if (fs::exists(p)) return fs::weakly_canonical(p);
    }
    const fs::path local = fs::current_path() / "runtime/runtime.cpp";
    if (fs::exists(local)) return local;
#ifdef NEXUS_INSTALL_RUNTIME_DIR
    const fs::path installed = fs::path(NEXUS_INSTALL_RUNTIME_DIR) / "runtime.cpp";
    if (fs::exists(installed)) return installed;
#endif
    throw std::runtime_error("Nexus runtime not found; set NEXUS_RUNTIME_DIR or install the runtime files");
}

fs::path defaultOutput(const fs::path& input, const std::string& explicitOutput, bool release) {
    if (!explicitOutput.empty()) return fs::path(explicitOutput);
    const fs::path projectRoot = findProjectRoot(input.parent_path());
    if (!projectRoot.empty() && fs::exists(projectRoot / "nexus.toml")) {
        const auto cfg = loadProject(projectRoot);
        return projectRoot / "build" / (release ? "release" : "debug") / cfg.artifact;
    }
    return fs::current_path() / input.stem();
}

int compileFile(const fs::path& in, const fs::path& out, const std::string& target, bool emitIR, int opt) {
    std::set<fs::path> seen;
    auto prog = loadProgram(in, seen);
    SemanticAnalyzer().analyze(prog);
    CodegenOptions cg{targetTriple(target), opt};
    const auto ir = LLVMIRGenerator(cg).generate(prog);
    fs::create_directories(out.parent_path().empty() ? fs::current_path() : out.parent_path());
    fs::path irPath = out;
    irPath += ".ll";
    std::ofstream irf(irPath, std::ios::binary);
    if (!irf) throw std::runtime_error("cannot write " + irPath.string());
    irf << ir;
    irf.close();
    if (emitIR) {
        std::cout << ir;
        return 0;
    }
    std::ostringstream cmd;
    cmd << "clang++ -std=c++20 -Wno-override-module ";
#ifdef __linux__
    cmd << "-ldl ";
#endif
    if (!target.empty() && target != "native") cmd << "--target=" << shellQuote(targetTriple(target)) << ' ';
    cmd << shellQuote(irPath.string()) << ' ' << shellQuote(runtimeSource().string())
        << " -O" << opt << " -o " << shellQuote(out.string());
    const int rc = std::system(cmd.str().c_str());
    if (rc != 0) throw std::runtime_error("native backend failed; the target toolchain or sysroot may be unavailable");
    return 0;
}

void writeProjectManifest(const fs::path& dir, const std::string& name, const std::string& templ) {
    std::ofstream(dir / "nexus.toml")
        << "[package]\n"
        << "name = \"" << name << "\"\n"
        << "version = \"" << kVersion << "\"\n"
        << "edition = \"2026\"\n\n"
        << "[build]\n"
        << "entry = \"src/main.nx\"\n"
        << "artifact = \"" << name << "\"\n"
        << "target = \"native\"\n\n"
        << "[window]\n"
        << "width = 1280\nheight = 720\ntitle = \"" << name << "\"\n\n"
        << "[assets]\ntextures = \"assets/textures\"\naudio = \"assets/audio\"\nmodels = \"assets/models\"\nfonts = \"assets/fonts\"\nscenes = \"scenes\"\n\n"
        << "[project]\ntemplate = \"" << templ << "\"\n";
}

fs::path sourceDataRoot() {
#ifdef NEXUS_SOURCE_ROOT
    const fs::path sourceRoot = NEXUS_SOURCE_ROOT;
    if (fs::exists(sourceRoot / "examples/assets") || fs::exists(sourceRoot / "templates")) return sourceRoot;
#endif
    if (!gExecutable.empty()) {
        const auto bin = fs::weakly_canonical(gExecutable).parent_path();
        const fs::path candidates[] = { fs::current_path(), bin / "..", bin / "../..", bin / "../share/nexus" };
        for (const auto& p : candidates) {
            if (fs::exists(p / "examples/assets") || fs::exists(p / "assets") || fs::exists(p / "templates")) return fs::weakly_canonical(p);
        }
    }
    return fs::current_path();
}

void seedAssets(const fs::path& dir) {
    const fs::path sourceRoot = sourceDataRoot();
    const fs::path assetRoot = fs::exists(sourceRoot / "examples/assets")
        ? sourceRoot / "examples/assets"
        : sourceRoot / "assets";
    const std::vector<std::pair<fs::path, fs::path>> assets = {
        {assetRoot / "beep.wav", dir / "assets/audio/beep.wav"},
        {assetRoot / "cube.obj", dir / "assets/models/cube.obj"},
        {assetRoot / "sprite_sheet.bmp", dir / "assets/textures/sprite_sheet.bmp"}
    };
    for (const auto& [src, dst] : assets) {
        if (fs::exists(src)) {
            std::error_code ec;
            fs::copy_file(src, dst, fs::copy_options::overwrite_existing, ec);
        }
    }
}

int initProject(const fs::path& dir, const std::string& name, const std::string& templ, bool failIfNonEmpty) {
    if (fs::exists(dir / "nexus.toml")) {
        if (failIfNonEmpty) throw std::runtime_error("project already exists: " + dir.string());
    } else if (failIfNonEmpty && fs::exists(dir) && !fs::is_empty(dir)) {
        throw std::runtime_error("directory is not empty: " + dir.string());
    }
    fs::create_directories(dir / "src");
    fs::create_directories(dir / "assets/textures");
    fs::create_directories(dir / "assets/audio");
    fs::create_directories(dir / "assets/models");
    fs::create_directories(dir / "assets/fonts");
    fs::create_directories(dir / "scenes");

    const fs::path templatePath = templateSource(templ);
    const std::string source = templatePath.empty()
        ? "fn main() { print(\"Hello from NEXUS!\") }\n"
        : readFile(templatePath);
    if (!fs::exists(dir / "src/main.nx")) std::ofstream(dir / "src/main.nx") << source;
    if (!fs::exists(dir / "scenes/main.nxs")) {
        std::ofstream(dir / "scenes/main.nxs") << "NXS1\nscene = Main\nversion = 1\n";
    }
    writeProjectManifest(dir, name.empty() ? dir.filename().string() : name, templ);
    seedAssets(dir);
    std::ofstream(dir / ".gitignore") << "build/\n*.ll\n*.o\n.cache/\n";
    std::ofstream(dir / "README.md")
        << "# " << (name.empty() ? dir.filename().string() : name) << "\n\n"
        << "Generated by NEXUS " << kVersion << " (`" << templ << "` template).\n\n"
        << "Build: `nexus build`\n"
        << "Run: `nexus run`\n"
        << "Check: `nexus check`\n"
        << "Assets: `nexus assets`\n"
        << "Info: `nexus info`\n";
    return 0;
}

int commandInfo() {
    const auto cfg = loadProject();
    std::cout << "NEXUS project\n"
              << "  name:    " << cfg.name << "\n"
              << "  version: " << cfg.version << "\n"
              << "  root:    " << cfg.root << "\n"
              << "  entry:   " << cfg.entry << "\n"
              << "  artifact:" << cfg.artifact << "\n"
              << "  target:  " << cfg.target << "\n"
              << "  window:  " << cfg.windowWidth << "x" << cfg.windowHeight << "\n"
              << "  title:   " << cfg.windowTitle << "\n";
    return 0;
}

int commandAssets() {
    const auto cfg = loadProject();
    const std::vector<fs::path> dirs = {
        cfg.root / "assets/textures", cfg.root / "assets/audio", cfg.root / "assets/models",
        cfg.root / "assets/fonts", cfg.root / "scenes"
    };
    std::size_t count = 0;
    std::cout << "Assets for " << cfg.name << ":\n";
    for (const auto& dir : dirs) {
        if (!fs::exists(dir)) continue;
        for (const auto& e : fs::recursive_directory_iterator(dir)) {
            if (!e.is_regular_file()) continue;
            ++count;
            std::cout << "  " << fs::relative(e.path(), cfg.root).string() << "\n";
        }
    }
    std::cout << "Total: " << count << " file(s)\n";
    return 0;
}

int commandClean() {
    const auto cfg = loadProject();
    std::error_code ec;
    fs::remove_all(cfg.root / "build", ec);
    if (ec) throw std::runtime_error("cannot remove build directory: " + ec.message());
    std::cout << "clean: OK\n";
    return 0;
}

int commandDoctor() {
    const int clangOk = std::system("clang++ --version > /dev/null 2>&1");
    std::cout << "NEXUS doctor\n"
              << "  compiler: " << (clangOk == 0 ? "OK" : "MISSING") << "\n"
              << "  runtime:  " << (fs::exists(runtimeSource()) ? "OK" : "MISSING") << "\n"
              << "  project:  " << (findProjectRoot(fs::current_path()).empty() ? "not in a project" : "OK") << "\n";
    if (clangOk != 0) throw std::runtime_error("clang++ is required for native code generation");
    return 0;
}


void writeText(const fs::path& p, const std::string& data) {
    fs::create_directories(p.parent_path().empty() ? fs::current_path() : p.parent_path());
    std::ofstream f(p, std::ios::binary);
    if (!f) throw std::runtime_error("cannot write " + p.string());
    f << data;
}

int commandFmt(const fs::path& file) {
    const std::string src = readFile(file);
    std::istringstream in(src);
    std::ostringstream out;
    std::string line;
    int indent = 0;
    bool touched = false;
    while (std::getline(in, line)) {
        while (!line.empty() && (line.back()==' ' || line.back()=='\t' || line.back()=='\r')) { line.pop_back(); touched=true; }
        std::string t = trim(line);
        if (t.empty()) { out << '\n'; continue; }
        if (!t.empty() && t.front()=='}') indent = std::max(0, indent-1);
        for (int i=0;i<indent;++i) out << "    ";
        out << t << '\n';
        if (!t.empty() && t.back()=='{' && !(t.rfind("//",0)==0)) ++indent;
        if (t.find("} else {") != std::string::npos) indent = std::max(0, indent);
    }
    const std::string formatted = out.str();
    if (formatted != src) { writeText(file, formatted); touched=true; }
    std::cout << "fmt: " << (touched ? "updated" : "already formatted") << " -> " << file << '\n';
    return 0;
}

int commandLint(const fs::path& file) {
    const std::string src = readFile(file);
    std::size_t warnings = 0, lineNo = 0;
    std::istringstream in(src);
    std::string line;
    while (std::getline(in,line)) {
        ++lineNo;
        if (!line.empty() && (line.back()==' ' || line.back()=='\t' || line.back()=='\r')) {
            ++warnings; std::cout << "warning[NX9001]: trailing whitespace at " << file << ':' << lineNo << '\n';
        }
        if (line.find("TODO") != std::string::npos || line.find("FIXME") != std::string::npos) {
            ++warnings; std::cout << "warning[NX9002]: unfinished marker at " << file << ':' << lineNo << '\n';
        }
    }
    std::set<fs::path> seen;
    auto prog = loadProgram(file, seen);
    SemanticAnalyzer().analyze(prog);
    std::cout << "lint: OK" << (warnings ? ", " + std::to_string(warnings) + " warning(s)" : "") << '\n';
    return 0;
}

int commandDoc() {
    const auto cfg = loadProject();
    const auto entry = cfg.root / cfg.entry;
    const std::string src = readFile(entry);
    std::istringstream in(src);
    std::ostringstream docs;
    docs << "# " << cfg.name << " API\n\nGenerated by NEXUS 1.0.0.\n\n";
    std::string pending;
    std::string line;
    while (std::getline(in,line)) {
        const auto t = trim(line);
        if (t.rfind("///",0)==0) { pending += trim(t.substr(3)) + "\n"; continue; }
        if (t.rfind("fn ",0)==0 || t.rfind("struct ",0)==0 || t.rfind("enum ",0)==0) {
            docs << "## " << t << "\n\n";
            if (!pending.empty()) { docs << pending << "\n"; pending.clear(); }
        }
    }
    const fs::path out = cfg.root / "docs/API.md";
    writeText(out, docs.str());
    std::cout << "doc: generated " << out << '\n';
    return 0;
}

int commandBench(const fs::path& file, int runs) {
    const auto out = fs::temp_directory_path() / (file.stem().string() + ".nexus-bench");
    compileFile(file, out, "native", false, 3);
    std::vector<long long> samples;
    samples.reserve(std::max(1,runs));
    for (int i=0;i<std::max(1,runs);++i) {
        const auto a = std::chrono::steady_clock::now();
        const std::string cmd = shellQuote(out.string());
        const int rc = std::system(cmd.c_str());
        const auto b = std::chrono::steady_clock::now();
        if (rc != 0) throw std::runtime_error("benchmark program exited with code " + std::to_string(rc));
        samples.push_back(std::chrono::duration_cast<std::chrono::microseconds>(b-a).count());
    }
    std::error_code ec; fs::remove(out,ec);
    long long total=0; for(auto x:samples) total+=x;
    std::cout << "bench: " << samples.size() << " run(s), average " << (total / samples.size()) << " us\n";
    return 0;
}


int commandTest() {
    const auto cfg = loadProject();
    const auto root = cfg.root / "tests/programs";
    if (!fs::exists(root)) { std::cout << "test: no tests/programs directory\n"; return 0; }
    std::size_t count=0;
    for (const auto& e : fs::recursive_directory_iterator(root)) {
        if (!e.is_regular_file() || e.path().extension() != ".nx") continue;
        ++count;
        const auto out = fs::temp_directory_path() / (e.path().stem().string()+".nexus-test");
        compileFile(e.path(),out,"native",false,2);
        const int rc=std::system(shellQuote(out.string()).c_str());
        std::error_code ec; fs::remove(out,ec);
        if(rc!=0) throw std::runtime_error("test failed: "+e.path().string());
        std::cout << "  PASS " << fs::relative(e.path(),cfg.root).string() << "\n";
    }
    std::cout << "test: " << count << " program(s) passed\n";
    return 0;
}

int commandEditor() {
    const auto cfg = loadProject();
    const auto vscode = cfg.root / ".vscode";
    fs::create_directories(vscode);
    writeText(vscode / "settings.json", R"JSON({
  "files.associations": { "*.nx": "rust" },
  "editor.tabSize": 4,
  "editor.insertSpaces": true
}
)JSON");
    writeText(vscode / "tasks.json", R"JSON({
  "version": "2.0.0",
  "tasks": [{
    "label": "NEXUS Build",
    "type": "shell",
    "command": "nexus build --release",
    "group": "build"
  }]
}
)JSON");
    std::cout << "editor: generated VS Code integration in .vscode/\n";
    std::cout << "editor: use `nexus lsp` as the language-server command\n";
    return 0;
}

void appendDependency(const fs::path& manifest, const std::string& name, const std::string& value) {
    std::string data = readFile(manifest);
    if (data.find("[dependencies]") == std::string::npos) data += "\n[dependencies]\n";
    if (data.find(name + " =") == std::string::npos) data += name + " = \"" + value + "\"\n";
    writeText(manifest, data);
}

int commandPkg(const std::vector<std::string>& args) {
    const auto cfg = loadProject();
    if (args.empty() || args[0]=="help") {
        std::cout << "nexus pkg init|add <path>|remove <name>|list|update|build|publish\n";
        return 0;
    }
    const std::string sub=args[0];
    const auto vendor=cfg.root/"vendor";
    if(sub=="init") {
        writeText(cfg.root/"nexus.lock", "# NEXUS lockfile v1\npackage = \""+cfg.name+"\"\nversion = \""+cfg.version+"\"\n");
        fs::create_directories(vendor); std::cout << "pkg init: OK\n"; return 0;
    }
    if(sub=="list") {
        std::cout << "Packages for "<<cfg.name<<":\n";
        if(fs::exists(vendor)) for(auto&e:fs::directory_iterator(vendor)) if(e.is_directory()) std::cout<<"  "<<e.path().filename().string()<<"\n";
        return 0;
    }
    if(sub=="add") {
        if(args.size()<2) throw std::runtime_error("pkg add requires a local package directory");
        fs::path src=args[1]; if(src.is_relative()) src=fs::absolute(src);
        if(!fs::exists(src/"nexus.toml")) throw std::runtime_error("package directory must contain nexus.toml");
        const auto pkg=loadProject(src); const auto dst=vendor/pkg.name; fs::remove_all(dst); fs::create_directories(dst);
        std::error_code ec; fs::copy(src,dst,fs::copy_options::recursive|fs::copy_options::overwrite_existing,ec); if(ec) throw std::runtime_error("cannot vendor package: "+ec.message());
        appendDependency(cfg.root/"nexus.toml", pkg.name, "vendor/"+pkg.name);
        writeText(cfg.root/"nexus.lock", "# NEXUS lockfile v1\n"+pkg.name+" = \""+pkg.version+"\"\n");
        std::cout << "pkg add: vendored "<<pkg.name<<" "<<pkg.version<<"\n"; return 0;
    }
    if(sub=="remove") {
        if(args.size()<2) throw std::runtime_error("pkg remove requires a package name");
        const auto dst=vendor/args[1]; std::error_code ec; fs::remove_all(dst,ec); std::cout<<"pkg remove: "<<args[1]<<"\n"; return 0;
    }
    if(sub=="update") { writeText(cfg.root/"nexus.lock", "# NEXUS lockfile v1\npackage = \""+cfg.name+"\"\nversion = \""+cfg.version+"\"\n"); std::cout<<"pkg update: lockfile refreshed\n"; return 0; }
    if(sub=="build") { const auto in=cfg.root/cfg.entry; const auto out=defaultOutput(in,"",true); compileFile(in,out,cfg.target,false,3); std::cout<<"pkg build: OK -> "<<out<<"\n"; return 0; }
    if(sub=="publish") {
        const auto dist=cfg.root/"dist"; fs::create_directories(dist); const auto archive=dist/(cfg.name+"-"+cfg.version+".nxpkg");
        std::string cmd="cmake -E tar cf "+shellQuote(archive.string())+" --format=zip";
        for (const auto& rel : {std::string("nexus.toml"),std::string("src"),std::string("assets"),std::string("scenes"),std::string("vendor"),std::string("docs")}) { if(fs::exists(cfg.root/rel)) cmd += " "+shellQuote(rel); }
        const int rc=std::system(("cd "+shellQuote(cfg.root.string())+" && "+cmd).c_str()); if(rc!=0) throw std::runtime_error("cannot create package archive");
        std::cout<<"pkg publish: created "<<archive<<"\n"; return 0;
    }
    throw std::runtime_error("unknown pkg command: "+sub);
}

int commandLsp() {
    std::string line, body;
    while(std::getline(std::cin,line)) {
        if(line.rfind("Content-Length:",0)==0) {
            const auto n=static_cast<std::size_t>(std::stoul(trim(line.substr(15))));
            std::getline(std::cin,line); body.assign(n,'\0'); std::cin.read(body.data(),std::streamsize(n));
            std::string id="1";
            const auto idpos=body.find("\"id\"");
            if(idpos!=std::string::npos){
                const auto colon=body.find(':',idpos);
                if(colon!=std::string::npos){std::size_t j=colon+1;while(j<body.size()&&std::isspace(static_cast<unsigned char>(body[j])))++j;std::size_t e=j;while(e<body.size()&&std::isdigit(static_cast<unsigned char>(body[e])))++e;if(e>j)id=body.substr(j,e-j);}
            }
            if(body.find("\"method\":\"initialize\"")!=std::string::npos) {
                const std::string result="{\"jsonrpc\":\"2.0\",\"id\":"+id+",\"result\":{\"capabilities\":{\"textDocumentSync\":1,\"documentFormattingProvider\":true,\"definitionProvider\":false,\"completionProvider\":{\"triggerCharacters\":[\".\"]}}}}";
                std::cout<<"Content-Length: "<<result.size()<<"\r\n\r\n"<<result<<std::flush;
            } else if(body.find("\"method\":\"shutdown\"")!=std::string::npos) {
                const std::string result="{\"jsonrpc\":\"2.0\",\"id\":"+id+",\"result\":null}";
                std::cout<<"Content-Length: "<<result.size()<<"\r\n\r\n"<<result<<std::flush;
            }
        }
    }
    return 0;
}

} // namespace

int run(int argc, char** argv) {
    try {
        gExecutable = argc > 0 ? fs::absolute(argv[0]) : fs::path();
        if (argc < 2) { usage(); return 1; }
        const std::string cmd = argv[1];

        if (cmd == "help" || cmd == "--help") { usage(); return 0; }
        if (cmd == "version" || cmd == "--version") {
            std::cout << "Nexus " << kVersion << " (C++20/LLVM IR + Native Graphics/Audio/Game Runtime)\n";
            return 0;
        }
        if (cmd == "targets") {
            std::cout << "native\nx86_64-linux\naarch64-linux\nx86_64-windows\naarch64-windows\n"
                         "x86_64-macos\naarch64-macos\naarch64-android\nwasm32-wasi\n";
            return 0;
        }

        if (cmd == "new") {
            if (argc < 3) throw std::runtime_error("new requires a project name");
            std::string templ = "cli";
            for (int i = 3; i < argc; ++i) {
                const std::string a = argv[i];
                if (a == "--template" && i + 1 < argc) templ = argv[++i];
                else if (a.rfind("--template=", 0) == 0) templ = a.substr(11);
                else throw std::runtime_error("unknown option: " + a);
            }
            if (templ != "cli" && templ != "app" && templ != "game" && templ != "3d" && templ != "server" && templ != "library") throw std::runtime_error("unknown template: " + templ);
            const fs::path projectPath = fs::absolute(argv[2]);
            return initProject(projectPath, projectPath.filename().string(), templ, true);
        }

        if (cmd == "init") {
            std::string templ = "cli";
            for (int i = 2; i < argc; ++i) {
                const std::string a = argv[i];
                if (a == "--template" && i + 1 < argc) templ = argv[++i];
                else if (a.rfind("--template=", 0) == 0) templ = a.substr(11);
                else throw std::runtime_error("unknown option: " + a);
            }
            if (templ != "cli" && templ != "app" && templ != "game" && templ != "3d" && templ != "server" && templ != "library") throw std::runtime_error("unknown template: " + templ);
            const fs::path root = fs::current_path();
            return initProject(root, root.filename().string(), templ, false);
        }

        if (cmd == "info") return commandInfo();
        if (cmd == "assets") return commandAssets();
        if (cmd == "clean") return commandClean();
        if (cmd == "doctor") return commandDoctor();

        if (cmd == "fmt" || cmd == "format") {
            const auto in = resolveInput(argc >= 3 && argv[2][0] != '-' ? argv[2] : "");
            return commandFmt(in);
        }
        if (cmd == "lint") {
            const auto in = resolveInput(argc >= 3 && argv[2][0] != '-' ? argv[2] : "");
            return commandLint(in);
        }
        if (cmd == "doc") return commandDoc();
        if (cmd == "bench") {
            const auto in = resolveInput(argc >= 3 && argv[2][0] != '-' ? argv[2] : "");
            int runs=5; for(int i=2;i<argc;++i){std::string a=argv[i]; if(a=="--runs"&&i+1<argc)runs=std::stoi(argv[++i]); else if(a.rfind("--runs=",0)==0)runs=std::stoi(a.substr(7));}
            return commandBench(in,runs);
        }
        if (cmd == "test") return commandTest();
        if (cmd == "editor") {
            bool gui=false; for(int i=2;i<argc;++i) if(std::string(argv[i])=="--gui") gui=true;
            if(!gui) return commandEditor();
            const auto tpl=templateSource("editor"); if(tpl.empty()) throw std::runtime_error("built-in editor template not found");
            const auto out=fs::temp_directory_path()/"nexus-editor"; compileFile(tpl,out,"native",false,2);
            return std::system(shellQuote(out.string()).c_str());
        }
        if (cmd == "lsp") return commandLsp();
        if (cmd == "pkg") { std::vector<std::string> args; for(int i=2;i<argc;++i) args.emplace_back(argv[i]); return commandPkg(args); }

        if (cmd == "check") {
            const auto in = resolveInput(argc >= 3 && argv[2][0] != '-' ? argv[2] : "");
            std::set<fs::path> seen;
            auto prog = loadProgram(in, seen);
            SemanticAnalyzer().analyze(prog);
            std::cout << "check: OK\n";
            return 0;
        }

        if (cmd == "build" || cmd == "run") {
            int argi = 2;
            std::string input;
            if (argi < argc && argv[argi][0] != '-') input = argv[argi++];
            ProjectConfig cfg;
            bool hasProject = false;
            try { cfg = loadProject(); hasProject = true; } catch (...) {}
            const auto in = resolveInput(input);
            std::string target = hasProject && input.empty() ? cfg.target : "native";
            std::string explicitOutput;
            bool emitIR = false;
            bool release = false;
            for (int i = argi; i < argc; ++i) {
                const std::string a = argv[i];
                if (a == "--emit-ir") emitIR = true;
                else if (a == "--release") release = true;
                else if (a == "-o" && i + 1 < argc) explicitOutput = argv[++i];
                else if (a.rfind("--target=", 0) == 0) target = a.substr(9);
                else if (a == "--target" && i + 1 < argc) target = argv[++i];
                else throw std::runtime_error("unknown option: " + a);
            }
            const auto out = defaultOutput(in, explicitOutput, release);
            const int rc = compileFile(in, out, target, emitIR, release ? 3 : 2);
            if (cmd == "run" && rc == 0) {
                const fs::path root = hasProject && input.empty() ? cfg.root : out.parent_path();
                const std::string command = "cd " + shellQuote(root.string()) + " && " + shellQuote(fs::absolute(out).string());
                return std::system(command.c_str());
            }
            std::cout << "build: OK -> " << out << "\n";
            return rc;
        }

        usage();
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "error[NX0000]: " << e.what() << "\n";
        return 2;
    }
}

} // namespace nexus
