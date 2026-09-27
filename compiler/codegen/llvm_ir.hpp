#pragma once
#include "ast/ast.hpp"
#include <string>
namespace nexus { class LLVMIRGenerator { public: std::string generate(const Program& p); };
}
