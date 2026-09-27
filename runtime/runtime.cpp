#include <cstdio>
#include <cstdint>
#include <cstdbool>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cmath>
#include <string>
#include <vector>
#include <unordered_map>
#include <array>
#include <algorithm>
#include <cctype>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>
#endif

// NEXUS runtime is deliberately built without SDL/OpenGL development headers.
// The graphics backend loads SDL2 at runtime and obtains OpenGL entry points
// from SDL. This keeps the generated NEXUS application portable across hosts
// that provide SDL2/OpenGL without requiring a compiler-time SDK.

namespace {

using byte = unsigned char;

typedef struct SDL_Window SDL_Window;
typedef void* SDL_GLContext;
typedef uint32_t SDL_AudioDeviceID;
typedef uint16_t SDL_AudioFormat;
typedef uint8_t SDL_AudioStatus;
typedef uint32_t SDL_AudioCallbackUserdata;

struct SDL_AudioSpec {
    int freq;
    SDL_AudioFormat format;
    uint8_t channels;
    uint8_t silence;
    uint16_t samples;
    uint16_t padding;
    uint32_t size;
    void (*callback)(void*, uint8_t*, int);
    void* userdata;
};

struct SDL_PixelFormat {
    uint32_t format;
    void* palette;
    uint8_t BitsPerPixel;
    uint8_t BytesPerPixel;
    uint8_t padding[2];
    uint32_t Rmask, Gmask, Bmask, Amask;
    uint8_t Rloss, Gloss, Bloss, Aloss;
    uint8_t Rshift, Gshift, Bshift, Ashift;
    int32_t refcount;
    void* next;
};

struct SDL_Surface {
    uint32_t flags;
    SDL_PixelFormat* format;
    int w, h;
    int pitch;
    void* pixels;
    void* userdata;
    int locked;
    void* lock_data;
    std::array<int, 2> clip_rect;
    void* map;
    int refcount;
    void* reserved;
};

struct SDL_Keysym {
    int32_t scancode;
    int32_t sym;
    uint16_t mod;
    uint32_t unused;
};
struct SDL_KeyboardEvent {
    uint32_t type;
    uint32_t timestamp;
    uint32_t windowID;
    uint8_t state;
    uint8_t repeat;
    uint8_t padding2;
    uint8_t padding3;
    SDL_Keysym keysym;
};
struct SDL_MouseMotionEvent {
    uint32_t type, timestamp, windowID;
    uint32_t which;
    uint32_t state;
    int x, y, xrel, yrel;
};
struct SDL_MouseButtonEvent {
    uint32_t type, timestamp, windowID;
    uint32_t which;
    uint8_t button, state, clicks, padding1;
    int x, y;
};
struct SDL_MouseWheelEvent {
    uint32_t type, timestamp, windowID;
    uint32_t which;
    int x, y;
    uint32_t direction;
    double preciseX, preciseY;
    int32_t mouseX, mouseY;
};
struct SDL_WindowEvent {
    uint32_t type, timestamp, windowID;
    uint8_t event;
    uint8_t padding1, padding2, padding3;
    int data1, data2;
};
union SDL_Event {
    uint32_t type;
    uint8_t padding[128];
    SDL_KeyboardEvent key;
    SDL_MouseMotionEvent motion;
    SDL_MouseButtonEvent button;
    SDL_MouseWheelEvent wheel;
    SDL_WindowEvent window;
};

// SDL constants used by the runtime.
constexpr uint32_t SDL_INIT_TIMER = 0x00000001u;
constexpr uint32_t SDL_INIT_AUDIO = 0x00000010u;
constexpr uint32_t SDL_INIT_VIDEO = 0x00000020u;
constexpr uint32_t SDL_WINDOW_OPENGL = 0x00000002u;
constexpr uint32_t SDL_WINDOW_SHOWN = 0x00000004u;
constexpr uint32_t SDL_WINDOW_RESIZABLE = 0x00000020u;
constexpr int SDL_WINDOWPOS_CENTERED = 0x2FFF0000;
constexpr int SDL_GL_CONTEXT_MAJOR_VERSION = 17;
constexpr int SDL_GL_CONTEXT_MINOR_VERSION = 18;
constexpr int SDL_GL_CONTEXT_PROFILE_MASK = 21;
constexpr int SDL_GL_CONTEXT_PROFILE_COMPATIBILITY = 0x0002;
constexpr uint32_t SDL_QUIT = 0x100u;
constexpr uint32_t SDL_WINDOWEVENT = 0x200u;
constexpr uint32_t SDL_KEYDOWN = 0x300u;
constexpr uint32_t SDL_KEYUP = 0x301u;
constexpr uint32_t SDL_MOUSEMOTION = 0x400u;
constexpr uint32_t SDL_MOUSEBUTTONDOWN = 0x401u;
constexpr uint32_t SDL_MOUSEBUTTONUP = 0x402u;
constexpr uint8_t SDL_PRESSED = 1;
constexpr uint8_t SDL_RELEASED = 0;
constexpr uint32_t SDL_BUTTON_LEFT = 1u;
constexpr uint32_t SDL_BUTTON_MIDDLE = 2u;
constexpr uint32_t SDL_BUTTON_RIGHT = 3u;
constexpr uint32_t SDL_PIXELFORMAT_ABGR8888 = 376840196u;
constexpr uint16_t AUDIO_S16LSB = 0x8010u;

// OpenGL constants/types.
using GLenum = unsigned int;
using GLboolean = unsigned char;
using GLbitfield = unsigned int;
using GLvoid = void;
using GLbyte = signed char;
using GLshort = short;
using GLint = int;
using GLsizei = int;
using GLuint = unsigned int;
using GLfloat = float;
using GLclampf = float;
using GLdouble = double;
using GLubyte = unsigned char;
using GLushort = unsigned short;

constexpr GLenum GL_COLOR_BUFFER_BIT = 0x00004000;
constexpr GLenum GL_DEPTH_BUFFER_BIT = 0x00000100;
constexpr GLenum GL_DEPTH_TEST = 0x0B71;
constexpr GLenum GL_BLEND = 0x0BE2;
constexpr GLenum GL_SRC_ALPHA = 0x0302;
constexpr GLenum GL_ONE_MINUS_SRC_ALPHA = 0x0303;
constexpr GLenum GL_TEXTURE_2D = 0x0DE1;
constexpr GLenum GL_RGBA = 0x1908;
constexpr GLenum GL_UNSIGNED_BYTE = 0x1401;
constexpr GLenum GL_TEXTURE_MIN_FILTER = 0x2801;
constexpr GLenum GL_TEXTURE_MAG_FILTER = 0x2800;
constexpr GLenum GL_LINEAR = 0x2601;
constexpr GLenum GL_MODELVIEW = 0x1700;
constexpr GLenum GL_PROJECTION = 0x1701;
constexpr GLenum GL_QUADS = 0x0007;
constexpr GLenum GL_LINES = 0x0001;
constexpr GLenum GL_TRIANGLES = 0x0004;
constexpr GLenum GL_FRONT_AND_BACK = 0x0408;
constexpr GLenum GL_FILL = 0x1B02;
constexpr GLenum GL_CULL_FACE = 0x0B44;

#define GLPROC(name, ret, args) using PFN_##name = ret (*)args; PFN_##name name = nullptr;
GLPROC(glViewport, void, (GLint,GLint,GLsizei,GLsizei))
GLPROC(glMatrixMode, void, (GLenum))
GLPROC(glLoadIdentity, void, ())
GLPROC(glOrtho, void, (GLdouble,GLdouble,GLdouble,GLdouble,GLdouble,GLdouble))
GLPROC(glFrustum, void, (GLdouble,GLdouble,GLdouble,GLdouble,GLdouble,GLdouble))
GLPROC(glBegin, void, (GLenum))
GLPROC(glEnd, void, ())
GLPROC(glColor4f, void, (GLfloat,GLfloat,GLfloat,GLfloat))
GLPROC(glVertex2f, void, (GLfloat,GLfloat))
GLPROC(glVertex3f, void, (GLfloat,GLfloat,GLfloat))
GLPROC(glTexCoord2f, void, (GLfloat,GLfloat))
GLPROC(glBindTexture, void, (GLenum,GLuint))
GLPROC(glGenTextures, void, (GLsizei,GLuint*))
GLPROC(glDeleteTextures, void, (GLsizei,const GLuint*))
GLPROC(glTexParameteri, void, (GLenum,GLenum,GLint))
GLPROC(glTexImage2D, void, (GLenum,GLint,GLint,GLsizei,GLsizei,GLint,GLenum,GLenum,const GLvoid*))
GLPROC(glEnable, void, (GLenum))
GLPROC(glDisable, void, (GLenum))
GLPROC(glClearColor, void, (GLfloat,GLfloat,GLfloat,GLfloat))
GLPROC(glClear, void, (GLbitfield))
GLPROC(glTranslatef, void, (GLfloat,GLfloat,GLfloat))
GLPROC(glRotatef, void, (GLfloat,GLfloat,GLfloat,GLfloat))
GLPROC(glScalef, void, (GLfloat,GLfloat,GLfloat))
GLPROC(glLineWidth, void, (GLfloat))
GLPROC(glPolygonMode, void, (GLenum,GLenum))
#undef GLPROC

#define SDLPROC(name, ret, args) using PFN_SDL_##name = ret (*)args; PFN_SDL_##name name = nullptr;
SDLPROC(SDL_Init, int, (uint32_t))
SDLPROC(SDL_Quit, void, ())
SDLPROC(SDL_GetError, const char*, ())
SDLPROC(SDL_CreateWindow, SDL_Window*, (const char*, int, int, int, int, uint32_t))
SDLPROC(SDL_DestroyWindow, void, (SDL_Window*))
SDLPROC(SDL_GL_SetAttribute, int, (int,int))
SDLPROC(SDL_GL_CreateContext, SDL_GLContext, (SDL_Window*))
SDLPROC(SDL_GL_DeleteContext, void, (SDL_GLContext))
SDLPROC(SDL_GL_SwapWindow, void, (SDL_Window*))
SDLPROC(SDL_GL_GetProcAddress, void*, (const char*))
SDLPROC(SDL_SetWindowTitle, void, (SDL_Window*, const char*))
SDLPROC(SDL_GL_SetSwapInterval, int, (int))
SDLPROC(SDL_PollEvent, int, (SDL_Event*))
SDLPROC(SDL_GetKeyboardState, const uint8_t*, (int*))
SDLPROC(SDL_GetMouseState, uint32_t, (int*,int*))
SDLPROC(SDL_LoadBMP, SDL_Surface*, (const char*))
SDLPROC(SDL_ConvertSurfaceFormat, SDL_Surface*, (SDL_Surface*,uint32_t,uint32_t))
SDLPROC(SDL_FreeSurface, void, (SDL_Surface*))
SDLPROC(SDL_GetPerformanceCounter, uint64_t, ())
SDLPROC(SDL_GetPerformanceFrequency, uint64_t, ())
SDLPROC(SDL_LoadWAV, SDL_AudioSpec*, (const char*,SDL_AudioSpec*,uint8_t**,uint32_t*))
SDLPROC(SDL_FreeWAV, void, (uint8_t*))
SDLPROC(SDL_OpenAudioDevice, SDL_AudioDeviceID, (const char*,int,const SDL_AudioSpec*,SDL_AudioSpec*,int))
SDLPROC(SDL_CloseAudioDevice, void, (SDL_AudioDeviceID))
SDLPROC(SDL_PauseAudioDevice, void, (SDL_AudioDeviceID,int))
SDLPROC(SDL_QueueAudio, int, (SDL_AudioDeviceID,const void*,uint32_t))
SDLPROC(SDL_GetNumAudioDevices, int, (int))
#undef SDLPROC

using FnLoadLibrary = void* (*)(const char*);
using FnGetProc = void* (*)(void*, const char*);
static void* gSDL = nullptr;
static SDL_Window* gWindow = nullptr;
static SDL_GLContext gGL = nullptr;
static bool gGraphics = false;
static bool gAudio = false;
static std::string gGraphicsError;
static bool gClose = false;
static int gWidth = 1280;
static int gHeight = 720;
static int gLastEvent = 0;
static int gLastKey = -1;
static int gLastMouseButton = 0;
static int gLastX = 0;
static int gLastY = 0;
static double gLastFrame = 0.0;
static std::vector<uint8_t> gKeys(512, 0);
static std::array<uint8_t, 8> gMouseButtons{};
static SDL_AudioDeviceID gAudioDevice = 0;

struct Texture { GLuint id{}; int w{}; int h{}; };
struct Sound { std::vector<uint8_t> data; int freq{}; uint16_t format{}; uint8_t channels{}; uint16_t samples{}; };
static std::unordered_map<int64_t, Texture> gTextures;
static std::unordered_map<int64_t, Sound> gSounds;
static int64_t gNextTexture = 1;
static int64_t gNextSound = 1;

static void* load_library(const char* name) {
#if defined(_WIN32)
    return reinterpret_cast<void*>(LoadLibraryA(name));
#else
    return dlopen(name, RTLD_NOW | RTLD_LOCAL);
#endif
}
static void* load_symbol(void* lib, const char* name) {
#if defined(_WIN32)
    return reinterpret_cast<void*>(GetProcAddress(reinterpret_cast<HMODULE>(lib), name));
#else
    return dlsym(lib, name);
#endif
}
static std::string sdl_error() { return gSDL && SDL_GetError ? SDL_GetError() : "SDL2 unavailable"; }
static bool load_sdl() {
    if(gSDL) return true;
#if defined(_WIN32)
    const char* names[] = {"SDL2.dll", "SDL2-2.0.dll"};
#elif defined(__APPLE__)
    const char* names[] = {"libSDL2-2.0.0.dylib", "libSDL2.dylib"};
#else
    const char* names[] = {"libSDL2-2.0.so.0", "libSDL2.so", "libSDL2-2.0.so"};
#endif
    for(const char* n: names){ if((gSDL=load_library(n))) break; }
    if(!gSDL){ gGraphicsError = "SDL2 runtime library not found"; return false; }
    bool ok=true;
#define LOAD_REQUIRED_SDL(n) do { n = reinterpret_cast<PFN_SDL_##n>(load_symbol(gSDL, #n)); if(!n) ok=false; } while(0)
#define LOAD_OPTIONAL_SDL(n) do { n = reinterpret_cast<PFN_SDL_##n>(load_symbol(gSDL, #n)); } while(0)
    // Only video/window/event/timing entry points are required to start graphics.
    // Image and audio entry points are optional and are loaded lazily when those
    // features are actually used. This avoids rejecting valid SDL2 runtimes.
    LOAD_REQUIRED_SDL(SDL_Init); LOAD_REQUIRED_SDL(SDL_Quit); LOAD_REQUIRED_SDL(SDL_GetError);
    LOAD_REQUIRED_SDL(SDL_CreateWindow); LOAD_REQUIRED_SDL(SDL_DestroyWindow); LOAD_REQUIRED_SDL(SDL_SetWindowTitle); LOAD_REQUIRED_SDL(SDL_GL_SetSwapInterval);
    LOAD_REQUIRED_SDL(SDL_GL_SetAttribute); LOAD_REQUIRED_SDL(SDL_GL_CreateContext); LOAD_REQUIRED_SDL(SDL_GL_DeleteContext); LOAD_REQUIRED_SDL(SDL_GL_SwapWindow); LOAD_REQUIRED_SDL(SDL_GL_GetProcAddress);
    LOAD_REQUIRED_SDL(SDL_PollEvent); LOAD_REQUIRED_SDL(SDL_GetKeyboardState); LOAD_REQUIRED_SDL(SDL_GetMouseState);
    LOAD_REQUIRED_SDL(SDL_GetPerformanceCounter); LOAD_REQUIRED_SDL(SDL_GetPerformanceFrequency);
    LOAD_OPTIONAL_SDL(SDL_LoadBMP); LOAD_OPTIONAL_SDL(SDL_ConvertSurfaceFormat); LOAD_OPTIONAL_SDL(SDL_FreeSurface);
    LOAD_OPTIONAL_SDL(SDL_LoadWAV); LOAD_OPTIONAL_SDL(SDL_FreeWAV); LOAD_OPTIONAL_SDL(SDL_OpenAudioDevice); LOAD_OPTIONAL_SDL(SDL_CloseAudioDevice); LOAD_OPTIONAL_SDL(SDL_PauseAudioDevice); LOAD_OPTIONAL_SDL(SDL_QueueAudio); LOAD_OPTIONAL_SDL(SDL_GetNumAudioDevices);
#undef LOAD_OPTIONAL_SDL
#undef LOAD_REQUIRED_SDL
    if(!ok){ gGraphicsError = "SDL2 runtime is missing required video/window symbols"; gSDL=nullptr; return false; }
    return true;
}

static bool load_gl() {
    if(!SDL_GL_GetProcAddress) return false;
#define LOAD_GL(n) do { n = reinterpret_cast<PFN_##n>(SDL_GL_GetProcAddress(#n)); if(!n) return false; } while(0)
    LOAD_GL(glViewport); LOAD_GL(glMatrixMode); LOAD_GL(glLoadIdentity); LOAD_GL(glOrtho); LOAD_GL(glFrustum);
    LOAD_GL(glBegin); LOAD_GL(glEnd); LOAD_GL(glColor4f); LOAD_GL(glVertex2f); LOAD_GL(glVertex3f); LOAD_GL(glTexCoord2f);
    LOAD_GL(glBindTexture); LOAD_GL(glGenTextures); LOAD_GL(glDeleteTextures); LOAD_GL(glTexParameteri); LOAD_GL(glTexImage2D);
    LOAD_GL(glEnable); LOAD_GL(glDisable); LOAD_GL(glClearColor); LOAD_GL(glClear); LOAD_GL(glTranslatef); LOAD_GL(glRotatef); LOAD_GL(glScalef); LOAD_GL(glLineWidth); LOAD_GL(glPolygonMode);
#undef LOAD_GL
    return true;
}

static double now_seconds() {
    if(SDL_GetPerformanceCounter && SDL_GetPerformanceFrequency){
        static uint64_t start = SDL_GetPerformanceCounter();
        return double(SDL_GetPerformanceCounter() - start) / double(SDL_GetPerformanceFrequency());
    }
    return double(std::clock()) / double(CLOCKS_PER_SEC);
}

// glBlendFunc is loaded separately to keep the backend tiny but complete.
using PFN_glBlendFunc = void (*)(GLenum,GLenum);
static PFN_glBlendFunc glBlendFuncPtr = nullptr;

static void setup_2d() {
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); if(glBlendFuncPtr) glBlendFuncPtr(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0, gWidth, gHeight, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
}

static void draw_quad(float x,float y,float w,float h,float r,float g,float b,float a){
    glColor4f(r,g,b,a); glBegin(GL_QUADS); glVertex2f(x,y); glVertex2f(x+w,y); glVertex2f(x+w,y+h); glVertex2f(x,y+h); glEnd();
}

static void draw_circle(float cx,float cy,float radius,float r,float g,float b,float a){
    glColor4f(r,g,b,a); glBegin(GL_TRIANGLES); constexpr int N=48; for(int i=0;i<N;i++){double a0=(2.0*M_PI*i)/N,a1=(2.0*M_PI*(i+1))/N;glVertex2f(cx,cy);glVertex2f(float(cx+std::cos(a0)*radius),float(cy+std::sin(a0)*radius));glVertex2f(float(cx+std::cos(a1)*radius),float(cy+std::sin(a1)*radius));} glEnd();
}

static const uint8_t FONT5X7[][7] = {
    {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}, // A
    {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}, // B
    {0x0F,0x10,0x10,0x10,0x10,0x10,0x0F}, // C
    {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E}, // D
    {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}, // E
    {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}, // F
    {0x0F,0x10,0x10,0x17,0x11,0x11,0x0F}, // G
    {0x11,0x11,0x11,0x1F,0x11,0x11,0x11}, // H
    {0x1F,0x04,0x04,0x04,0x04,0x04,0x1F}, // I
    {0x01,0x01,0x01,0x01,0x11,0x11,0x0E}, // J
    {0x11,0x12,0x14,0x18,0x14,0x12,0x11}, // K
    {0x10,0x10,0x10,0x10,0x10,0x10,0x1F}, // L
    {0x11,0x1B,0x15,0x15,0x11,0x11,0x11}, // M
    {0x11,0x19,0x15,0x13,0x11,0x11,0x11}, // N
    {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}, // O
    {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}, // P
    {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}, // Q
    {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}, // R
    {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}, // S
    {0x1F,0x04,0x04,0x04,0x04,0x04,0x04}, // T
    {0x11,0x11,0x11,0x11,0x11,0x11,0x0E}, // U
    {0x11,0x11,0x11,0x11,0x11,0x0A,0x04}, // V
    {0x11,0x11,0x11,0x15,0x15,0x1B,0x11}, // W
    {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11}, // X
    {0x11,0x11,0x0A,0x04,0x04,0x04,0x04}, // Y
    {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}, // Z
    {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}, // 0
    {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}, // 1
    {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}, // 2
    {0x1E,0x01,0x01,0x0E,0x01,0x01,0x1E}, // 3
    {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}, // 4
    {0x1F,0x10,0x10,0x1E,0x01,0x01,0x1E}, // 5
    {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E}, // 6
    {0x1F,0x01,0x02,0x04,0x08,0x08,0x08}, // 7
    {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}, // 8
    {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C}, // 9
};
static const uint8_t* glyph(char c){
    c=(c>='a'&&c<='z')?char(c-'a'+'A'):c;
    if(c>='A'&&c<='Z') return FONT5X7[c-'A'];
    if(c>='0'&&c<='9') return FONT5X7[26+(c-'0')];
    return nullptr;
}
static void draw_text(float x,float y,float scale,const char* text,float r,float g,float b,float a){
    if(!text) return; float ox=x; glColor4f(r,g,b,a);
    for(const char* p=text;*p;p++){
        if(*p=='\n'){y+=8.0f*scale;x=ox;continue;} if(*p==' '){x+=6.0f*scale;continue;}
        const auto* glyphBits=glyph(*p); if(!glyphBits){x+=6.0f*scale;continue;}
        for(int row=0;row<7;row++) for(int col=0;col<5;col++) if(glyphBits[row]&(1u<<(4-col))) draw_quad(x+col*scale,y+row*scale,scale,scale,r,g,b,a);
        x+=6.0f*scale;
    }
}

static void draw_textured_quad(const Texture& t,float x,float y,float w,float h,float tintR,float tintG,float tintB,float tintA){
    glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D,t.id); glColor4f(tintR,tintG,tintB,tintA); glBegin(GL_QUADS);
    glTexCoord2f(0,0);glVertex2f(x,y);glTexCoord2f(1,0);glVertex2f(x+w,y);glTexCoord2f(1,1);glVertex2f(x+w,y+h);glTexCoord2f(0,1);glVertex2f(x,y+h);glEnd();glBindTexture(GL_TEXTURE_2D,0);glDisable(GL_TEXTURE_2D);
}

