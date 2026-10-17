// The part of the SDL3 API the game uses, built on devkitPro's SDL2 (window, OpenGL, touch,
// keyboard, mouse) and on libnx directly (controllers, because SDL2's Switch joystick driver
// does not tell a single Joy-Con held sideways from a pair) and a small mixer (sound).
// Only what the game calls is here; the names and meanings are SDL3's.
#pragma once

// SDL2's names that SDL3 uses for other things are renamed while its header is read.
#define SDL_Event SDL2_Event
#define SDL_AudioSpec SDL2_AudioSpec
#define SDL_AudioStream SDL2_AudioStream
#include <SDL.h>
#undef SDL_Event
#undef SDL_AudioSpec
#undef SDL_AudioStream

#include <switch.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <string>
#include <sys/stat.h>
#include <vector>

#ifndef SDLCALL
#define SDLCALL
#endif

// ---------------------------------------------------------------------------------------------
// Start-up, folders
// ---------------------------------------------------------------------------------------------

#define SDL_INIT_GAMEPAD 0  // controllers are read through libnx, not through SDL

namespace abx {

constexpr const char* kHome = "/switch/atomic-bomberman/";  // the SD card (libnx's default device)

inline void makeHome() {
    mkdir("/switch", 0777);
    mkdir("/switch/atomic-bomberman", 0777);
}

}  // namespace abx

inline bool ab_sdl_init(Uint32 flags) { return ::SDL_Init(flags) == 0; }
inline bool ab_sdl_init_sub(Uint32 flags) { return ::SDL_InitSubSystem(flags) == 0; }
#define SDL_Init ab_sdl_init
#define SDL_InitSubSystem ab_sdl_init_sub

// The per-user folder and the folder of the program are the one folder on the SD card.
inline char* ab_sdl_pref_path(const char*, const char*) {
    abx::makeHome();
    return SDL_strdup(abx::kHome);
}
inline char* ab_sdl_base_path() {
    abx::makeHome();
    return SDL_strdup(abx::kHome);
}
#define SDL_GetPrefPath ab_sdl_pref_path
#define SDL_GetBasePath ab_sdl_base_path

inline Uint64 SDL_GetTicksNS() {
    return static_cast<Uint64>(static_cast<double>(::SDL_GetPerformanceCounter()) * 1e9 / static_cast<double>(::SDL_GetPerformanceFrequency()));
}

// ---------------------------------------------------------------------------------------------
// Types, constants
// ---------------------------------------------------------------------------------------------

using SDL_KeyboardID = Uint32;
using SDL_MouseButtonFlags = Uint32;
using SDL_GLContextPtr = SDL_GLContext;
constexpr Uint16 SDL_KMOD_CTRL = KMOD_CTRL;
constexpr Uint16 SDL_KMOD_ALT = KMOD_ALT;
// SDL3 names its letter keys with capitals.
constexpr SDL_Keycode SDLK_A = SDLK_a;
constexpr SDL_Keycode SDLK_B = SDLK_b;
constexpr SDL_Keycode SDLK_C = SDLK_c;
constexpr SDL_Keycode SDLK_D = SDLK_d;
constexpr SDL_Keycode SDLK_E = SDLK_e;
constexpr SDL_Keycode SDLK_F = SDLK_f;
constexpr SDL_Keycode SDLK_G = SDLK_g;
constexpr SDL_Keycode SDLK_H = SDLK_h;
constexpr SDL_Keycode SDLK_I = SDLK_i;
constexpr SDL_Keycode SDLK_J = SDLK_j;
constexpr SDL_Keycode SDLK_K = SDLK_k;
constexpr SDL_Keycode SDLK_L = SDLK_l;
constexpr SDL_Keycode SDLK_M = SDLK_m;
constexpr SDL_Keycode SDLK_N = SDLK_n;
constexpr SDL_Keycode SDLK_O = SDLK_o;
constexpr SDL_Keycode SDLK_P = SDLK_p;
constexpr SDL_Keycode SDLK_Q = SDLK_q;
constexpr SDL_Keycode SDLK_R = SDLK_r;
constexpr SDL_Keycode SDLK_S = SDLK_s;
constexpr SDL_Keycode SDLK_T = SDLK_t;
constexpr SDL_Keycode SDLK_U = SDLK_u;
constexpr SDL_Keycode SDLK_V = SDLK_v;
constexpr SDL_Keycode SDLK_W = SDLK_w;
constexpr SDL_Keycode SDLK_X = SDLK_x;
constexpr SDL_Keycode SDLK_Y = SDLK_y;
constexpr SDL_Keycode SDLK_Z = SDLK_z;

