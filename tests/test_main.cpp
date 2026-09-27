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
        {
            auto p=parse("fn main(){ let mut running = true; while running { running = false } let name = input(\"Name: \"); let n = input_i64(\"N: \"); let x = str_i64(n); let ok = file_write(\"/tmp/nexus-tests.txt\", x); print(ok) }");
            nexus::SemanticAnalyzer().analyze(p);
        }
        {
            auto p=parse("fn main(){ let s=gfx_sound_load(\"examples/assets/beep.wav\"); let a=gfx_audio_available(); let e=gfx_audio_error(); gfx_sound_volume(0.5); gfx_sound_playing(); gfx_sound_play(s,false); gfx_sound_stop(); gfx_sound_unload(s); }");
            nexus::SemanticAnalyzer().analyze(p);
        }
        {
            auto p=parse("fn main(){ let m=gfx3d_model_load(\"examples/assets/cube.obj\"); gfx3d_model_draw(m,0.0,0.0,0.0,1.0,1.0,1.0,0.0,0.0,0.0); gfx3d_model_unload(m); let p=gfx_particle_create(0.0,0.0,1.0,1.0,1.0,4.0,1.0,0.0,0.0,1.0); gfx_particle_alive(p); gfx_particles_update(0.016); gfx_particles_draw(); gfx_particle_destroy(p); }");
            nexus::SemanticAnalyzer().analyze(p);
        }
        {
            auto p=parse("fn main(){ let s=scene_create(); let e=ecs_create(); ecs_set_position(e,10.0,20.0); ecs_set_size(e,32.0,32.0); scene_add(s,e); scene_save(s,\"/tmp/nexus-scene.nxs\"); let s2=scene_load(\"/tmp/nexus-scene.nxs\"); scene_clear(s2); scene_destroy(s2); scene_destroy(s); let hit=physics_aabb(0.0,0.0,10.0,10.0,5.0,5.0,4.0,4.0); let circ=physics_circle(0.0,0.0,5.0,6.0,0.0,2.0); let a=gfx_anim_create(1,16,16,4,8,12.0,true); gfx_anim_update(a,0.1); gfx_anim_frame(a); gfx_anim_destroy(a); ui_checkbox(10.0,10.0,\"Ready\",true); ui_slider(10.0,50.0,200.0,20.0,0.5); }" );
            nexus::SemanticAnalyzer().analyze(p);
        }
        std::cout<<"Nexus tests: PASS\n";
        return 0;
    } catch(const std::exception&e){
        std::cerr<<e.what()<<"\n";
        return 1;
    }
}