static int64_t scancode_for(const char* name){
    if(!name)return -1; std::string s(name); std::transform(s.begin(),s.end(),s.begin(),[](unsigned char c){return char(std::toupper(c));});
    if(s.size()==1 && s[0]>='A'&&s[0]<='Z'){static const int codes[26]={4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29};return codes[s[0]-'A'];}
    if(s=="SPACE")return 44; if(s=="ENTER")return 40; if(s=="ESC"||s=="ESCAPE")return 41; if(s=="LEFT")return 80; if(s=="RIGHT")return 79; if(s=="UP")return 82; if(s=="DOWN")return 81; if(s=="TAB")return 43; if(s=="SHIFT")return 225; if(s=="CTRL"||s=="CONTROL")return 224; return -1;
}

static void clear_event(){ gLastEvent=0; gLastKey=-1; gLastMouseButton=0; }
static void process_events(){
    if(!SDL_PollEvent) return; SDL_Event e{};
    clear_event();
    while(SDL_PollEvent(&e)){
        gLastEvent=int(e.type);
        if(e.type==SDL_QUIT){gClose=true;}
        else if(e.type==SDL_KEYDOWN){gLastKey=int(e.key.keysym.scancode);if(gLastKey>=0&&gLastKey<int(gKeys.size()))gKeys[size_t(gLastKey)]=1;}
        else if(e.type==SDL_KEYUP){gLastKey=int(e.key.keysym.scancode);if(gLastKey>=0&&gLastKey<int(gKeys.size()))gKeys[size_t(gLastKey)]=0;}
        else if(e.type==SDL_MOUSEMOTION){gLastX=e.motion.x;gLastY=e.motion.y;}
        else if(e.type==SDL_MOUSEBUTTONDOWN){gLastMouseButton=int(e.button.button);gLastX=e.button.x;gLastY=e.button.y;if(e.button.button<gMouseButtons.size())gMouseButtons[e.button.button]=1;}
        else if(e.type==SDL_MOUSEBUTTONUP){gLastMouseButton=int(e.button.button);gLastX=e.button.x;gLastY=e.button.y;if(e.button.button<gMouseButtons.size())gMouseButtons[e.button.button]=0;}
        else if(e.type==SDL_WINDOWEVENT){gLastX=e.window.data1;gLastY=e.window.data2;if(e.window.data1>0)gWidth=e.window.data1;if(e.window.data2>0)gHeight=e.window.data2;}
    }
}