enum : Uint32 {
    SDL_EVENT_QUIT = 0x100,
    SDL_EVENT_KEY_DOWN = 0x300,
    SDL_EVENT_TEXT_INPUT = 0x303,
    SDL_EVENT_MOUSE_MOTION = 0x400,
    SDL_EVENT_MOUSE_BUTTON_DOWN = 0x401,
    SDL_EVENT_MOUSE_BUTTON_UP = 0x402,
    SDL_EVENT_FINGER_DOWN = 0x700,
    SDL_EVENT_FINGER_UP = 0x701,
    SDL_EVENT_FINGER_MOTION = 0x702,
    SDL_EVENT_GAMEPAD_AXIS_MOTION = 0x650,
    SDL_EVENT_GAMEPAD_BUTTON_DOWN = 0x651,
    SDL_EVENT_GAMEPAD_BUTTON_UP = 0x652,
    SDL_EVENT_GAMEPAD_ADDED = 0x653,
    SDL_EVENT_GAMEPAD_REMOVED = 0x654,
};

struct SDL_Event {
    Uint32 type = 0;
    struct {
        SDL_KeyboardID which;
        SDL_Keycode key;
        Uint16 mod;
        SDL_Scancode scancode;
        bool down;
        bool repeat;
    } key{};
    struct {
        const char* text;
    } text{};
    struct {
        Uint32 which;
        float x, y;
        Uint8 button;
    } button{};
    struct {
        Uint32 which;
        float x, y;
    } motion{};
    struct {
        SDL_FingerID fingerID;
        float x, y;
    } tfinger{};
    struct {
        SDL_JoystickID which;
        Uint8 button;
    } gbutton{};
    struct {
        SDL_JoystickID which;
    } gaxis{};
    struct {
        SDL_JoystickID which;
    } gdevice{};
};

// ---------------------------------------------------------------------------------------------
// Controllers (libnx HID)
// ---------------------------------------------------------------------------------------------

enum SDL_GamepadButton {
    SDL_GAMEPAD_BUTTON_SOUTH,  // the Switch's A: "confirm" and the first action button
    SDL_GAMEPAD_BUTTON_EAST,   // B
    SDL_GAMEPAD_BUTTON_WEST,   // Y (and the side buttons of a Joy-Con held sideways)
    SDL_GAMEPAD_BUTTON_NORTH,  // X
    SDL_GAMEPAD_BUTTON_START,  // +
    SDL_GAMEPAD_BUTTON_DPAD_UP,
    SDL_GAMEPAD_BUTTON_DPAD_DOWN,
    SDL_GAMEPAD_BUTTON_DPAD_LEFT,
    SDL_GAMEPAD_BUTTON_DPAD_RIGHT,
    SDL_GAMEPAD_BUTTON_COUNT
};
enum SDL_GamepadAxis { SDL_GAMEPAD_AXIS_LEFTX, SDL_GAMEPAD_AXIS_LEFTY };

