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

namespace nexus {
namespace fs = std::filesystem;

namespace {
constexpr const char* kVersion = "0.9.0";
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
        << "  new <project> [--template cli|app|game|3d]\n"
        << "  init [--template cli|app|game|3d]\n"
        << "  info\n"
        << "  assets\n"
        << "  clean\n"
        << "  doctor\n\n"
        << "Build commands:\n"
        << "  build [file.nx] [-o output] [--target TRIPLE] [--emit-ir] [--release]\n"
        << "  run [file.nx] [--target TRIPLE] [--release]\n"
        << "  check [file.nx]\n"
        << "  targets\n  version\n  help\n\n"
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
        if (imp.rfind("std.", 0) == 0) continue;
        fs::path child = imp;
        if (child.extension().empty()) child += ".nx";
        if (child.is_relative()) child = canon.parent_path() / child;
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
            if (templ != "cli" && templ != "app" && templ != "game" && templ != "3d") throw std::runtime_error("unknown template: " + templ);
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
            if (templ != "cli" && templ != "app" && templ != "game" && templ != "3d") throw std::runtime_error("unknown template: " + templ);
            const fs::path root = fs::current_path();
            return initProject(root, root.filename().string(), templ, false);
        }

        if (cmd == "info") return commandInfo();
        if (cmd == "assets") return commandAssets();
        if (cmd == "clean") return commandClean();
        if (cmd == "doctor") return commandDoctor();

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