static bool init_graphics(int w,int h,const char* title){
    if(gGraphics)return true;
    gGraphicsError.clear();
    if(!load_sdl())return false;
    // Audio is intentionally NOT initialized here. A missing/default audio device
    // must never prevent an SDL/OpenGL window from starting. Audio is lazy-init
    // when the first sound is loaded.
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)!=0){ gGraphicsError = std::string("SDL_Init failed: ")+sdl_error(); return false; }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    gWindow=SDL_CreateWindow(title?title:"NEXUS",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,w,h,SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE);
    if(!gWindow){ gGraphicsError = std::string("SDL_CreateWindow failed: ")+sdl_error(); SDL_Quit(); return false; }
    gGL=SDL_GL_CreateContext(gWindow);
    if(!gGL){ gGraphicsError = std::string("SDL_GL_CreateContext failed: ")+sdl_error(); SDL_DestroyWindow(gWindow); gWindow=nullptr; SDL_Quit(); return false; }
    if(!load_gl()){ gGraphicsError = "OpenGL 2.1 compatibility entry points unavailable"; SDL_GL_DeleteContext(gGL); gGL=nullptr; SDL_DestroyWindow(gWindow); gWindow=nullptr; SDL_Quit(); return false; }
    glBlendFuncPtr = reinterpret_cast<PFN_glBlendFunc>(SDL_GL_GetProcAddress("glBlendFunc"));
    gWidth=w;gHeight=h;gGraphics=true;gClose=false;gLastFrame=now_seconds();
    glViewport(0,0,w,h); glEnable(GL_BLEND); if(glBlendFuncPtr)glBlendFuncPtr(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA); glEnable(GL_DEPTH_TEST);
    return true;
}