namespace abx {

constexpr int kPads = 9;  // players 1-8 and the handheld's own

enum class Kind { Standard, JoyLeft, JoyRight };  // a Joy-Con held sideways on its own is a pad of its own

struct Slot {
    PadState pad;
    bool connected = false;
    Kind kind = Kind::Standard;
    u32 style = 0, type = 0;
    std::array<bool, SDL_GAMEPAD_BUTTON_COUNT> held{};
    int x = 0, y = 0;
    int loggedPresses = 0;
};

inline std::array<Slot, kPads>& slots() {
    static std::array<Slot, kPads> s;
    return s;
}
inline std::deque<SDL_Event>& queue() {
    static std::deque<SDL_Event> q;
    return q;
}

inline HidNpadIdType idOf(int slot) { return slot < 8 ? static_cast<HidNpadIdType>(HidNpadIdType_No1 + slot) : HidNpadIdType_Handheld; }

// What the SD card's log.txt says about the controllers: enough to settle from one test run which
// way a Joy-Con held sideways reports its buttons and stick.
inline void logLine(const char* what, int slot, const Slot& s, u64 buttons = 0, int sx = 0, int sy = 0, int rx = 0, int ry = 0) {
    std::fprintf(stderr, "PAD   %s slot=%d style=0x%x type=0x%x kind=%d buttons=0x%llx stickL=%d,%d stickR=%d,%d\n", what, slot, s.style, s.type,
                 static_cast<int>(s.kind), static_cast<unsigned long long>(buttons), sx, sy, rx, ry);
    std::fflush(stderr);
}

inline bool& controllersStarted() {
    static bool started = false;
    return started;
}

// Once, after SDL has started (its own joystick driver is not used, so it does not touch these settings).
inline void startControllers() {
    if (controllersStarted()) return;
    controllersStarted() = true;
    // Eight players; Joy-Con held sideways, each alone, play as separate controllers.
    padConfigureInput(8, HidNpadStyleSet_NpadStandard);
    hidSetNpadJoyHoldType(HidNpadJoyHoldType_Horizontal);
    for (int i = 0; i < kPads; ++i) padInitializeWithMask(&slots()[static_cast<std::size_t>(i)].pad, 1ull << idOf(i));
}

// The system's controller screen, where a pair of Joy-Con is split into two players (or joined again)
// and controllers are assigned.
inline void openControllerApplet() {
    HidLaControllerSupportResultInfo info;
    HidLaControllerSupportArg arg;
    hidLaCreateControllerSupportArg(&arg);
    arg.hdr.player_count_max = 8;
    arg.hdr.enable_permit_joy_dual = 1;
    arg.hdr.enable_take_over_connection = 1;
    hidLaShowControllerSupportForSystem(&info, &arg, false);
}

inline Kind kindOf(u32 style, u32 type) {
    if (style & HidNpadStyleTag_NpadJoyDual) return Kind::Standard;
    if (type == HidDeviceTypeBits_JoyLeft) return Kind::JoyLeft;
    if (type == HidDeviceTypeBits_JoyRight) return Kind::JoyRight;
    return Kind::Standard;
}

// The buttons as the game knows them. A Joy-Con on its own is held sideways, and the mapping is
// the same one devkitPro's SDL2 uses for it: its four arrow buttons (left one) or four face
// buttons (right one) are the face buttons, and its stick's four directions are the arrows.
inline void readButtons(const Slot& s, std::array<bool, SDL_GAMEPAD_BUTTON_COUNT>& out, bool& applet) {
    const u64 b = padGetButtons(&s.pad);
    auto has = [&](u64 mask) { return (b & mask) != 0; };
    const u64 shoulders = HidNpadButton_L | HidNpadButton_R | HidNpadButton_ZL | HidNpadButton_ZR;
    applet = false;
    if (s.kind == Kind::JoyLeft) {
        out[SDL_GAMEPAD_BUTTON_SOUTH] = has(HidNpadButton_Down);
        out[SDL_GAMEPAD_BUTTON_EAST] = has(HidNpadButton_Left | HidNpadButton_LeftSL);
        out[SDL_GAMEPAD_BUTTON_NORTH] = has(HidNpadButton_Right);
        out[SDL_GAMEPAD_BUTTON_WEST] = has(HidNpadButton_Up | HidNpadButton_LeftSR);
        out[SDL_GAMEPAD_BUTTON_START] = has(HidNpadButton_StickL);
        out[SDL_GAMEPAD_BUTTON_DPAD_LEFT] = has(HidNpadButton_StickLUp);
        out[SDL_GAMEPAD_BUTTON_DPAD_UP] = has(HidNpadButton_StickLRight);
        out[SDL_GAMEPAD_BUTTON_DPAD_RIGHT] = has(HidNpadButton_StickLDown);
        out[SDL_GAMEPAD_BUTTON_DPAD_DOWN] = has(HidNpadButton_StickLLeft);
        applet = has(HidNpadButton_Minus);
    } else if (s.kind == Kind::JoyRight) {
        out[SDL_GAMEPAD_BUTTON_SOUTH] = has(HidNpadButton_X);
        out[SDL_GAMEPAD_BUTTON_EAST] = has(HidNpadButton_A | HidNpadButton_RightSL);
        out[SDL_GAMEPAD_BUTTON_NORTH] = has(HidNpadButton_Y);
        out[SDL_GAMEPAD_BUTTON_WEST] = has(HidNpadButton_B | HidNpadButton_RightSR);
        out[SDL_GAMEPAD_BUTTON_START] = has(HidNpadButton_StickR);
        out[SDL_GAMEPAD_BUTTON_DPAD_LEFT] = has(HidNpadButton_StickRDown);
        out[SDL_GAMEPAD_BUTTON_DPAD_UP] = has(HidNpadButton_StickRLeft);
        out[SDL_GAMEPAD_BUTTON_DPAD_RIGHT] = has(HidNpadButton_StickRUp);
        out[SDL_GAMEPAD_BUTTON_DPAD_DOWN] = has(HidNpadButton_StickRRight);
        applet = has(HidNpadButton_Plus);
    } else {
        out[SDL_GAMEPAD_BUTTON_SOUTH] = has(HidNpadButton_A);
        out[SDL_GAMEPAD_BUTTON_EAST] = has(HidNpadButton_B);
        out[SDL_GAMEPAD_BUTTON_NORTH] = has(HidNpadButton_X);
        out[SDL_GAMEPAD_BUTTON_WEST] = has(HidNpadButton_Y | shoulders);
        out[SDL_GAMEPAD_BUTTON_START] = has(HidNpadButton_Plus);
        out[SDL_GAMEPAD_BUTTON_DPAD_UP] = has(HidNpadButton_Up);
        out[SDL_GAMEPAD_BUTTON_DPAD_DOWN] = has(HidNpadButton_Down);
        out[SDL_GAMEPAD_BUTTON_DPAD_LEFT] = has(HidNpadButton_Left);
        out[SDL_GAMEPAD_BUTTON_DPAD_RIGHT] = has(HidNpadButton_Right);
        applet = has(HidNpadButton_Minus);
    }
}

// Reads every controller and queues what changed since the last time, as SDL3 would report it.
inline void pollControllers() {
    if (!controllersStarted()) startControllers();
    bool wantApplet = false;
    for (int i = 0; i < kPads; ++i) {
        Slot& s = slots()[static_cast<std::size_t>(i)];
        padUpdate(&s.pad);
        const bool now = padGetStyleSet(&s.pad) != 0 && padIsConnected(&s.pad);
        const SDL_JoystickID id = i + 1;
        if (now) {
            const u32 style = padGetStyleSet(&s.pad), type = hidGetNpadDeviceType(idOf(i));
            const Kind kind = kindOf(style, type);
            if (kind != s.kind || style != s.style || type != s.type) {
                s.style = style, s.type = type, s.kind = kind;
                s.held = {};
                s.x = s.y = 0;
                logLine(s.connected ? "changed" : "found", i, s);
            }
        }
        if (now != s.connected) {
            s.connected = now;
            if (!now) logLine("lost", i, s);
            SDL_Event e;
            e.type = now ? SDL_EVENT_GAMEPAD_ADDED : SDL_EVENT_GAMEPAD_REMOVED;
            e.gdevice.which = id;
            queue().push_back(e);
            s.held = {};
            s.x = s.y = 0;
            continue;
        }
        if (!now) continue;
        std::array<bool, SDL_GAMEPAD_BUTTON_COUNT> held{};
        bool applet = false;
        readButtons(s, held, applet);
        wantApplet = wantApplet || (applet && padGetButtonsDown(&s.pad) != 0);
        if (s.loggedPresses < 40 && padGetButtonsDown(&s.pad) != 0) {  // the first presses, as raw as they come
            ++s.loggedPresses;
            const HidAnalogStickState l = padGetStickPos(&s.pad, 0), r = padGetStickPos(&s.pad, 1);
            logLine("press", i, s, padGetButtonsDown(&s.pad), l.x, l.y, r.x, r.y);
        }
        for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; ++b) {
            if (held[static_cast<std::size_t>(b)] == s.held[static_cast<std::size_t>(b)]) continue;
            SDL_Event e;
            e.type = held[static_cast<std::size_t>(b)] ? SDL_EVENT_GAMEPAD_BUTTON_DOWN : SDL_EVENT_GAMEPAD_BUTTON_UP;
            e.gbutton.which = id;
            e.gbutton.button = static_cast<Uint8>(b);
            queue().push_back(e);
        }
        s.held = held;
        // The stick's two axes: only a pad that is not on its own has one the game reads (alone, its
        // stick is four arrow buttons above).
        int x = 0, y = 0;
        if (s.kind == Kind::Standard) {
            const HidAnalogStickState l = padGetStickPos(&s.pad, 0), r = padGetStickPos(&s.pad, 1);
            const bool useRight = std::abs(r.x) + std::abs(r.y) > std::abs(l.x) + std::abs(l.y);
            const HidAnalogStickState st = useRight ? r : l;
            x = st.x, y = -st.y;  // libnx: up is positive; SDL: down is positive
        }
        if (x != s.x || y != s.y) {
            s.x = x;
            s.y = y;
            SDL_Event e;
            e.type = SDL_EVENT_GAMEPAD_AXIS_MOTION;
            e.gaxis.which = id;
            queue().push_back(e);
        }
    }
    if (wantApplet) {
        std::fprintf(stderr, "PAD   opening the controller screen\n");
        std::fflush(stderr);
        openControllerApplet();
    }
}

}  // namespace abx

