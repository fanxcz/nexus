#pragma once
#include "ast/ast.hpp"
#include <optional>
#include <string>
#include <vector>

namespace nexus {

struct BuiltinSignature {
    std::string name;
    std::vector<Type> params;
    Type ret;
    std::string runtimeName;
};

inline std::optional<BuiltinSignature> builtinSignature(const std::string& name) {
    using T = Type;
    if (name == "input") return BuiltinSignature{name, {T::string()}, T::string(), "nexus_input"};
    if (name == "input_i64") return BuiltinSignature{name, {T::string()}, T::i64(), "nexus_input_i64"};
    if (name == "input_f64") return BuiltinSignature{name, {T::string()}, T::f64(), "nexus_input_f64"};
    if (name == "str_i64") return BuiltinSignature{name, {T::i64()}, T::string(), "nexus_str_i64"};
    if (name == "str_f64") return BuiltinSignature{name, {T::f64()}, T::string(), "nexus_str_f64"};
    if (name == "str_bool") return BuiltinSignature{name, {T::boolean()}, T::string(), "nexus_str_bool"};
    if (name == "read_key") return BuiltinSignature{name, {}, T::i64(), "nexus_read_key"};
    if (name == "key_pressed") return BuiltinSignature{name, {}, T::boolean(), "nexus_key_pressed"};
    if (name == "clear") return BuiltinSignature{name, {}, T::void_(), "nexus_clear"};
    if (name == "sleep") return BuiltinSignature{name, {T::i64()}, T::void_(), "nexus_sleep_ms"};
    if (name == "random_i64") return BuiltinSignature{name, {T::i64(), T::i64()}, T::i64(), "nexus_random_i64"};
    if (name == "time_ms") return BuiltinSignature{name, {}, T::i64(), "nexus_time_ms"};
    if (name == "system") return BuiltinSignature{name, {T::string()}, T::i64(), "nexus_system"};
    if (name == "file_read") return BuiltinSignature{name, {T::string()}, T::string(), "nexus_file_read"};
    if (name == "file_write") return BuiltinSignature{name, {T::string(), T::string()}, T::i64(), "nexus_file_write"};
    if (name == "file_exists") return BuiltinSignature{name, {T::string()}, T::boolean(), "nexus_file_exists"};
    if (name == "env") return BuiltinSignature{name, {T::string()}, T::string(), "nexus_env"};
    if (name == "exit") return BuiltinSignature{name, {T::i64()}, T::void_(), "nexus_exit"};
    if (name == "screen_begin") return BuiltinSignature{name, {}, T::void_(), "nexus_screen_begin"};
    if (name == "screen_end") return BuiltinSignature{name, {}, T::void_(), "nexus_screen_end"};
    if (name == "screen_clear") return BuiltinSignature{name, {}, T::void_(), "nexus_screen_clear"};
    if (name == "screen_put") return BuiltinSignature{name, {T::i64(), T::i64(), T::string()}, T::void_(), "nexus_screen_put"};
    if (name == "screen_present") return BuiltinSignature{name, {}, T::void_(), "nexus_screen_present"};
    if (name == "screen_set_title") return BuiltinSignature{name, {T::string()}, T::void_(), "nexus_screen_set_title"};
    if (name == "screen_width") return BuiltinSignature{name, {}, T::i64(), "nexus_screen_width"};
    if (name == "screen_height") return BuiltinSignature{name, {}, T::i64(), "nexus_screen_height"};
    if (name == "beep") return BuiltinSignature{name, {}, T::void_(), "nexus_beep"};
    return std::nullopt;
}

} // namespace nexus