static void shutdown_graphics(){
    if(!gGraphics)return; if(gGL&&SDL_GL_DeleteContext)SDL_GL_DeleteContext(gGL);gGL=nullptr;if(gWindow&&SDL_DestroyWindow)SDL_DestroyWindow(gWindow);gWindow=nullptr;gTextures.clear();gSounds.clear();if(gAudioDevice&&SDL_CloseAudioDevice){SDL_CloseAudioDevice(gAudioDevice);gAudioDevice=0;}if(SDL_Quit)SDL_Quit();gGraphics=false;
}

} // namespace

extern "C" {

// Existing runtime API -------------------------------------------------------
static char* nx_strdup(const char* src){if(!src)src="";size_t n=std::strlen(src);char* out=(char*)std::malloc(n+1);if(!out)return nullptr;std::memcpy(out,src,n+1);return out;}
void nexus_print_i64(int64_t x){std::printf("%lld\n",(long long)x);} void nexus_print_f64(double x){std::printf("%.15g\n",x);} void nexus_print_bool(bool x){std::puts(x?"true":"false");} void nexus_print_str(const char*x){std::puts(x?x:"null");}
void* nexus_alloc(size_t n){return std::malloc(n);} void nexus_free(void*p){std::free(p);} void nexus_bounds_check(int64_t i,int64_t n){if(i<0||i>=n){std::fprintf(stderr,"Nexus runtime error: array index %lld out of bounds for size %lld\n",(long long)i,(long long)n);std::exit(1);}}
char* nexus_str_concat(const char*a,const char*b){if(!a)a="";if(!b)b="";size_t la=std::strlen(a),lb=std::strlen(b);char*out=(char*)std::malloc(la+lb+1);if(!out)return nullptr;std::memcpy(out,a,la);std::memcpy(out+la,b,lb);out[la+lb]=0;return out;} bool nexus_str_equal(const char*a,const char*b){return a==b||(!a&&!b)||(a&&b&&std::strcmp(a,b)==0);}
static char* nx_readline(){size_t cap=256,len=0;char*buf=(char*)std::malloc(cap);if(!buf)return nullptr;int ch;while((ch=std::getchar())!=EOF){if(ch=='\r')continue;if(ch=='\n')break;if(len+1>=cap){cap*=2;char*nb=(char*)std::realloc(buf,cap);if(!nb){std::free(buf);return nullptr;}buf=nb;}buf[len++]=(char)ch;}buf[len]=0;return buf;}
char* nexus_input(const char*p){if(p)std::fputs(p,stdout);std::fflush(stdout);return nx_readline();}
int64_t nexus_input_i64(const char*p){for(;;){char*s=nexus_input(p);if(!s)return 0;char*e=nullptr;long long v=std::strtoll(s,&e,10);while(e&&*e==' ')++e;if(e&&*e==0){std::free(s);return v;}std::fprintf(stderr,"Invalid integer.\n");std::free(s);}}
double nexus_input_f64(const char*p){for(;;){char*s=nexus_input(p);if(!s)return 0;char*e=nullptr;double v=std::strtod(s,&e);while(e&&*e==' ')++e;if(e&&*e==0){std::free(s);return v;}std::fprintf(stderr,"Invalid number.\n");std::free(s);}}
char* nexus_str_i64(int64_t v){char b[64];std::snprintf(b,sizeof(b),"%lld",(long long)v);return nx_strdup(b);} char* nexus_str_f64(double v){char b[96];std::snprintf(b,sizeof(b),"%.15g",v);return nx_strdup(b);} char* nexus_str_bool(bool v){return nx_strdup(v?"true":"false");}
void nexus_clear(){std::fputs("\033[2J\033[H",stdout);std::fflush(stdout);} void nexus_sleep_ms(int64_t ms){if(ms<=0)return;
#if defined(_WIN32)
    Sleep((DWORD)ms);
#else
    timespec ts{};ts.tv_sec=ms/1000;ts.tv_nsec=(long)((ms%1000)*1000000);nanosleep(&ts,nullptr);
#endif
}
int64_t nexus_random_i64(int64_t min,int64_t max){static bool s=false;if(!s){std::srand((unsigned)std::time(nullptr));s=true;}if(min>max)std::swap(min,max);uint64_t span=uint64_t(max-min)+1u;if(!span)return min;uint64_t r=(uint64_t(std::rand())<<32)^uint64_t(std::rand());return min+int64_t(r%span);}int64_t nexus_time_ms(){return int64_t(now_seconds()*1000.0);}
int64_t nexus_system(const char*c){return c?std::system(c):-1;}char*nexus_file_read(const char*p){if(!p)return nullptr;std::FILE*f=std::fopen(p,"rb");if(!f)return nullptr;std::fseek(f,0,SEEK_END);long n=std::ftell(f);std::rewind(f);if(n<0){std::fclose(f);return nullptr;}char*o=(char*)std::malloc(size_t(n)+1);if(!o){std::fclose(f);return nullptr;}size_t got=std::fread(o,1,size_t(n),f);std::fclose(f);o[got]=0;return o;}int64_t nexus_file_write(const char*p,const char*d){if(!p)return-1;std::FILE*f=std::fopen(p,"wb");if(!f)return-1;const char*s=d?d:"";size_t n=std::strlen(s),w=std::fwrite(s,1,n,f);int ok=w==n&&std::fclose(f)==0;return ok?0:-1;}bool nexus_file_exists(const char*p){if(!p)return false;std::FILE*f=std::fopen(p,"rb");if(!f)return false;std::fclose(f);return true;}char*nexus_env(const char*n){const char*v=n?std::getenv(n):nullptr;return nx_strdup(v?v:"");}void nexus_exit(int64_t c){std::exit(int(c));}

// Interactive terminal helpers remain available.
void nexus_screen_begin(){ } void nexus_screen_end(){ } void nexus_screen_clear(){nexus_clear();}void nexus_screen_put(int64_t x,int64_t y,const char*t){std::printf("\033[%lld;%lldH%s",(long long)(y+1),(long long)(x+1),t?t:"");}void nexus_screen_present(){std::fflush(stdout);}void nexus_screen_set_title(const char*t){(void)t;}int64_t nexus_screen_width(){return 80;}int64_t nexus_screen_height(){return 24;}bool nexus_key_pressed(){return false;}int64_t nexus_read_key(){return -1;}void nexus_beep(){std::fputs("\a",stdout);std::fflush(stdout);}

// Window / events ------------------------------------------------------------
bool nexus_gfx_init(int64_t w,int64_t h,const char* title){return init_graphics(int(w),int(h),title);}
const char* nexus_gfx_error(){return gGraphicsError.c_str();}
void nexus_gfx_shutdown(){shutdown_graphics();}
bool nexus_gfx_should_close(){return gClose;}
void nexus_gfx_poll(){process_events();}
int64_t nexus_gfx_event_type(){return gLastEvent;}
int64_t nexus_gfx_event_key(){return gLastKey;}
int64_t nexus_gfx_event_mouse_button(){return gLastMouseButton;}
int64_t nexus_gfx_event_x(){return gLastX;}
int64_t nexus_gfx_event_y(){return gLastY;}
double nexus_gfx_mouse_x(){if(gGraphics&&SDL_GetMouseState){int x=0,y=0;uint32_t b=SDL_GetMouseState(&x,&y);gLastX=x;gLastY=y;}return gLastX;}
double nexus_gfx_mouse_y(){nexus_gfx_mouse_x();return gLastY;}
bool nexus_gfx_mouse_down(int64_t button){if(button<0||button>=int64_t(gMouseButtons.size()))return false;if(gGraphics&&SDL_GetMouseState){int x,y;uint32_t b=SDL_GetMouseState(&x,&y);return button>0 && (b&(1u<<uint32_t(button-1)))!=0;}return gMouseButtons[size_t(button)]!=0;}
bool nexus_gfx_key_down(const char* name){if(!gGraphics||!SDL_GetKeyboardState)return false;int64_t sc=scancode_for(name);if(sc<0)return false;int n=0;const uint8_t*k=SDL_GetKeyboardState(&n);return sc<n&&k!=nullptr&&k[sc]!=0;}
double nexus_gfx_dt(){double n=now_seconds();double d=gLastFrame? n-gLastFrame:0.0;gLastFrame=n;return d;}
bool nexus_gfx_event_is(const char* name){if(!name)return false;std::string s(name);if(s=="quit")return gLastEvent==int(SDL_QUIT);if(s=="key_down")return gLastEvent==int(SDL_KEYDOWN);if(s=="key_up")return gLastEvent==int(SDL_KEYUP);if(s=="mouse_move")return gLastEvent==int(SDL_MOUSEMOTION);if(s=="mouse_down")return gLastEvent==int(SDL_MOUSEBUTTONDOWN);if(s=="mouse_up")return gLastEvent==int(SDL_MOUSEBUTTONUP);if(s=="window")return gLastEvent==int(SDL_WINDOWEVENT);return false;}
int64_t nexus_gfx_width(){return gWidth;} int64_t nexus_gfx_height(){return gHeight;}
bool nexus_gfx_vsync(bool enabled){return gGraphics && SDL_GL_SetSwapInterval && SDL_GL_SetSwapInterval(enabled?1:0)==0;}
void nexus_gfx_set_title(const char*title){if(gWindow&&title&&SDL_SetWindowTitle)SDL_SetWindowTitle(gWindow,title);}
void nexus_gfx_begin(){if(!gGraphics)return;glViewport(0,0,gWidth,gHeight);setup_2d();}
void nexus_gfx_end(){if(gWindow&&SDL_GL_SwapWindow)SDL_GL_SwapWindow(gWindow);}
void nexus_gfx_clear(double r,double g,double b,double a){if(!gGraphics)return;glClearColor(float(r),float(g),float(b),float(a));glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);}
void nexus_gfx_rect(double x,double y,double w,double h,double r,double g,double b,double a){if(!gGraphics)return;setup_2d();draw_quad(float(x),float(y),float(w),float(h),float(r),float(g),float(b),float(a));}
void nexus_gfx_circle(double x,double y,double radius,double r,double g,double b,double a){if(!gGraphics)return;setup_2d();draw_circle(float(x),float(y),float(radius),float(r),float(g),float(b),float(a));}
void nexus_gfx_line(double x1,double y1,double x2,double y2,double width,double r,double g,double b,double a){if(!gGraphics)return;setup_2d();glLineWidth(float(width));glColor4f(float(r),float(g),float(b),float(a));glBegin(GL_LINES);glVertex2f(float(x1),float(y1));glVertex2f(float(x2),float(y2));glEnd();}
void nexus_gfx_text(double x,double y,double scale,const char*t,double r,double g,double b,double a){if(!gGraphics)return;setup_2d();draw_text(float(x),float(y),float(scale),t,float(r),float(g),float(b),float(a));}
void nexus_gfx_set_fps(int64_t fps){if(fps<=0)return;/* language-level limiter is preferable; keep backend deterministic */}