struct SDL_Gamepad {
    int slot;
};

inline SDL_JoystickID* SDL_GetGamepads(int* count) {
    abx::pollControllers();  // the first call also finds the ones already there (their ADDED events are queued too)
    auto* ids = static_cast<SDL_JoystickID*>(SDL_malloc(sizeof(SDL_JoystickID) * abx::kPads));
    int n = 0;
    for (int i = 0; i < abx::kPads; ++i)
        if (abx::slots()[static_cast<std::size_t>(i)].connected) ids[n++] = i + 1;
    if (count != nullptr) *count = n;
    return ids;
}
inline SDL_Gamepad* SDL_OpenGamepad(SDL_JoystickID id) {
    if (id < 1 || id > abx::kPads) return nullptr;
    return new SDL_Gamepad{id - 1};
}
inline void SDL_CloseGamepad(SDL_Gamepad* pad) { delete pad; }
inline SDL_JoystickID SDL_GetGamepadID(SDL_Gamepad* pad) { return pad->slot + 1; }
inline const char* SDL_GetGamepadName(SDL_Gamepad* pad) {
    const u32 style = padGetStyleSet(&abx::slots()[static_cast<std::size_t>(pad->slot)].pad);
    if (style & HidNpadStyleTag_NpadHandheld) return "Switch handheld";
    if (style & HidNpadStyleTag_NpadJoyDual) return "Joy-Con pair";
    if (style & HidNpadStyleTag_NpadJoyLeft) return "Joy-Con (L)";
    if (style & HidNpadStyleTag_NpadJoyRight) return "Joy-Con (R)";
    if (style & HidNpadStyleTag_NpadFullKey) return "Pro Controller";
    return "Controller";
}
inline bool SDL_GetGamepadButton(SDL_Gamepad* pad, SDL_GamepadButton b) {
    return abx::slots()[static_cast<std::size_t>(pad->slot)].held[static_cast<std::size_t>(b)];
}
inline Sint16 SDL_GetGamepadAxis(SDL_Gamepad* pad, SDL_GamepadAxis a) {
    const abx::Slot& s = abx::slots()[static_cast<std::size_t>(pad->slot)];
    return static_cast<Sint16>(a == SDL_GAMEPAD_AXIS_LEFTX ? s.x : s.y);
}

