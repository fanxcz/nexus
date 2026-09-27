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

    if (name == "gfx_init") return BuiltinSignature{name, {T::i64(), T::i64(), T::string()}, T::boolean(), "nexus_gfx_init"};
    if (name == "gfx_error") return BuiltinSignature{name, {}, T::string(), "nexus_gfx_error"};
    if (name == "gfx_shutdown") return BuiltinSignature{name, {}, T::void_(), "nexus_gfx_shutdown"};
    if (name == "gfx_should_close") return BuiltinSignature{name, {}, T::boolean(), "nexus_gfx_should_close"};
    if (name == "gfx_poll") return BuiltinSignature{name, {}, T::void_(), "nexus_gfx_poll"};
    if (name == "gfx_event_type") return BuiltinSignature{name, {}, T::i64(), "nexus_gfx_event_type"};
    if (name == "gfx_event_key") return BuiltinSignature{name, {}, T::i64(), "nexus_gfx_event_key"};
    if (name == "gfx_event_mouse_button") return BuiltinSignature{name, {}, T::i64(), "nexus_gfx_event_mouse_button"};
    if (name == "gfx_event_x") return BuiltinSignature{name, {}, T::i64(), "nexus_gfx_event_x"};
    if (name == "gfx_event_y") return BuiltinSignature{name, {}, T::i64(), "nexus_gfx_event_y"};
    if (name == "gfx_event_is") return BuiltinSignature{name, {T::string()}, T::boolean(), "nexus_gfx_event_is"};
    if (name == "gfx_width") return BuiltinSignature{name, {}, T::i64(), "nexus_gfx_width"};
    if (name == "gfx_height") return BuiltinSignature{name, {}, T::i64(), "nexus_gfx_height"};
    if (name == "gfx_vsync") return BuiltinSignature{name, {T::boolean()}, T::boolean(), "nexus_gfx_vsync"};
    if (name == "gfx_mouse_x") return BuiltinSignature{name, {}, T::f64(), "nexus_gfx_mouse_x"};
    if (name == "gfx_mouse_y") return BuiltinSignature{name, {}, T::f64(), "nexus_gfx_mouse_y"};
    if (name == "gfx_mouse_down") return BuiltinSignature{name, {T::i64()}, T::boolean(), "nexus_gfx_mouse_down"};
    if (name == "gfx_key_down") return BuiltinSignature{name, {T::string()}, T::boolean(), "nexus_gfx_key_down"};
    if (name == "gfx_dt") return BuiltinSignature{name, {}, T::f64(), "nexus_gfx_dt"};
    if (name == "gfx_begin") return BuiltinSignature{name, {}, T::void_(), "nexus_gfx_begin"};
    if (name == "gfx_end") return BuiltinSignature{name, {}, T::void_(), "nexus_gfx_end"};
    if (name == "gfx_clear") return BuiltinSignature{name, {T::f64(), T::f64(), T::f64(), T::f64()}, T::void_(), "nexus_gfx_clear"};
    if (name == "gfx_rect") return BuiltinSignature{name, {T::f64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64()}, T::void_(), "nexus_gfx_rect"};
    if (name == "gfx_circle") return BuiltinSignature{name, {T::f64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64()}, T::void_(), "nexus_gfx_circle"};
    if (name == "gfx_line") return BuiltinSignature{name, {T::f64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64()}, T::void_(), "nexus_gfx_line"};
    if (name == "gfx_text") return BuiltinSignature{name, {T::f64(), T::f64(), T::f64(), T::string(), T::f64(), T::f64(), T::f64(), T::f64()}, T::void_(), "nexus_gfx_text"};
    if (name == "gfx_set_title") return BuiltinSignature{name, {T::string()}, T::void_(), "nexus_gfx_set_title"};
    if (name == "gfx_texture_load") return BuiltinSignature{name, {T::string()}, T::i64(), "nexus_gfx_texture_load"};
    if (name == "gfx_texture_draw") return BuiltinSignature{name, {T::i64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64()}, T::void_(), "nexus_gfx_texture_draw"};
    if (name == "gfx_texture_unload") return BuiltinSignature{name, {T::i64()}, T::void_(), "nexus_gfx_texture_unload"};
    if (name == "gfx_texture_width") return BuiltinSignature{name, {T::i64()}, T::i64(), "nexus_gfx_texture_width"};
    if (name == "gfx_texture_height") return BuiltinSignature{name, {T::i64()}, T::i64(), "nexus_gfx_texture_height"};
    if (name == "gfx3d_begin") return BuiltinSignature{name, {T::f64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64()}, T::void_(), "nexus_gfx3d_begin"};
    if (name == "gfx3d_cube") return BuiltinSignature{name, {T::f64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64()}, T::void_(), "nexus_gfx3d_cube"};
    if (name == "gfx3d_grid") return BuiltinSignature{name, {T::i64(), T::f64(), T::f64(), T::f64(), T::f64(), T::f64()}, T::void_(), "nexus_gfx3d_grid"};
    if (name == "gfx3d_end") return BuiltinSignature{name, {}, T::void_(), "nexus_gfx3d_end"};
    if (name == "gfx_sound_load") return BuiltinSignature{name, {T::string()}, T::i64(), "nexus_gfx_sound_load"};
    if (name == "gfx_sound_play") return BuiltinSignature{name, {T::i64(), T::boolean()}, T::void_(), "nexus_gfx_sound_play"};
    if (name == "gfx_sound_stop") return BuiltinSignature{name, {}, T::void_(), "nexus_gfx_sound_stop"};
    if (name == "gfx_sound_unload") return BuiltinSignature{name, {T::i64()}, T::void_(), "nexus_gfx_sound_unload"};
    return std::nullopt;
}

} // namespace nexus