int64_t nexus_gfx_texture_load(const char*path){if(!gGraphics||!path||!SDL_LoadBMP||!SDL_ConvertSurfaceFormat||!SDL_FreeSurface){gGraphicsError="BMP texture support is unavailable in this SDL2 runtime";return 0;}SDL_Surface*src=SDL_LoadBMP(path);if(!src)return 0;SDL_Surface*surf=SDL_ConvertSurfaceFormat(src,SDL_PIXELFORMAT_ABGR8888,0);SDL_FreeSurface(src);if(!surf)return 0;GLuint tex=0;glGenTextures(1,&tex);glBindTexture(GL_TEXTURE_2D,tex);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexImage2D(GL_TEXTURE_2D,0,4,surf->w,surf->h,0,GL_RGBA,GL_UNSIGNED_BYTE,surf->pixels);glBindTexture(GL_TEXTURE_2D,0);int64_t id=gNextTexture++;gTextures[id]=Texture{tex,surf->w,surf->h};SDL_FreeSurface(surf);return id;}
void nexus_gfx_texture_draw(int64_t id,double x,double y,double w,double h,double r,double g,double b,double a){auto it=gTextures.find(id);if(!gGraphics||it==gTextures.end())return;setup_2d();draw_textured_quad(it->second,float(x),float(y),float(w),float(h),float(r),float(g),float(b),float(a));}
void nexus_gfx_texture_unload(int64_t id){auto it=gTextures.find(id);if(it==gTextures.end())return;if(gGraphics&&it->second.id)glDeleteTextures(1,&it->second.id);gTextures.erase(it);}
int64_t nexus_gfx_texture_width(int64_t id){auto it=gTextures.find(id);return it==gTextures.end()?0:it->second.w;}int64_t nexus_gfx_texture_height(int64_t id){auto it=gTextures.find(id);return it==gTextures.end()?0:it->second.h;}