// ---------------------------------------------------------------------------------------------
// Events, keyboard, mouse, window
// ---------------------------------------------------------------------------------------------

inline bool SDL_PushEvent(SDL_Event* e) {
    abx::queue().push_back(*e);
    return true;
}

inline bool SDL_PollEvent(SDL_Event* out) {
    SDL2_Event e2;
    while (::SDL_PollEvent(&e2) != 0) {
        SDL_Event e;
        switch (e2.type) {
            case SDL_QUIT: e.type = SDL_EVENT_QUIT; break;
            case SDL_KEYDOWN:
                e.type = SDL_EVENT_KEY_DOWN;
                e.key.which = 0;
                e.key.key = e2.key.keysym.sym;
                e.key.mod = e2.key.keysym.mod;
                e.key.scancode = e2.key.keysym.scancode;
                e.key.down = true;
                e.key.repeat = e2.key.repeat != 0;
                break;
            case SDL_TEXTINPUT:
                e.type = SDL_EVENT_TEXT_INPUT;
                {  // the characters live on in a ring of buffers, as SDL3's string does until the next event
                    static char ring[8][32];
                    static int next = 0;
                    char* slot = ring[next++ % 8];
                    std::strncpy(slot, e2.text.text, 31);
                    slot[31] = 0;
                    e.text.text = slot;
                }
                break;
            case SDL_MOUSEMOTION:
                e.type = SDL_EVENT_MOUSE_MOTION;
                e.motion.which = e2.motion.which;
                e.motion.x = static_cast<float>(e2.motion.x);
                e.motion.y = static_cast<float>(e2.motion.y);
                break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                e.type = e2.type == SDL_MOUSEBUTTONDOWN ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
                e.button.which = e2.button.which;
                e.button.x = static_cast<float>(e2.button.x);
                e.button.y = static_cast<float>(e2.button.y);
                e.button.button = e2.button.button;
                break;
            case SDL_FINGERDOWN:
            case SDL_FINGERUP:
            case SDL_FINGERMOTION:
                e.type = e2.type == SDL_FINGERDOWN ? SDL_EVENT_FINGER_DOWN : e2.type == SDL_FINGERUP ? SDL_EVENT_FINGER_UP : SDL_EVENT_FINGER_MOTION;
                e.tfinger.fingerID = e2.tfinger.fingerId;
                e.tfinger.x = e2.tfinger.x;
                e.tfinger.y = e2.tfinger.y;
                break;
            default: continue;
        }
        abx::queue().push_back(e);
    }
    if (abx::queue().empty()) abx::pollControllers();
    if (abx::queue().empty()) return false;
    *out = abx::queue().front();
    abx::queue().pop_front();
    return true;
}