// Simple OpenGL 2.1 compatible 3D backend.
void nexus_gfx3d_begin(double fov,double nearPlane,double farPlane,double camX,double camY,double camZ,double pitch,double yaw,double roll){if(!gGraphics)return;glViewport(0,0,gWidth,gHeight);glEnable(GL_DEPTH_TEST);glMatrixMode(GL_PROJECTION);glLoadIdentity();double aspect=double(gWidth)/double(gHeight?gHeight:1);double top=nearPlane*std::tan(fov*3.14159265358979323846/360.0);double right=top*aspect;glFrustum(-right,right,-top,top,nearPlane,farPlane);glMatrixMode(GL_MODELVIEW);glLoadIdentity();glRotatef(float(-roll),0,0,1);glRotatef(float(-pitch),1,0,0);glRotatef(float(-yaw),0,1,0);glTranslatef(float(-camX),float(-camY),float(-camZ));}
void nexus_gfx3d_cube(double x,double y,double z,double sx,double sy,double sz,double r,double g,double b,double a){if(!gGraphics)return;glColor4f(float(r),float(g),float(b),float(a));float x0=float(x-sx/2),x1=float(x+sx/2),y0=float(y-sy/2),y1=float(y+sy/2),z0=float(z-sz/2),z1=float(z+sz/2);glBegin(GL_QUADS);
    glVertex3f(x0,y0,z1);glVertex3f(x1,y0,z1);glVertex3f(x1,y1,z1);glVertex3f(x0,y1,z1);glVertex3f(x1,y0,z0);glVertex3f(x0,y0,z0);glVertex3f(x0,y1,z0);glVertex3f(x1,y1,z0);glVertex3f(x0,y0,z0);glVertex3f(x0,y0,z1);glVertex3f(x0,y1,z1);glVertex3f(x0,y1,z0);glVertex3f(x1,y0,z1);glVertex3f(x1,y0,z0);glVertex3f(x1,y1,z0);glVertex3f(x1,y1,z1);glVertex3f(x0,y1,z1);glVertex3f(x1,y1,z1);glVertex3f(x1,y1,z0);glVertex3f(x0,y1,z0);glVertex3f(x0,y0,z0);glVertex3f(x1,y0,z0);glVertex3f(x1,y0,z1);glVertex3f(x0,y0,z1);glEnd();}