inline const bool* SDL_GetKeyboardStateBool(int* numkeys) {
    static bool keys[SDL_NUM_SCANCODES];
    const Uint8* now = ::SDL_GetKeyboardState(numkeys);
    for (int i = 0; i < SDL_NUM_SCANCODES; ++i) keys[i] = now[i] != 0;
    return keys;
}
#define SDL_GetKeyboardState SDL_GetKeyboardStateBool

inline SDL_MouseButtonFlags SDL_GetMouseState(float* x, float* y) {
    int ix = 0, iy = 0;
    const Uint32 b = ::SDL_GetMouseState(&ix, &iy);
    if (x != nullptr) *x = static_cast<float>(ix);
    if (y != nullptr) *y = static_cast<float>(iy);
    return b;
}

inline SDL_Window* SDL_CreateWindow(const char* title, int w, int h, Uint32 flags) {
    return ::SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, flags);
}
inline void SDL_GL_DestroyContext(SDL_GLContext c) { ::SDL_GL_DeleteContext(c); }
inline void SDL_StartTextInput(SDL_Window*) {}
inline void SDL_StopTextInput(SDL_Window*) {}

// No folder chooser on the console: the answer is "none", and the game says where to put the files.
inline void SDL_ShowOpenFolderDialog(void(SDLCALL* callback)(void*, const char* const*, int), void* userdata, SDL_Window*, const char*, bool) {
    callback(userdata, nullptr, -1);
}

// ---------------------------------------------------------------------------------------------
// Sound: streams mixed into one SDL2 playback device
// ---------------------------------------------------------------------------------------------

using SDL_AudioFormat2 = SDL_AudioFormat;
struct SDL_AudioSpec {
    SDL_AudioFormat2 format;
    int channels;
    int freq;
};
constexpr SDL_AudioFormat2 SDL_AUDIO_S16LE = AUDIO_S16LSB;
constexpr SDL_AudioDeviceID SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK = 0xFFFFFFFFu;

struct SDL_AudioStream {
    SDL_AudioSpec in{};
    float gain = 1.0f;
    bool bound = false;
    std::vector<Sint16> pcm;  // stereo, at the device's rate
    std::size_t at = 0;       // how much of it has been played
};

namespace abx {

constexpr int kRate = 22050;

inline std::vector<SDL_AudioStream*>& streams() {
    static std::vector<SDL_AudioStream*> v;
    return v;
}
inline SDL_AudioDeviceID& device() {
    static SDL_AudioDeviceID d = 0;
    return d;
}

inline void SDLCALL mix(void*, Uint8* out, int len) {
    auto* o = reinterpret_cast<Sint16*>(out);
    const std::size_t samples = static_cast<std::size_t>(len) / sizeof(Sint16);
    std::vector<int> sum(samples, 0);
    for (SDL_AudioStream* s : streams()) {
        const std::size_t n = std::min(samples, s->pcm.size() - s->at);
        for (std::size_t i = 0; i < n; ++i) sum[i] += static_cast<int>(static_cast<float>(s->pcm[s->at + i]) * s->gain);
        s->at += n;
        if (s->at > 65536 && s->at * 2 > s->pcm.size()) {  // forget what has been played
            s->pcm.erase(s->pcm.begin(), s->pcm.begin() + static_cast<std::ptrdiff_t>(s->at));
            s->at = 0;
        }
    }
    for (std::size_t i = 0; i < samples; ++i) o[i] = static_cast<Sint16>(std::clamp(sum[i], -32768, 32767));
}

}  // namespace abx

inline SDL_AudioDeviceID SDL_OpenAudioDevice(SDL_AudioDeviceID, const SDL_AudioSpec*) {
    SDL2_AudioSpec want{}, have{};
    want.freq = abx::kRate;
    want.format = AUDIO_S16LSB;
    want.channels = 2;
    want.samples = 1024;
    want.callback = abx::mix;
    const SDL_AudioDeviceID id = ::SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
    if (id == 0) return 0;
    abx::device() = id;
    ::SDL_PauseAudioDevice(id, 0);
    return id;
}
inline void SDL_CloseAudioDevice2(SDL_AudioDeviceID id) {
    ::SDL_CloseAudioDevice(id);
    abx::device() = 0;
}
#define SDL_CloseAudioDevice SDL_CloseAudioDevice2

inline SDL_AudioStream* SDL_CreateAudioStream(const SDL_AudioSpec* in, const SDL_AudioSpec*) {
    auto* s = new SDL_AudioStream;
    s->in = *in;
    return s;
}
inline bool SDL_BindAudioStream(SDL_AudioDeviceID, SDL_AudioStream* s) {
    if (abx::device() == 0) return false;
    ::SDL_LockAudioDevice(abx::device());
    if (!s->bound) abx::streams().push_back(s);
    s->bound = true;
    ::SDL_UnlockAudioDevice(abx::device());
    return true;
}
inline void SDL_DestroyAudioStream(SDL_AudioStream* s) {
    if (s == nullptr) return;
    if (abx::device() != 0) ::SDL_LockAudioDevice(abx::device());
    auto& v = abx::streams();
    v.erase(std::remove(v.begin(), v.end(), s), v.end());
    if (abx::device() != 0) ::SDL_UnlockAudioDevice(abx::device());
    delete s;
}
inline bool SDL_SetAudioStreamGain(SDL_AudioStream* s, float gain) {
    s->gain = gain;
    return true;
}
inline bool SDL_PutAudioStreamData(SDL_AudioStream* s, const void* data, int len) {
    const auto* in = static_cast<const Sint16*>(data);
    const int ch = std::max(1, s->in.channels);
    const std::size_t frames = static_cast<std::size_t>(len) / (sizeof(Sint16) * static_cast<std::size_t>(ch));
    const double step = static_cast<double>(s->in.freq) / abx::kRate;
    const std::size_t outFrames = static_cast<std::size_t>(static_cast<double>(frames) / step);
    std::vector<Sint16> add;
    add.reserve(outFrames * 2);
    for (std::size_t i = 0; i < outFrames; ++i) {
        const std::size_t src = std::min(frames - 1, static_cast<std::size_t>(static_cast<double>(i) * step));
        const Sint16 l = in[src * static_cast<std::size_t>(ch)];
        const Sint16 r = ch > 1 ? in[src * static_cast<std::size_t>(ch) + 1] : l;
        add.push_back(l);
        add.push_back(r);
    }
    if (abx::device() != 0) ::SDL_LockAudioDevice(abx::device());
    s->pcm.insert(s->pcm.end(), add.begin(), add.end());
    if (abx::device() != 0) ::SDL_UnlockAudioDevice(abx::device());
    return true;
}
inline bool SDL_FlushAudioStream(SDL_AudioStream*) { return true; }
inline int SDL_GetAudioStreamQueued(SDL_AudioStream* s) {
    return static_cast<int>((s->pcm.size() - s->at) * sizeof(Sint16));
}