void nexus_gfx3d_grid(int64_t cells,double spacing,double r,double g,double b,double a){if(!gGraphics)return;glColor4f(float(r),float(g),float(b),float(a));glLineWidth(1);glBegin(GL_LINES);for(int64_t i=-cells;i<=cells;i++){float p=float(i)*float(spacing);glVertex3f(p,0,float(-cells*spacing));glVertex3f(p,0,float(cells*spacing));glVertex3f(float(-cells*spacing),0,p);glVertex3f(float(cells*spacing),0,p);}glEnd();}
void nexus_gfx3d_end(){if(!gWindow||!SDL_GL_SwapWindow)return;SDL_GL_SwapWindow(gWindow);}

// WAV audio using SDL2 core audio queue.
int64_t nexus_gfx_sound_load(const char*path){if(!load_sdl()||!path||!SDL_LoadWAV||!SDL_FreeWAV||!SDL_OpenAudioDevice||!SDL_CloseAudioDevice||!SDL_PauseAudioDevice||!SDL_QueueAudio){gGraphicsError="WAV audio support is unavailable in this SDL2 runtime";return 0;}if(SDL_Init(0x00000010u)!=0){gGraphicsError=std::string("SDL audio init failed: ")+sdl_error();return 0;}SDL_AudioSpec spec{};uint8_t*data=nullptr;uint32_t len=0;if(!SDL_LoadWAV(path,&spec,&data,&len)||!data||len==0)return 0;if(!gAudioDevice){SDL_AudioSpec want=spec;gAudioDevice=SDL_OpenAudioDevice(nullptr,0,&want,nullptr,0);if(!gAudioDevice){SDL_FreeWAV(data);return 0;}SDL_PauseAudioDevice(gAudioDevice,0);}Sound s; s.data.assign(data,data+len);s.freq=spec.freq;s.format=spec.format;s.channels=spec.channels;s.samples=spec.samples;SDL_FreeWAV(data);int64_t id=gNextSound++;gSounds[id]=std::move(s);return id;}
void nexus_gfx_sound_play(int64_t id,bool loop){auto it=gSounds.find(id);if(it==gSounds.end()||!gAudioDevice)return;/* one-shot is exact; loop is implemented by queuing a bounded repeat */int repeats=loop?32:1;for(int i=0;i<repeats;i++)SDL_QueueAudio(gAudioDevice,it->second.data.data(),uint32_t(it->second.data.size()));}
void nexus_gfx_sound_stop(){if(gAudioDevice){SDL_CloseAudioDevice(gAudioDevice);gAudioDevice=0;}}
void nexus_gfx_sound_unload(int64_t id){gSounds.erase(id);}

} // extern C
