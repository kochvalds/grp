// GRAND RESCUE PROTOCOL v2 - open-world parody game (C++17, raylib 5.0)
// Made by the fictional studio "KZstar Kokshetay". Unofficial parody, not affiliated with
// Rockstar Games / Take-Two. All content (textures, music, sounds) is original and procedural.
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <vector>
#include <string>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <algorithm>

static const int RW = 854, RH = 480, N = 40;
static const float P = 44.f, RHW = 7.f, WORLD = N * P;
static const Vector3 START = {32 * P, 0, 20 * P + 9};

// ================================================================ utils
static float rf(float a, float b) { return a + (b - a) * (float)GetRandomValue(0, 10000) / 10000.f; }
static float wrapA(float a) { while (a > PI) a -= 2 * PI; while (a < -PI) a += 2 * PI; return a; }
static float lerpA(float a, float b, float t) { return wrapA(a + wrapA(b - a) * Clamp(t, 0, 1)); }
static float d2(Vector3 a, Vector3 b) { float x = a.x - b.x, z = a.z - b.z; return sqrtf(x * x + z * z); }
static Vector3 IJ(int i, int j) { return {i * P, 0, j * P}; }
static Vector3 add(Vector3 a, Vector3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
static Color mixC(Color a, Color b, float t) {
    t = Clamp(t, 0, 1);
    return {(unsigned char)(a.r + (b.r - a.r) * t), (unsigned char)(a.g + (b.g - a.g) * t), (unsigned char)(a.b + (b.b - a.b) * t), 255};
}
static Vector3 dirv(int d) { static const Vector3 v[4] = {{0, 0, 1}, {1, 0, 0}, {0, 0, -1}, {-1, 0, 0}}; return v[d]; }
static const float DY[4] = {0, PI / 2, PI, -PI / 2};
static unsigned sR = 12345;
static float nz() { sR = sR * 1664525u + 1013904223u; return ((sR >> 9) & 0xFFFF) / 32768.f - 1.f; }

// ================================================================ sky / light
static Color gTop, gHor;
static float gL = 1, gHour = 18.5f, gRain = 0, gRainT = 80, gT = 0;
static bool gRainOn = false;
static Color lit(Color c) { return {(unsigned char)(c.r * gL), (unsigned char)(c.g * gL), (unsigned char)(c.b * gL), 255}; }
static Color F(Color c, float d) { return mixC(lit(c), gHor, (d - 70.f) / 150.f); }
static Color Fe(Color c, float d) { return mixC(c, gHor, (d - 70.f) / 150.f); }
static Color tintC(Color c, float d, bool night) {
    float L = night ? fmaxf(gL, 0.75f) : gL;
    Color l = {(unsigned char)(c.r * L), (unsigned char)(c.g * L), (unsigned char)(c.b * L), 255};
    return mixC(l, gHor, (d - 70.f) / 150.f);
}
static void updateSky() {
    struct K { float h; Color t, b; float l; };
    static const K k[] = {
        {0, {8, 8, 30, 255}, {50, 15, 80, 255}, .4f},
        {6, {100, 70, 150, 255}, {255, 160, 130, 255}, .8f},
        {12, {70, 140, 230, 255}, {190, 225, 255, 255}, 1.f},
        {18, {70, 25, 120, 255}, {255, 105, 140, 255}, .85f},
        {21, {8, 8, 30, 255}, {50, 15, 80, 255}, .4f},
        {24.01f, {8, 8, 30, 255}, {50, 15, 80, 255}, .4f}};
    int i = 0;
    while (i < 4 && gHour >= k[i + 1].h) i++;
    float t = (gHour - k[i].h) / (k[i + 1].h - k[i].h);
    gTop = mixC(k[i].t, k[i + 1].t, t);
    gHor = mixC(k[i].b, k[i + 1].b, t);
    gL = k[i].l + (k[i + 1].l - k[i].l) * t;
    Color gr = {110, 115, 125, 255};
    gTop = mixC(gTop, gr, gRain * .6f);
    gHor = mixC(gHor, gr, gRain * .6f);
    gL *= 1 - .3f * gRain;
}

// ================================================================ procedural textures (all original)
static Texture2D tFac[6][2], tAsph, tSide, tGrass, tSand, tConc, tWater, tRoof;
static Color jit(Color c, int a) {
    int d = GetRandomValue(-a, a);
    return {(unsigned char)Clamp((float)(c.r + d), 0, 255), (unsigned char)Clamp((float)(c.g + d), 0, 255), (unsigned char)Clamp((float)(c.b + d), 0, 255), 255};
}
static void noiseFill(Image& im, Color c, int a) { for (int y = 0; y < im.height; y++) for (int x = 0; x < im.width; x++) ImageDrawPixel(&im, x, y, jit(c, a)); }
static void rect(Image& im, int x, int y, int w, int h, Color c) { ImageDrawRectangle(&im, x, y, w, h, c); }
static Texture2D toTex(Image im, const char* name) {  // optional override: assets/tex/<name>.png (your own licensed textures)
    char fn[96]; snprintf(fn, sizeof fn, "assets/tex/%s.png", name);
    if (FileExists(fn)) { UnloadImage(im); im = LoadImage(fn); }
    Texture2D t = LoadTextureFromImage(im); UnloadImage(im);
    SetTextureWrap(t, TEXTURE_WRAP_REPEAT); SetTextureFilter(t, TEXTURE_FILTER_POINT);
    return t;
}
static Image genFacade(int style, bool night) {
    SetRandomSeed(100 + style * 2 + (night ? 1 : 0));
    Image im = GenImageColor(64, 64, BLANK);
    Color wall = night ? Color{120, 118, 132, 255} : Color{238, 234, 228, 255};
    if (style == 4) wall = night ? Color{100, 60, 55, 255} : Color{185, 95, 70, 255};
    if (style == 5) wall = night ? Color{105, 108, 118, 255} : Color{160, 165, 175, 255};
    noiseFill(im, wall, 6);
    auto win = [&](int x, int y, int w, int h) {
        bool on = night && GetRandomValue(0, 99) < 45;
        Color g = night ? (on ? Color{255, 226, 130, 255} : Color{25, 30, 52, 255}) : Color{45, 68, 110, 255};
        rect(im, x - 1, y - 1, w + 2, h + 2, night ? Color{60, 60, 75, 255} : Color{250, 250, 248, 255});
        rect(im, x, y, w, h, g);
        if (!night) { rect(im, x, y, w, 2, {120, 165, 215, 255}); rect(im, x, y + h / 2, w, 1, {30, 50, 85, 255}); }
        else if (on) rect(im, x, y + h / 2, w, 1, {255, 200, 100, 255});
    };
    switch (style) {
    case 0: win(6, 6, 20, 20); win(38, 6, 20, 20); win(6, 36, 20, 20); win(38, 36, 20, 20); break;
    case 1: for (int r = 0; r < 2; r++) for (int c = 0; c < 4; c++) win(2 + c * 15, 6 + r * 32, 13, 16); break;
    case 2: for (int c = 0; c < 3; c++) { win(5 + c * 21, 5, 12, 24); win(5 + c * 21, 36, 12, 24); } break;
    case 3: win(8, 6, 20, 26); win(36, 6, 20, 26); rect(im, 0, 52, 64, 6, night ? Color{110, 70, 90, 255} : Color{205, 120, 165, 255}); rect(im, 0, 60, 64, 4, {60, 60, 70, 255}); break;
    case 4:
        for (int row = 0; row < 16; row++) {
            int y = row * 4; rect(im, 0, y + 3, 64, 1, {215, 205, 190, 255});
            for (int x = (row & 1) ? 0 : 4; x < 64; x += 8) rect(im, x, y, 1, 3, {215, 205, 190, 255});
        }
        win(24, 14, 16, 22); break;
    default:
        for (int x = 0; x < 64; x += 4) rect(im, x, 0, 2, 64, night ? Color{85, 88, 98, 255} : Color{140, 145, 155, 255});
        for (int c = 0; c < 4; c++) win(4 + c * 15, 8, 11, 8);
        rect(im, 22, 34, 20, 30, {70, 75, 85, 255}); break;
    }
    return im;
}
static void initTextures() {
    for (int s = 0; s < 6; s++) for (int n = 0; n < 2; n++) { char nm[32]; snprintf(nm, sizeof nm, "facade%d%s", s, n ? "_night" : ""); tFac[s][n] = toTex(genFacade(s, n != 0), nm); }
    SetRandomSeed(7);
    Image a = GenImageColor(64, 64, BLANK); noiseFill(a, {68, 68, 76, 255}, 5); for (int i = 0; i < 100; i++) ImageDrawPixel(&a, GetRandomValue(0, 63), GetRandomValue(0, 63), {112, 112, 120, 255}); tAsph = toTex(a, "asphalt");
    Image s = GenImageColor(64, 64, BLANK); noiseFill(s, {190, 185, 195, 255}, 5); for (int k = 0; k < 4; k++) { rect(s, 0, k * 16, 64, 1, {150, 146, 156, 255}); rect(s, k * 16, 0, 1, 64, {150, 146, 156, 255}); } tSide = toTex(s, "sidewalk");
    Image g = GenImageColor(64, 64, BLANK); noiseFill(g, {55, 150, 80, 255}, 10); for (int i = 0; i < 40; i++) ImageDrawPixel(&g, GetRandomValue(0, 63), GetRandomValue(0, 63), (i % 7 == 0) ? Color{255, 130, 170, 255} : Color{35, 110, 60, 255}); tGrass = toTex(g, "grass");
    Image sa = GenImageColor(64, 64, BLANK); noiseFill(sa, {232, 212, 160, 255}, 6); for (int i = 0; i < 60; i++) ImageDrawPixel(&sa, GetRandomValue(0, 63), GetRandomValue(0, 63), {200, 180, 130, 255}); tSand = toTex(sa, "sand");
    Image c = GenImageColor(64, 64, BLANK); noiseFill(c, {140, 140, 148, 255}, 7); rect(c, 0, 31, 64, 1, {110, 110, 118, 255}); rect(c, 31, 0, 1, 64, {110, 110, 118, 255}); tConc = toTex(c, "concrete");
    Image w = GenImageColor(64, 64, BLANK); noiseFill(w, {28, 145, 190, 255}, 6); for (int i = 0; i < 45; i++) rect(w, GetRandomValue(0, 63), GetRandomValue(0, 63), GetRandomValue(4, 10), 1, {120, 215, 235, 255}); tWater = toTex(w, "water");
    Image r = GenImageColor(64, 64, BLANK); noiseFill(r, {84, 78, 88, 255}, 8); rect(r, 8, 8, 22, 14, {125, 130, 138, 255}); rect(r, 10, 10, 18, 10, {105, 110, 118, 255}); rect(r, 42, 34, 12, 12, {125, 130, 138, 255}); ImageDrawCircle(&r, 48, 14, 5, {70, 70, 78, 255}); tRoof = toTex(r, "roof");
}

// ================================================================ data
struct Line { const char* who; const char* text; };
struct Look { const char* n; Color shirt, pants, skin, hair; int acc; };  // acc: 1 cap, 2 sunglasses, 4 chain, 8 long hair
static const Look LOOKS[] = {
    {"ARMAN", {255, 90, 160, 255}, {240, 240, 240, 255}, {225, 180, 140, 255}, {30, 25, 25, 255}, 2},
    {"BOLAT", {205, 150, 50, 255}, {190, 170, 130, 255}, {205, 160, 120, 255}, {170, 170, 170, 255}, 1},
    {"KENZHE", {50, 100, 190, 255}, {50, 100, 190, 255}, {200, 150, 110, 255}, {25, 25, 25, 255}, 1},
    {"DANA", {200, 90, 230, 255}, {70, 90, 140, 255}, {230, 185, 150, 255}, {40, 25, 20, 255}, 8},
    {"DON", {245, 245, 235, 255}, {245, 245, 235, 255}, {190, 140, 105, 255}, {20, 20, 20, 255}, 6},
    {"AIGERIM", {255, 210, 60, 255}, {30, 30, 40, 255}, {235, 190, 155, 255}, {90, 40, 20, 255}, 8},
    {"RUSTAM", {30, 200, 200, 255}, {40, 40, 55, 255}, {200, 150, 110, 255}, {200, 160, 60, 255}, 2},
    {"ZHANAT", {40, 70, 190, 255}, {30, 40, 90, 255}, {195, 150, 115, 255}, {60, 60, 60, 255}, 1}};
static const Look* lookOf(const std::string& n) { for (auto& l : LOOKS) if (n == l.n) return &l; return nullptr; }

struct Bld { float x, z, w, d, h; Color c; bool neon; int style; };
struct Tree { float x, z, h; int kind; };  // kind 0 palm, 1 street lamp
struct Car {
    Vector3 p{}; float yaw = 0, spd = 0, hp = 100, stun = 0, dead = 0;
    int dir = 0, kind = 0; Color c{}; bool police = false, owned = false, alive = true, active = true;
};
struct Ped {
    Vector3 p{}; float yaw = 0, hp = 40, t = 0, flee = 0, anim = 0, shoot = 0, fall = 0, spd = 1.6f;
    Look lk{}; bool dead = false, gang = false, boss = false, film = false;
};
struct Pick { Vector3 p{}; int type = 0; bool on = true, once = false; float rs = 0; };
struct Tr { Vector3 a, b; float t; };
struct Fx { Vector3 p; float t; };
struct Mission {
    const char* title; int type, ti, tj, ui, uj, count, reward; float hour; int si, sj, limit;
    std::vector<Line> intro, outro; const char* obj;
};
enum { MT_GOTO, MT_STEAL, MT_CRATES, MT_DELIVER, MT_FOLLOW, MT_KILL, MT_EVADE, MT_RACE, MT_SURVIVE, MT_HEIST, MT_BOSS };
struct Player {
    Vector3 p = START; float yaw = 0, vy = 0, hp = 100, ar = 0, drunk = 0, stam = 100, cd = 0, anim = 0, heat = 0, last = 99;
    int money = 250, beers = 2, wpn = 1, car = -1; int ammo[3] = {0, 60, 0};
} pl;

enum St { S_SPLASH, S_TITLE, S_CUT, S_PLAY, S_PAUSE, S_WASTED };
static St st = S_SPLASH;
static float stT = 0, camYaw = 0, camPit = 0.45f, mouseIdle = 0, shake = 0, msgT = 0, radioT = 0, bannerT = 0, wastedT = 0, copT = 0;
static bool phone = false, bossDead = false;
static std::string msg, obj;
static int radio = 0;
static const char* RADIO[] = {"KOKSHE FM", "NEON 86", "KUMYS & BASS", "RADIO OFF"};
static const char* TRK[3][3] = {{"Steppe Sunset - The Neon Yurts", "Night Drive to Astana - Laser Aqsaqal", "Baursak Dreams - Synth Nomad"},
                                {"Cold Lager Love - Disco Kokshe", "Palm Trees & Pink Skies - Miami Steppe", "Turbo Kumys - Electro Aul"},
                                {"Dombra Disco - DJ Tulpar", "Kokshe Funk - The Yurt Boys", "Sagyndym Seni - Bass Batyr"}};

static std::vector<Bld> B;
static std::vector<Tree> TR;
static std::vector<int> cellB[N][N], cellT[N][N];
static std::vector<Car> cars;
static std::vector<Ped> peds;
static std::vector<Pick> picks;
static std::vector<Vector3> kiosks, rcPts;
static std::vector<Tr> trs;
static std::vector<Fx> fxs;
static bool parkC[N][N]; static int gCell[N][N];
static const int TRAFFIC = 140, COPS = 8, FOLLOW = TRAFFIC + COPS;

// mission / cutscene state
static std::vector<Mission> MS;
static int curM = -1, mCount = 0, mStage = 0, mDone = -1; static bool mOn = false, markOn = false;
static float mT = 0, doneT = 0, mTimer = 0, mFailT = 0, mFar = 0, waveT = 0; static Vector3 mark{};
struct Actor { const Look* lk; Vector3 p; float yaw; };
static std::vector<Line> cLines; static std::vector<Actor> cast;
static int cIdx = 0, cNext = 0, cMis = 0; static float cSceneT = 0, cLineT = 0, cOut = -1; static Vector3 cF{};
static std::vector<Actor>* gCastDraw = nullptr; static int gSpeaker = -1;

static int stars() { return (int)ceilf(pl.heat - 0.001f); }
static void crime(float a) { pl.heat = fminf(5.f, pl.heat + a); pl.last = 0; }
static void toast(const std::string& s) { msg = s; msgT = 3.5f; }

// ================================================================ audio: own synth radio + sfx
static AudioStream gStream; static Sound sfx[6];
static double sT = 0; static int sLast = -1;
static float eBass = 0, eLead = 0, eK = 0, eKF = 0, eS = 0, eH = 0, eHd = 55, bassF = 110, leadF = 440, sBassPh = 0, sLeadPh = 0, sKPh = 0, sPad[3] = {0, 0, 0}, sLP = 0, sPrevN = 0;
static float mf(int n) { return 440.f * powf(2.f, (n - 69) / 12.f); }
static const int MINS[7] = {0, 2, 3, 5, 7, 8, 10};
static void synth(short* out, int n, int stn) {
    static const float BPM[4] = {104, 122, 112, 76};
    static const int ROOT[4] = {57, 60, 55, 53};
    static const int PROG[4][4] = {{0, 5, 3, 4}, {0, 3, 4, 0}, {0, 3, 0, 4}, {0, 5, 3, 4}};
    static const int LEAD[4][16] = {{7, -1, 9, -1, 7, -1, 6, -1, 4, -1, 6, -1, 7, -1, -1, -1}, {7, 9, 11, 9, 7, 9, 11, 12, 7, 9, 11, 9, 14, 12, 11, 9},
                                    {0, -1, 2, 4, -1, 2, 0, -1, 4, -1, 5, 4, -1, 2, 0, -1}, {0, -1, -1, -1, 2, -1, -1, -1, 4, -1, -1, -1, 2, -1, -1, -1}};
    const float SR = 22050.f; float bpm = BPM[stn];
    auto nd = [&](int d) { return ROOT[stn] + MINS[d % 7] + 12 * (d / 7); };
    float dB = expf(-(stn == 3 ? 0.8f : 3.2f) / SR), dL = expf(-(stn == 2 ? 4.5f : stn == 3 ? 1.2f : 6.f) / SR), dK = expf(-9.f / SR), dKF = expf(-35.f / SR), dS = expf(-16.f / SR);
    for (int i = 0; i < n; i++) {
        double beat = sT * bpm / 60.0; int step = (int)(beat * 4.0);
        if (step != sLast) {
            sLast = step; int s = step & 15, bar = (step >> 4) & 3, deg = PROG[stn][bar];
            bool bOn = stn == 0 ? (s == 0 || s == 3 || s == 6 || s == 8 || s == 11 || s == 14) : stn == 1 ? (s % 2 == 0) : stn == 2 ? true : (s == 0);
            if (bOn) { eBass = 1; bassF = mf(nd(deg) - 12 + ((stn == 2 && (s & 1)) ? 12 : 0)); }
            int ld = LEAD[stn][s]; if (ld >= 0) { eLead = 1; leadF = mf(nd(ld + ((bar & 1) ? 2 : 0)) + 12); }
            bool kOn = stn == 0 ? (s == 0 || s == 6 || s == 10) : stn == 3 ? (s == 0) : (s % 4 == 0);
            if (kOn) { eK = stn == 3 ? 0.35f : 1; eKF = 1; sKPh = 0; }
            if (stn != 3 && (s == 4 || s == 12)) eS = 1;
            if (stn == 0 && s % 2 == 0) { eH = 1; eHd = 55; }
            else if (stn == 1) { eH = (s % 4 == 2) ? 1.f : 0.6f; eHd = 55; }
            else if (stn == 2) { if (s % 4 == 2) { eH = 1; eHd = 12; } else if (s & 1) { eH = 0.4f; eHd = 55; } }
        }
        int bar = (int)(beat * 4.0 / 16.0) & 3, deg = PROG[stn][bar];
        float x = 0;
        sBassPh += bassF / SR; if (sBassPh >= 1) sBassPh -= 1; x += (2 * sBassPh - 1) * eBass * 0.30f; eBass *= dB;
        sLeadPh += leadF / SR; if (sLeadPh >= 1) sLeadPh -= 1;
        if (stn == 2) x += (sinf(2 * PI * sLeadPh) + 0.5f * sinf(4 * PI * sLeadPh) + 0.25f * sinf(6 * PI * sLeadPh)) * eLead * 0.16f;
        else if (stn == 3) x += sinf(2 * PI * sLeadPh) * eLead * 0.2f;
        else x += (sLeadPh < 0.3f ? 1.f : -1.f) * eLead * 0.11f;
        eLead *= dL;
        float pf[3] = {mf(nd(deg)), mf(nd(deg + 2)), mf(nd(deg + 4))}, pd = 0;
        for (int k = 0; k < 3; k++) { sPad[k] += pf[k] * (1.f + 0.004f * k) / SR; if (sPad[k] >= 1) sPad[k] -= 1; pd += 2 * sPad[k] - 1; }
        sLP += (pd * (stn == 3 ? 0.07f : 0.035f) - sLP) * 0.08f; x += sLP;
        sKPh += (45.f + 120.f * eKF) / SR; x += sinf(2 * PI * sKPh) * eK * 0.75f; eK *= dK; eKF *= dKF;
        x += nz() * eS * 0.35f; eS *= dS;
        float no = nz(); x += (no - sPrevN) * eH * 0.2f; sPrevN = no; eH *= expf(-eHd / SR);
        out[i] = (short)(tanhf(x * 1.3f) * 0.7f * 32767.f); sT += 1.0 / SR;
    }
}
static Sound makeSnd(int k) {
    const int sr = 22050; const float dur[6] = {0.28f, 1.0f, 0.25f, 0.45f, 0.3f, 1.0f};
    int n = (int)(dur[k] * sr); short* d = (short*)MemAlloc(n * sizeof(short)); float lp = 0, ph = 0;
    for (int i = 0; i < n; i++) {
        float t = (float)i / sr, v = 0;
        switch (k) {
        case 0: v = nz() * expf(-t * 22) * 0.9f + sinf(2 * PI * 90 * t) * expf(-t * 30) * 0.5f; break;
        case 1: lp += (nz() - lp) * 0.08f; v = lp * expf(-t * 3.2f) * 2.2f; break;
        case 2: { float f = t < 0.08f ? 880.f : t < 0.16f ? 1175.f : 1568.f; ph += f / sr; v = ((fmodf(ph, 1.f) < 0.5f) ? 1.f : -1.f) * 0.3f * expf(-fmodf(t, 0.08f) * 14); break; }
        case 3: lp += (nz() - lp) * 0.2f; v = lp * sinf(PI * t / dur[3]) * 0.9f * (0.5f + 0.5f * sinf(t * 60)); break;
        case 4: lp += (nz() - lp) * 0.15f; v = lp * expf(-t * 14) * 1.5f + sinf(2 * PI * 70 * t) * expf(-t * 12) * 0.5f; break;
        default: { const float f[6] = {523, 659, 784, 1047, 784, 1047}; ph += f[std::min((int)(t / 0.16f), 5)] / sr; v = sinf(2 * PI * ph) * 0.35f * expf(-fmodf(t, 0.16f) * 5); break; }
        }
        d[i] = (short)(Clamp(v, -1.f, 1.f) * 30000);
    }
    Wave w = {(unsigned)n, (unsigned)sr, 16, 1, d}; Sound s = LoadSoundFromWave(w); UnloadWave(w); return s;
}
static void playSfx(int k, float pitch = 1.f) { SetSoundPitch(sfx[k], pitch); PlaySound(sfx[k]); }
static void musicFeed(int stn, bool on) {
    static short buf[2048];
    int guard = 0;
    while (IsAudioStreamProcessed(gStream) && guard++ < 3) {
        if (on) synth(buf, 2048, stn); else memset(buf, 0, sizeof buf);
        UpdateAudioStream(gStream, buf, 2048);
    }
}

// ================================================================ missions
static void initMissions() {
    MS = {
        {"Welcome to Kokshe Beach", MT_GOTO, 30, 12, 0, 0, 0, 300, 18.5f, 32, 20, 0,
         {{"NARRATOR", "1986. Kokshe Beach. Neon, palms and cold beer. Four thousand kilometres from Kokshetau."},
          {"ARMAN", "Dana called me from this city three days ago. Then the line went dead."},
          {"ARMAN", "Her last words: find Uncle Bolat at the Baursak Motel. So here I am."}},
         {{"BOLAT", "Arman! Salem, aka! You look like a man who walked here from Kokshetau."},
          {"ARMAN", "I drove most of the way, Uncle. Where is Dana?"},
          {"BOLAT", "Don Baursak's men took her. He rules this city with kumys, cash and fear."},
          {"BOLAT", "You need wheels first. Go see Kenzhe at the garage on Palm Avenue."}},
         "Go to the Baursak Motel"},
        {"Wheels", MT_STEAL, 20, 26, 0, 0, 0, 400, 19.5f, 30, 12, 0,
         {{"BOLAT", "Take any car you see. In Kokshe Beach we call it 'borrowing'."},
          {"ARMAN", "Borrowing. Right."},
          {"BOLAT", "Bring it to Kenzhe's garage. Try not to wake the police."}},
         {{"KENZHE", "Zhaksy! Smooth driving, aka. I like a man who borrows with style."},
          {"KENZHE", "The Don's guards accept only one bribe: Kokshe Lager, ice cold."},
          {"ARMAN", "Beer. Of course it's beer."},
          {"KENZHE", "Eight crates are hidden around the city. Go find them."}},
         "Steal any car"},
        {"Beer Run", MT_CRATES, 0, 0, 0, 0, 8, 500, 20.5f, -1, -1, 0,
         {{"ARMAN", "Eight crates of Kokshe Lager. Don't drink them all, Arman."}},
         {{"KENZHE", "Eight crates, not one missing! Rakhmet, aka!"},
          {"KENZHE", "Now the Don's beer truck is parked near the old market. Take it to Hotel Steppe on the beach."}},
         "Collect the beer crates"},
        {"Cold Delivery", MT_DELIVER, 14, 22, 36, 30, 0, 700, 21.f, 20, 26, 170,
         {{"KENZHE", "The beer truck is waiting at the old market. Any car works, aka, but be fast."},
          {"KENZHE", "Hotel Steppe is on the east side. If the beer warms up, the Don's friends get angry."}},
         {{"AIGERIM", "You're the man asking about Dana? I'm Aigerim. I write for the Kokshe Times."},
          {"AIGERIM", "Baursak's courier carries every ransom note to a warehouse. Follow him and you'll find her."},
          {"ARMAN", "Where do I find this courier?"},
          {"AIGERIM", "He's about to leave the hotel. Yellow car. Don't lose him."}},
         "Get a car and go to the old market"},
        {"Tail the Courier", MT_FOLLOW, 0, 0, 0, 0, 70, 800, 22.f, 36, 30, 0,
         {{"AIGERIM", "There he is. Stay close, but not too close."}},
         {{"ARMAN", "He drove straight to the docks. Armed men everywhere."},
          {"AIGERIM", "That's the Kumys Cartel. Dana isn't there, but her phone might be."}},
         "Follow the yellow car"},
        {"Kumys Cartel", MT_KILL, 8, 34, 0, 0, 8, 1000, 23.f, -1, -1, 0,
         {{"ARMAN", "Eight armed men at the docks. I brought a pistol and bad jokes."}},
         {{"ARMAN", "Her phone. A message: 'Winner of tonight's race gets an invitation to the Don's party.'"},
          {"NARRATOR", "Sirens. Somebody called General Zhanat."}},
         "Eliminate the cartel at the docks"},
        {"Hot Pursuit", MT_EVADE, 0, 0, 0, 0, 0, 900, 23.5f, -1, -1, 0,
         {{"ZHANAT", "All units. Suspect in a pink shirt. Shoot first, paperwork later."},
          {"ARMAN", "Great. A general who loves paperwork."}},
         {{"ZHANAT", "Pink shirt escaped. Ha. He won't be so lucky next time."},
          {"ARMAN", "Race night. Let's go meet Rustam."}},
         "Lose the police"},
        {"Street Race", MT_RACE, 20, 20, 0, 0, 8, 1500, 2.f, 20, 20, 130,
         {{"RUSTAM", "So you want the Don's invitation? Then beat the clock, Kazakh."},
          {"RUSTAM", "Eight checkpoints through downtown. No brakes, no mercy."},
          {"ARMAN", "I only brake for kiosks."}},
         {{"RUSTAM", "Respect! Here is the invitation. But the Don knows you are coming."},
          {"ARMAN", "Then let him wait."}},
         "Race through the checkpoints (in a car)"},
        {"Last Stand at the Motel", MT_SURVIVE, 30, 12, 0, 0, 60, 1800, 5.5f, 30, 12, 0,
         {{"BOLAT", "They found us! Take the pistol, Arman. Hold the motel!"},
          {"ARMAN", "How long, Uncle?"},
          {"BOLAT", "Until sunrise. Or until they run out of bullets."}},
         {{"BOLAT", "Rakhmet, balam. You saved my motel and my beer fridge."},
          {"BOLAT", "The Don wants a ransom. Six bags of cash from Casino Steppe downtown."}},
         "Survive the attack"},
        {"Casino Heist", MT_HEIST, 0, 0, 0, 0, 6, 3000, 1.5f, -1, -1, 0,
         {{"ARMAN", "Six bags of cash. A casino. And a pink shirt. What could go wrong?"}},
         {{"ARMAN", "Money collected. Time to pay a visit to the Don."},
          {"DANA", "Arman? I heard shooting. Please hurry, I'm in the mansion on the hills."}},
         "Steal the cash bags in downtown"},
        {"Grand Rescue Protocol", MT_BOSS, 36, 4, 0, 0, 0, 10000, 17.5f, 35, 5, 0,
         {{"ARMAN", "The Don's mansion. White suit, gold chains and a lot of guards."},
          {"DON", "Welcome, Arman. You brought beer? How thoughtful."},
          {"ARMAN", "Run the Grand Rescue Protocol."}},
         {{"DANA", "Rakhmet, aka. I knew you would come."},
          {"ARMAN", "Let's get out of here. I know a beer kiosk with a sunset view."},
          {"NARRATOR", "THE END. Kokshe Beach is yours. Thanks for playing! (Free roam unlocked)"}},
         "Defeat Don Baursak"}};
}

// ================================================================ world
static bool resolve(Vector3& p, float r) {
    bool hit = false; int ci = (int)(p.x / P), cj = (int)(p.z / P);
    for (int di = -1; di <= 1; di++) for (int dj = -1; dj <= 1; dj++) {
        int i = ci + di, j = cj + dj; if (i < 0 || j < 0 || i >= N || j >= N) continue;
        for (int idx : cellB[i][j]) {
            Bld& b = B[idx];
            if (fabsf(b.x - p.x) > b.w / 2 + r + 1 || fabsf(b.z - p.z) > b.d / 2 + r + 1) continue;
            float cx = Clamp(p.x, b.x - b.w / 2, b.x + b.w / 2), cz = Clamp(p.z, b.z - b.d / 2, b.z + b.d / 2);
            float dx = p.x - cx, dz = p.z - cz, q = dx * dx + dz * dz;
            if (q < r * r) {
                if (q > 1e-6f) { float d = sqrtf(q); p.x = cx + dx / d * r; p.z = cz + dz / d * r; } else p.x += r;
                hit = true;
            }
        }
    }
    return hit;
}
static Vector3 cpos(const Car& c) {
    if (c.owned) return c.p;
    return {c.p.x - cosf(c.yaw) * 2.6f, 0, c.p.z + sinf(c.yaw) * 2.6f};
}
static float maxSpeed(const Car& c) { return c.police ? 44.f : c.kind == 1 ? 52.f : c.kind == 2 ? 32.f : 40.f; }
static void placeOnRoad(Car& c, Vector3 nr, float minD, float maxD) {
    Vector3 p{}; int dir = 0;
    for (int t = 0; t < 40; t++) {
        dir = GetRandomValue(0, 3); int k = GetRandomValue(0, N); float a = rf(0, WORLD);
        p = (dir & 1) ? Vector3{a, 0, k * P} : Vector3{k * P, 0, a};
        float d = d2(p, nr); if (d >= minD && d <= maxD) break;
    }
    c.p = p; c.dir = dir; c.yaw = DY[dir]; c.spd = 0; c.hp = 100; c.alive = true; c.dead = 0; c.stun = 0; c.owned = false;
}
static int district(int i, int j) {
    if (i >= N - 6) return 1;
    if (i >= N - 15 && j < 14) return 4;
    if (j >= N - 10 && i < N / 2) return 2;
    if (j < 14 && i < 17) return 3;
    if (hypotf(i - N / 2.f, j - N / 2.f) < 8) return 0;
    return 5;
}
static Look randLook(int dsc) {
    static const Color skins[4] = {{235, 195, 160, 255}, {220, 170, 130, 255}, {190, 140, 100, 255}, {140, 95, 65, 255}};
    static const Color hairs[6] = {{30, 25, 25, 255}, {60, 40, 25, 255}, {120, 80, 40, 255}, {200, 170, 90, 255}, {150, 150, 150, 255}, {20, 20, 20, 255}};
    static const Color shirts[8] = {{255, 105, 180, 255}, {0, 200, 210, 255}, {170, 100, 230, 255}, {255, 220, 90, 255}, {240, 240, 235, 255}, {255, 150, 80, 255}, {120, 220, 160, 255}, {255, 110, 110, 255}};
    static const Color pants[4] = {{70, 90, 140, 255}, {200, 180, 140, 255}, {40, 40, 50, 255}, {235, 235, 235, 255}};
    Look l{}; l.n = nullptr; l.skin = skins[GetRandomValue(0, 3)]; l.hair = hairs[GetRandomValue(0, 5)];
    l.shirt = shirts[GetRandomValue(0, 7)]; l.pants = pants[GetRandomValue(0, 3)]; l.acc = 0;
    if (dsc == 1) { l.pants = {235, 235, 235, 255}; if (GetRandomValue(0, 2) == 0) l.acc |= 2; }
    if (dsc == 0 && GetRandomValue(0, 1)) { l.shirt = GetRandomValue(0, 1) ? Color{235, 235, 240, 255} : Color{60, 60, 70, 255}; l.pants = {35, 35, 45, 255}; if (GetRandomValue(0, 4) == 0) l.acc |= 4; }
    if (GetRandomValue(0, 5) == 0) l.acc |= 1;
    if (GetRandomValue(0, 6) == 0) l.acc |= 2;
    if (GetRandomValue(0, 3) == 0) l.acc |= 8;
    return l;
}
static void spawnGang(Vector3 c, int n, bool boss, float spread = 8) {
    for (int i = 0; i < n; i++) {
        Ped q; q.p = {c.x + rf(-spread, spread), 0, c.z + rf(-spread, spread)}; q.gang = true; q.hp = 60; q.shoot = rf(0.5f, 2.f);
        q.lk = randLook(5); q.lk.shirt = {190, 30, 50, 255}; q.lk.pants = {40, 30, 30, 255}; q.lk.acc = 4 | (GetRandomValue(0, 1) ? 2 : 0);
        peds.push_back(q);
    }
    if (boss) { Ped q; q.p = c; q.gang = true; q.boss = true; q.hp = 400; q.lk = *lookOf("DON"); peds.push_back(q); }
}
static void genWorld() {
    SetRandomSeed(1337);
    Color pal[8] = {{255, 105, 180, 255}, {0, 230, 230, 255}, {170, 100, 230, 255}, {255, 220, 90, 255}, {245, 245, 235, 255}, {255, 150, 80, 255}, {120, 230, 170, 255}, {255, 120, 120, 255}};
    static const int gtex[6] = {0, 1, 2, 3, 3, 0};
    for (int i = 0; i < N; i++) for (int j = 0; j < N; j++) {
        int dsc = district(i, j);
        float x0 = i * P + RHW + 1.5f, z0 = j * P + RHW + 1.5f, s = P - 2 * RHW - 3, cx = x0 + s / 2, cz = z0 + s / 2;
        float r = rf(0, 1); parkC[i][j] = false;
        auto mk = [&](float x, float z, float w, float d, float hmin, float hmax, int style, float neonP, bool pastel = true) {
            Bld b; b.x = x; b.z = z; b.w = w; b.d = d; b.h = hmin + rf(0, hmax - hmin); b.style = style;
            Color c = pal[GetRandomValue(0, 7)]; b.c = pastel ? mixC(c, WHITE, 0.25f) : mixC(c, {150, 150, 160, 255}, 0.7f);
            b.neon = rf(0, 1) < neonP; cellB[i][j].push_back((int)B.size()); B.push_back(b);
        };
        auto tree = [&](float x, float z, float h, int kind) { cellT[i][j].push_back((int)TR.size()); TR.push_back({x, z, h, kind}); };
        if (i == 36 && j == 4) { mk(cx, cz, s - 2, s - 2, 18, 18, 3, 1.f); for (int t = 0; t < 8; t++) tree(x0 + rf(1, s - 1), z0 + rf(1, s - 1), 6.f, 0); }
        else if (i == 20 && j == 20) mk(cx, cz, 20, 20, 150, 150, 2, 1.f);
        else switch (dsc) {
        case 0:
            if (r < 0.05f) { parkC[i][j] = true; for (int t = 0; t < 6; t++) tree(x0 + rf(2, s - 2), z0 + rf(2, s - 2), rf(5, 8), 0); }
            else if (r < 0.6f) mk(cx, cz, s - rf(0, 5), s - rf(0, 5), 25, 95, GetRandomValue(0, 3), 0.35f);
            else for (int a = 0; a < 2; a++) for (int b2 = 0; b2 < 2; b2++) mk(x0 + s / 4 + a * s / 2, z0 + s / 4 + b2 * s / 2, s / 2 - 1.5f, s / 2 - 1.5f, 15, 45, GetRandomValue(0, 3), 0.3f);
            break;
        case 1:
            for (int a = 0; a < 2; a++) for (int b2 = 0; b2 < 2; b2++) mk(x0 + s / 4 + a * s / 2, z0 + s / 4 + b2 * s / 2, s / 2 - 2.5f, s / 2 - 2.5f, 8, 20, GetRandomValue(0, 1) ? 3 : 0, 0.5f);
            for (int t = 0; t < 4; t++) tree(x0 + rf(0, s), z0 + s / 2 + rf(-1, 1), rf(6, 8), 0);
            break;
        case 2:
            if (r < 0.7f) mk(cx, cz, s - 2, s - 6, 7, 11, 5, 0.f, false);
            else { mk(cx, z0 + s / 4, s - 2, s / 2 - 2, 7, 10, 5, 0.f, false); mk(cx, z0 + 3 * s / 4, s - 2, s / 2 - 2, 7, 10, 5, 0.f, false); }
            break;
        case 3:
            if (r < 0.12f) { parkC[i][j] = true; for (int t = 0; t < 6; t++) tree(x0 + rf(2, s - 2), z0 + rf(2, s - 2), rf(5, 8), 0); }
            else for (int a = 0; a < 3; a++) for (int b2 = 0; b2 < 3; b2++) { mk(x0 + s / 6 + a * s / 3, z0 + s / 6 + b2 * s / 3, 6.5f, 6.5f, 5, 7, GetRandomValue(0, 1) ? 4 : 3, 0.f); if (GetRandomValue(0, 2) == 0) tree(x0 + s / 6 + a * s / 3 + 4, z0 + s / 6 + b2 * s / 3 + 4, 5.f, 0); }
            break;
        case 4:
            for (int a = 0; a < 2; a++) for (int b2 = 0; b2 < 2; b2++) mk(x0 + s / 4 + a * s / 2, z0 + s / 4 + b2 * s / 2, 9, 9, 6, 9, 3, 0.15f);
            for (int t = 0; t < 6; t++) tree(x0 + rf(1, s - 1), z0 + rf(1, s - 1), rf(5, 8), 0);
            break;
        default:
            if (r < 0.1f) { parkC[i][j] = true; for (int t = 0; t < 6; t++) tree(x0 + rf(2, s - 2), z0 + rf(2, s - 2), rf(5, 8), 0); }
            else if (r < 0.45f) mk(cx, cz, s - rf(0, 6), s - rf(0, 6), 10, 30, GetRandomValue(0, 4), 0.25f);
            else for (int a = 0; a < 2; a++) for (int b2 = 0; b2 < 2; b2++) mk(x0 + s / 4 + a * s / 2, z0 + s / 4 + b2 * s / 2, s / 2 - 1.5f, s / 2 - 1.5f, 8, 22, GetRandomValue(0, 4), 0.25f);
        }
        gCell[i][j] = parkC[i][j] ? 3 : gtex[dsc];
        if ((i + j) % 2 == 0) tree(i * P + RHW + 0.8f, j * P + RHW + 0.8f, 6.f, 0);
        else tree(i * P + RHW + 0.6f, j * P + RHW + 0.6f, 7.f, 1);
    }
    for (int i = 0; i < 40; i++) kiosks.push_back({(float)GetRandomValue(1, N - 1) * P + RHW + 2.f, 0.f, (float)GetRandomValue(1, N - 1) * P + RHW + 2.f});
    auto side = [&]() {
        float k = (float)GetRandomValue(1, N - 1) * P, o = (GetRandomValue(0, 1) ? 1 : -1) * (RHW - 1.5f), a = rf(5, WORLD - 5);
        return GetRandomValue(0, 1) ? Vector3{k + o, 0, a} : Vector3{a, 0, k + o};
    };
    for (int i = 0; i < 200; i++) { Pick p; p.p = side(); p.type = 0; picks.push_back(p); }
    for (int i = 0; i < 90; i++) { Pick p; p.p = side(); p.type = 1; picks.push_back(p); }
    for (int i = 0; i < 25; i++) { Pick p; p.p = side(); p.type = 2; picks.push_back(p); }
    for (int i = 0; i < 25; i++) { Pick p; p.p = side(); p.type = 3; picks.push_back(p); }
    for (int i = 0; i < 520; i++) {
        Ped q; q.p = side(); q.yaw = rf(-PI, PI); q.t = rf(1, 5); q.hp = 40;
        int ci = (int)Clamp((float)(int)(q.p.x / P), 0.f, (float)N - 1), cj = (int)Clamp((float)(int)(q.p.z / P), 0.f, (float)N - 1);
        q.lk = randLook(district(ci, cj)); if (GetRandomValue(0, 4) == 0) q.spd = 3.6f;
        peds.push_back(q);
    }
    for (int i = 0; i < TRAFFIC + COPS + 1; i++) {
        Car c; c.police = (i >= TRAFFIC && i < FOLLOW); c.active = (i < TRAFFIC); c.c = pal[GetRandomValue(0, 7)];
        int kr = GetRandomValue(0, 9); c.kind = kr < 6 ? 0 : kr < 8 ? 1 : 2; if (i == FOLLOW) { c.kind = 1; c.c = {255, 230, 50, 255}; }
        placeOnRoad(c, pl.p, 0, 1e9f); cars.push_back(c);
    }
}

// ================================================================ drawing
static void boxR(Vector3 c, float yaw, Vector3 o, Vector3 s, Color col) {
    rlPushMatrix(); rlTranslatef(c.x, c.y, c.z); rlRotatef(yaw * RAD2DEG, 0, 1, 0);
    DrawCube(o, s.x, s.y, s.z, col); rlPopMatrix();
}
static void person(const Look& l, Vector3 p, float yaw, float anim, float dist, bool down, bool talk) {
    Color s = F(l.shirt, dist), pc = F(l.pants, dist), sk = F(l.skin, dist), hr = F(l.hair, dist), dk = F({20, 20, 25, 255}, dist);
    if (down) { boxR(p, yaw, {0, 0.3f, 0}, {0.9f, 0.5f, 1.8f}, s); return; }
    float sw = sinf(anim) * 0.35f, ar = talk ? sinf(gT * 6) * 0.4f : 0.f, hb = talk ? fabsf(sinf(gT * 8)) * 0.04f : 0.f;
    boxR(p, yaw, {-0.22f, 0.45f, sw}, {0.38f, 0.9f, 0.4f}, pc);
    boxR(p, yaw, {0.22f, 0.45f, -sw}, {0.38f, 0.9f, 0.4f}, pc);
    boxR(p, yaw, {0, 1.35f, 0}, {0.9f, 0.9f, 0.5f}, s);
    boxR(p, yaw, {-0.6f, 1.3f + ar * 0.3f, -sw + ar}, {0.25f, 0.8f, 0.3f}, s);
    boxR(p, yaw, {0.6f, 1.3f, sw}, {0.25f, 0.8f, 0.3f}, s);
    DrawSphere({p.x, p.y + 2.0f + hb, p.z}, 0.3f, sk);
    boxR(p, yaw, {0, 2.27f + hb, -0.02f}, {0.52f, 0.14f, 0.52f}, hr);
    boxR(p, yaw, {-0.1f, 2.04f + hb, 0.27f}, {0.07f, 0.07f, 0.04f}, dk);
    boxR(p, yaw, {0.1f, 2.04f + hb, 0.27f}, {0.07f, 0.07f, 0.04f}, dk);
    if (l.acc & 1) { boxR(p, yaw, {0, 2.36f + hb, 0}, {0.56f, 0.12f, 0.56f}, s); boxR(p, yaw, {0, 2.3f + hb, 0.36f}, {0.4f, 0.04f, 0.25f}, s); }
    if (l.acc & 2) boxR(p, yaw, {0, 2.04f + hb, 0.28f}, {0.5f, 0.1f, 0.06f}, dk);
    if (l.acc & 4) boxR(p, yaw, {0, 1.72f, 0.27f}, {0.5f, 0.08f, 0.05f}, Fe({255, 200, 40, 255}, dist));
    if (l.acc & 8) boxR(p, yaw, {0, 1.85f + hb, -0.27f}, {0.5f, 0.75f, 0.14f}, hr);
}
static void drawCar(const Car& c, float dist) {
    Vector3 p = cpos(c);
    if (!c.alive) { boxR(p, c.yaw, {0, 0.6f, 0}, {2.2f, 1.0f, 4.4f}, {20, 20, 20, 255}); return; }
    Color body = c.police ? Color{40, 70, 190, 255} : c.c, cab = c.police ? Color{240, 240, 250, 255} : mixC(c.c, BLACK, 0.4f), glass = F({30, 40, 70, 255}, dist);
    int k = c.police ? 0 : c.kind;
    if (k == 0) { boxR(p, c.yaw, {0, 0.75f, 0}, {2.2f, 0.8f, 4.4f}, F(body, dist)); boxR(p, c.yaw, {0, 1.4f, -0.2f}, {1.8f, 0.65f, 2.2f}, F(cab, dist)); boxR(p, c.yaw, {0, 1.4f, 0.95f}, {1.6f, 0.5f, 0.1f}, glass); }
    else if (k == 1) { boxR(p, c.yaw, {0, 0.6f, 0}, {2.2f, 0.6f, 4.5f}, F(body, dist)); boxR(p, c.yaw, {0, 1.1f, -0.4f}, {1.7f, 0.5f, 1.7f}, F(cab, dist)); boxR(p, c.yaw, {0, 1.12f, 0.45f}, {1.5f, 0.4f, 0.1f}, glass); boxR(p, c.yaw, {0, 1.05f, -2.0f}, {2.0f, 0.1f, 0.5f}, F(cab, dist)); }
    else { boxR(p, c.yaw, {0, 1.25f, 0}, {2.3f, 1.9f, 4.8f}, F(body, dist)); boxR(p, c.yaw, {0, 1.7f, 2.0f}, {2.0f, 0.7f, 0.1f}, glass); boxR(p, c.yaw, {0, 0.7f, 0}, {2.35f, 0.2f, 4.85f}, F(cab, dist)); }
    Color w = F({15, 15, 15, 255}, dist);
    for (int sx = -1; sx <= 1; sx += 2) for (int sz = -1; sz <= 1; sz += 2) boxR(p, c.yaw, {sx * 1.1f, 0.35f, sz * 1.4f}, {0.3f, 0.7f, 0.7f}, w);
    float fz = k == 2 ? 2.4f : 2.2f;
    boxR(p, c.yaw, {-0.7f, 0.8f, fz}, {0.4f, 0.25f, 0.1f}, Fe({255, 240, 160, 255}, dist)); boxR(p, c.yaw, {0.7f, 0.8f, fz}, {0.4f, 0.25f, 0.1f}, Fe({255, 240, 160, 255}, dist));
    boxR(p, c.yaw, {-0.7f, 0.8f, -fz}, {0.4f, 0.25f, 0.1f}, Fe({230, 30, 30, 255}, dist)); boxR(p, c.yaw, {0.7f, 0.8f, -fz}, {0.4f, 0.25f, 0.1f}, Fe({230, 30, 30, 255}, dist));
    if (c.police) {
        bool f = fmodf(gT * 4, 1.f) < .5f;
        boxR(p, c.yaw, {-0.5f, 1.85f, -0.2f}, {0.5f, 0.2f, 0.5f}, Fe(f ? Color{255, 0, 0, 255} : Color{60, 0, 0, 255}, dist));
        boxR(p, c.yaw, {0.5f, 1.85f, -0.2f}, {0.5f, 0.2f, 0.5f}, Fe(f ? Color{0, 0, 60, 255} : Color{0, 120, 255, 255}, dist));
    }
}
static void drawStar2D(float cx, float cy, float R, float r, Color c) {
    Vector2 pt[10];
    for (int i = 0; i < 10; i++) { float a = -PI / 2 + i * PI / 5, rr = (i & 1) ? r : R; pt[i] = {cx + cosf(a) * rr, cy + sinf(a) * rr}; }
    for (int i = 0; i < 10; i++) { Vector2 a = pt[i], b = pt[(i + 1) % 10]; DrawTriangle({cx, cy}, a, b, c); DrawTriangle({cx, cy}, b, a, c); }
}

// textured quads
static void flatQ(float x0, float z0, float x1, float z1, float y, float tile, Color c, float uo = 0, float vo = 0) {
    rlColor4ub(c.r, c.g, c.b, 255);
    rlTexCoord2f(x0 / tile + uo, z1 / tile + vo); rlVertex3f(x0, y, z1);
    rlTexCoord2f(x1 / tile + uo, z1 / tile + vo); rlVertex3f(x1, y, z1);
    rlTexCoord2f(x1 / tile + uo, z0 / tile + vo); rlVertex3f(x1, y, z0);
    rlTexCoord2f(x0 / tile + uo, z0 / tile + vo); rlVertex3f(x0, y, z0);
}
static void sideQ(Vector3 a, Vector3 b, Vector3 c, Vector3 d, float u, float vb, float vt) {
    rlTexCoord2f(0, vb); rlVertex3f(a.x, a.y, a.z); rlTexCoord2f(u, vb); rlVertex3f(b.x, b.y, b.z);
    rlTexCoord2f(u, vt); rlVertex3f(c.x, c.y, c.z); rlTexCoord2f(0, vt); rlVertex3f(d.x, d.y, d.z);
}
static void drawBld(const Bld& b, float d, bool night) {
    float hx = b.w / 2, hz = b.d / 2, h = b.h, tile = 8.f, vb = ceilf(h / tile), vt = vb - h / tile, uw = b.w / tile, ud = b.d / tile;
    Color tc = tintC(b.c, d, night);
    rlSetTexture(tFac[b.style][night ? 1 : 0].id); rlBegin(RL_QUADS); rlColor4ub(tc.r, tc.g, tc.b, 255);
    sideQ({b.x - hx, 0, b.z + hz}, {b.x + hx, 0, b.z + hz}, {b.x + hx, h, b.z + hz}, {b.x - hx, h, b.z + hz}, uw, vb, vt);
    sideQ({b.x + hx, 0, b.z - hz}, {b.x - hx, 0, b.z - hz}, {b.x - hx, h, b.z - hz}, {b.x + hx, h, b.z - hz}, uw, vb, vt);
    sideQ({b.x + hx, 0, b.z + hz}, {b.x + hx, 0, b.z - hz}, {b.x + hx, h, b.z - hz}, {b.x + hx, h, b.z + hz}, ud, vb, vt);
    sideQ({b.x - hx, 0, b.z - hz}, {b.x - hx, 0, b.z + hz}, {b.x - hx, h, b.z + hz}, {b.x - hx, h, b.z - hz}, ud, vb, vt);
    rlEnd(); rlSetTexture(tRoof.id); rlBegin(RL_QUADS);
    Color rc = tintC(mixC(b.c, BLACK, 0.4f), d, night); rlColor4ub(rc.r, rc.g, rc.b, 255);
    rlTexCoord2f(0, ud); rlVertex3f(b.x - hx, h, b.z + hz); rlTexCoord2f(uw, ud); rlVertex3f(b.x + hx, h, b.z + hz);
    rlTexCoord2f(uw, 0); rlVertex3f(b.x + hx, h, b.z - hz); rlTexCoord2f(0, 0); rlVertex3f(b.x - hx, h, b.z - hz);
    rlEnd(); rlSetTexture(0);
    if (b.neon) DrawCube({b.x, h * 0.92f, b.z}, b.w + 0.3f, 0.5f, b.d + 0.3f, Fe(mixC(b.c, {255, 40, 200, 255}, .6f), d));
}

static void drawWorld(Vector3 v) {
    int ci = (int)floorf(v.x / P), cj = (int)floorf(v.z / P);
    Texture2D gt[4] = {tSide, tSand, tConc, tGrass};
    for (int g = 0; g < 4; g++) {
        rlSetTexture(gt[g].id); rlBegin(RL_QUADS);
        for (int i = std::max(0, ci - 6); i <= std::min(N - 1, ci + 6); i++) for (int j = std::max(0, cj - 6); j <= std::min(N - 1, cj + 6); j++) {
            if (gCell[i][j] != g) continue;
            float d = d2({i * P + P / 2, 0, j * P + P / 2}, v);
            flatQ(i * P + RHW, j * P + RHW, (i + 1) * P - RHW, (j + 1) * P - RHW, 0, 8, F(WHITE, d));
        }
        rlEnd();
    }
    rlSetTexture(tAsph.id); rlBegin(RL_QUADS);
    for (int k = std::max(0, ci - 6); k <= std::min(N, ci + 7); k++) for (int s = std::max(0, cj - 6); s <= std::min(N - 1, cj + 6); s++) {
        float d = d2({k * P, 0, s * P + P / 2}, v); flatQ(k * P - RHW, s * P, k * P + RHW, (s + 1) * P, 0.01f, 8, F(WHITE, d));
    }
    for (int k = std::max(0, cj - 6); k <= std::min(N, cj + 7); k++) for (int s = std::max(0, ci - 6); s <= std::min(N - 1, ci + 6); s++) {
        float d = d2({s * P + P / 2, 0, k * P}, v); flatQ(s * P + RHW, k * P - RHW, (s + 1) * P - RHW, k * P + RHW, 0.01f, 8, F(WHITE, d));
    }
    rlEnd(); rlSetTexture(0); rlBegin(RL_QUADS);
    for (int k = std::max(0, ci - 6); k <= std::min(N, ci + 7); k++) for (int s = std::max(0, cj - 6); s <= std::min(N - 1, cj + 6); s++) {
        float d = d2({k * P, 0, s * P + P / 2}, v); Color dc = F({250, 220, 80, 255}, d);
        for (int q = 0; q < 3; q++) flatQ(k * P - 0.15f, s * P + 6.f + q * 16.f - 3, k * P + 0.15f, s * P + 6.f + q * 16.f + 3, 0.03f, 1000, dc);
    }
    for (int k = std::max(0, cj - 6); k <= std::min(N, cj + 7); k++) for (int s = std::max(0, ci - 6); s <= std::min(N - 1, ci + 6); s++) {
        float d = d2({s * P + P / 2, 0, k * P}, v); Color dc = F({250, 220, 80, 255}, d);
        for (int q = 0; q < 3; q++) flatQ(s * P + 6.f + q * 16.f - 3, k * P - 0.15f, s * P + 6.f + q * 16.f + 3, k * P + 0.15f, 0.03f, 1000, dc);
    }
    rlEnd();
    // ocean
    rlSetTexture(tWater.id); rlBegin(RL_QUADS);
    float ws = gT * 0.02f, e = 1000;
    flatQ(WORLD + 15, -e, WORLD + e, WORLD + e, -0.6f, 14, F(WHITE, fmaxf(1, WORLD + 15 - v.x)), ws, ws);
    flatQ(-e, -e, -15, WORLD + e, -0.6f, 14, F(WHITE, fmaxf(1, v.x + 15)), ws, ws);
    flatQ(-15, WORLD + 15, WORLD + 15, WORLD + e, -0.6f, 14, F(WHITE, fmaxf(1, WORLD + 15 - v.z)), ws, ws);
    flatQ(-15, -e, WORLD + 15, -15, -0.6f, 14, F(WHITE, fmaxf(1, v.z + 15)), ws, ws);
    rlEnd(); rlSetTexture(0);
    bool night = gL < 0.7f;
    for (int i = std::max(0, ci - 6); i <= std::min(N - 1, ci + 6); i++) for (int j = std::max(0, cj - 6); j <= std::min(N - 1, cj + 6); j++) {
        for (int id : cellB[i][j]) { Bld& b = B[id]; float d = sqrtf((b.x - v.x) * (b.x - v.x) + (b.z - v.z) * (b.z - v.z)); if (d < 250) drawBld(b, d, night); }
        for (int id : cellT[i][j]) {
            Tree& t = TR[id]; float d = d2({t.x, 0, t.z}, v); if (d > 160) continue;
            if (t.kind == 1) { DrawCylinder({t.x, 0, t.z}, 0.12f, 0.15f, 6.5f, 5, F({90, 90, 100, 255}, d)); DrawSphere({t.x, 6.7f, t.z}, 0.4f, night ? Fe({255, 230, 150, 255}, d) : F({200, 200, 200, 255}, d)); continue; }
            DrawCylinder({t.x, 0, t.z}, 0.25f, 0.4f, t.h, 6, F({110, 75, 45, 255}, d));
            DrawCylinder({t.x, t.h - 0.5f, t.z}, 0.f, 2.6f, 1.2f, 7, F({40, 150, 70, 255}, d));
            DrawCylinder({t.x, t.h + 0.3f, t.z}, 0.f, 1.8f, 1.1f, 7, F({60, 180, 90, 255}, d));
        }
    }
    for (auto& k : kiosks) {
        float d = d2(k, v); if (d > 160) continue;
        DrawCube({k.x, 1.1f, k.z}, 1.4f, 2.2f, 1.4f, F({255, 90, 160, 255}, d));
        DrawCube({k.x, 2.6f, k.z}, 1.8f, 0.5f, 0.3f, Fe({0, 255, 240, 255}, d));
        DrawSphere({k.x, 3.2f, k.z}, 0.35f, Fe({255, 200, 40, 255}, d));
    }
    bool bags = mOn && curM >= 0 && MS[curM].type == MT_HEIST;
    for (auto& p : picks) {
        if (!p.on) continue; float d = d2(p.p, v); if (d > 120) continue;
        float y = 1.0f + sinf(gT * 3 + p.p.x) * 0.2f; Color c; Vector3 s = {0.5f, 0.8f, 0.5f};
        switch (p.type) {
        case 0: c = {255, 190, 30, 255}; break;
        case 1: c = {80, 220, 90, 255}; s = {0.9f, 0.1f, 0.5f}; break;
        case 2: c = {70, 130, 255, 255}; break;
        case 3: c = {150, 150, 160, 255}; s = {0.8f, 0.3f, 0.3f}; break;
        default: c = bags ? Color{60, 210, 90, 255} : Color{160, 100, 50, 255}; s = {1.3f, 1.1f, 1.3f}; }
        boxR({p.p.x, y, p.p.z}, gT * 2, {0, 0, 0}, s, Fe(c, d));
        if (p.type == 4) DrawCylinder({p.p.x, 0, p.p.z}, 0.4f, 0.4f, 40, 8, ColorAlpha(YELLOW, 0.35f));
    }
    if (markOn) {
        DrawCylinder({mark.x, 0, mark.z}, 2.5f, 2.5f, 60, 16, ColorAlpha({255, 230, 40, 255}, 0.28f));
        DrawCylinder({mark.x, 0, mark.z}, 5.f, 5.f, 0.3f, 24, ColorAlpha({255, 60, 200, 255}, 0.5f));
    }
}

static void renderScene(Camera3D cam, Vector3 view, bool showPlayer) {
    Vector3 dr = Vector3Normalize(Vector3Subtract(cam.target, cam.position));
    float a = asinf(dr.y), yv = atan2f(dr.x, dr.z), focal = (RH / 2.f) / tanf(cam.fovy * 0.5f * DEG2RAD);
    int hy = (int)(RH / 2.f + tanf(a) * focal);
    if (hy > 0) DrawRectangleGradientV(0, 0, RW, std::min(hy, RH), gTop, gHor);
    if (gHour > 20 || gHour < 5) for (int i = 0; i < 70; i++) {
        float sx = fmodf(i * 131.7f - yv * 300.f + 100000.f, (float)RW), sy = fmodf(i * 37.3f, (float)std::max(hy, 1));
        DrawPixel((int)sx, (int)sy, WHITE);
    }
    if (gHour > 6 && gHour < 19) {
        float dy = wrapA(1.2f - yv), e = sinf((gHour - 6) / 13.f * PI);
        if (fabsf(dy) < 1.1f) {
            float sx = RW / 2.f - tanf(dy) * focal, sy = hy - e * 150.f + 20, r = 38;
            DrawCircle((int)sx, (int)sy, r, {255, 200, 70, 255});
            DrawCircleGradient((int)sx, (int)sy, r * 2.6f, ColorAlpha({255, 140, 120, 255}, 0.25f), ColorAlpha({255, 140, 120, 255}, 0.f));
            if (e < 0.4f) for (int k = 0; k < 6; k++) DrawRectangle((int)(sx - r), (int)(sy + r * 0.1f + k * 9), (int)(2 * r), 2 + k, gHor);
        }
    }
    int h0 = std::max(hy, 0); if (h0 < RH) DrawRectangle(0, h0, RW, RH - h0, gHor);
    BeginMode3D(cam);
    drawWorld(view);
    for (auto& c : cars) { if (!c.active) continue; float d = d2(c.p, view); if (d < 150) drawCar(c, d); }
    for (auto& q : peds) {
        float d = d2(q.p, view); if (d > 130) continue;
        person(q.lk, q.p, q.yaw, q.anim, d, q.dead, false);
        if (q.film && !q.dead) boxR(q.p, q.yaw, {0.3f, 1.9f, 0.5f}, {0.25f, 0.4f, 0.05f}, Fe({0, 255, 255, 255}, d));
    }
    if (showPlayer && pl.car < 0) {
        person(LOOKS[0], pl.p, pl.yaw, pl.anim, 0, false, false);
        if (pl.wpn > 0) boxR(pl.p, pl.yaw, {0.45f, 1.3f, 0.6f}, {0.15f, 0.2f, 0.6f}, {40, 40, 40, 255});
    }
    if (gCastDraw) for (size_t i = 0; i < gCastDraw->size(); i++) { Actor& ac = (*gCastDraw)[i]; if (ac.lk) person(*ac.lk, ac.p, ac.yaw, 0, 0, false, (int)i == gSpeaker); }
    for (auto& t : trs) DrawLine3D(t.a, t.b, YELLOW);
    for (auto& f : fxs) if (f.t < 1) DrawSphere(f.p, 2 + f.t * 5, ColorAlpha(ORANGE, 1 - f.t));
    EndMode3D();
}

// ================================================================ 2D helpers
static void T(const char* s, int x, int y, int sz, Color c, bool center = false) {
    Vector2 m = MeasureTextEx(GetFontDefault(), s, (float)sz, sz / 10.f);
    float px = center ? x - m.x / 2 : (float)x;
    DrawTextEx(GetFontDefault(), s, {px + 1.5f, (float)y + 1.5f}, (float)sz, sz / 10.f, ColorAlpha(BLACK, 0.8f));
    DrawTextEx(GetFontDefault(), s, {px, (float)y}, (float)sz, sz / 10.f, c);
}
static void TW(const std::string& s, int x, int y, int sz, Color c, int maxc) {
    std::string r = s; int yy = y;
    while (!r.empty()) {
        size_t n = std::min((size_t)maxc, r.size());
        if (n < r.size()) { size_t sp = r.rfind(' ', n); if (sp != std::string::npos && sp > 0) n = sp; }
        T(r.substr(0, n).c_str(), x, yy, sz, c); yy += sz + 4;
        r = r.substr(std::min(r.size(), n + (n < r.size() && r[n] == ' ' ? 1 : 0)));
    }
}
static void logoKZ(int cx, int cy, float sc, float al) {
    int s = (int)(130 * sc);
    DrawRectangleRounded({(float)(cx - s / 2), (float)(cy - s / 2), (float)s, (float)s}, 0.12f, 8, ColorAlpha({255, 140, 0, 255}, al));
    drawStar2D((float)cx, (float)cy - s * 0.08f, s * 0.36f, s * 0.15f, ColorAlpha(BLACK, al));
    T("KZ", cx, cy + (int)(s * 0.22f), (int)(34 * sc), ColorAlpha(WHITE, al), true);
}
static void portrait(const Look* l, int x, int y, int s) {
    DrawRectangle(x - 2, y - 2, s + 4, s + 4, {255, 90, 190, 255}); DrawRectangle(x, y, s, s, {30, 20, 50, 255});
    if (!l) { T("?", x + s / 2, y + s / 4, s / 2, WHITE, true); return; }
    int fx = x + s / 6, fw = s * 2 / 3;
    DrawRectangle(x + 4, y + s - s / 6, s - 8, s / 6, l->shirt);
    DrawRectangle(fx, y + s / 5, fw, s * 3 / 5, l->skin);
    DrawRectangle(fx - 2, y + s / 6, fw + 4, s / 6, l->hair);
    if (l->acc & 8) { DrawRectangle(fx - 4, y + s / 5, 5, s * 2 / 3, l->hair); DrawRectangle(fx + fw - 1, y + s / 5, 5, s * 2 / 3, l->hair); }
    if (l->acc & 1) DrawRectangle(fx - 3, y + s / 8, fw + 6, s / 9, l->shirt);
    if (l->acc & 2) DrawRectangle(fx + 2, y + (int)(s * 0.42f), fw - 4, s / 9, BLACK);
    else { DrawRectangle(fx + fw / 4, y + (int)(s * 0.44f), 4, 4, BLACK); DrawRectangle(fx + fw * 3 / 4 - 4, y + (int)(s * 0.44f), 4, 4, BLACK); }
    DrawRectangle(fx + fw / 3, y + (int)(s * 0.72f), fw / 3, 3, {150, 60, 60, 255});
    if (l->acc & 4) DrawRectangle(x + s / 4, y + s - s / 7, s / 2, 3, GOLD);
}

// ================================================================ missions logic
static void startCut(int mi, bool intro);
static void setupMission(int k) {
    curM = k; mOn = true; mCount = 0; mStage = 0; bossDead = false; markOn = false; mT = 0; mDone = -1; mTimer = 0; mFar = 0; waveT = 3; rcPts.clear();
    peds.erase(std::remove_if(peds.begin(), peds.end(), [](const Ped& q) { return q.gang; }), peds.end());
    picks.erase(std::remove_if(picks.begin(), picks.end(), [](const Pick& q) { return q.type == 4; }), picks.end());
    cars[FOLLOW].active = false;
    Mission& m = MS[k]; obj = m.obj;
    switch (m.type) {
    case MT_GOTO: mark = IJ(m.ti, m.tj); markOn = true; break;
    case MT_STEAL: break;
    case MT_CRATES: case MT_HEIST:
        for (int i = 0; i < m.count; i++) {
            Pick q; q.type = 4; q.once = true;
            if (m.type == MT_HEIST) q.p = {(float)(20 + GetRandomValue(-7, 7)) * P + rf(-5, 5), 0.f, (float)(20 + GetRandomValue(-7, 7)) * P + rf(-20, 20)};
            else q.p = {(float)GetRandomValue(2, N - 2) * P + rf(-5, 5), 0.f, (float)GetRandomValue(2, N - 2) * P + rf(-20, 20)};
            q.p.x = Clamp(q.p.x, 10.f, WORLD - 10.f); q.p.z = Clamp(q.p.z, 10.f, WORLD - 10.f); picks.push_back(q);
        }
        break;
    case MT_DELIVER: mark = IJ(m.ti, m.tj); markOn = true; break;
    case MT_FOLLOW: { Car& f = cars[FOLLOW]; f.active = true; f.police = false; placeOnRoad(f, pl.p, 40, 80); f.c = {255, 230, 50, 255}; markOn = true; mark = cpos(f); break; }
    case MT_KILL: mark = IJ(m.ti, m.tj); markOn = true; spawnGang(mark, m.count, false); break;
    case MT_EVADE: pl.heat = 3; pl.last = 0; break;
    case MT_RACE: {
        int ci = m.ti, cj = m.tj;
        for (int k2 = 0; k2 < m.count; k2++) {
            int step = GetRandomValue(3, 6) * (GetRandomValue(0, 1) ? 1 : -1);
            if (k2 % 2 == 0) ci = (int)Clamp((float)(ci + step), 2.f, (float)N - 2); else cj = (int)Clamp((float)(cj + step), 2.f, (float)N - 2);
            rcPts.push_back(IJ(ci, cj));
        }
        mark = rcPts[0]; markOn = true; mTimer = (float)m.limit; break; }
    case MT_SURVIVE: mark = IJ(m.ti, m.tj); markOn = true; break;
    case MT_BOSS: mark = IJ(m.ti, m.tj); markOn = true; spawnGang(mark, 10, true, 10); break;
    }
}
static void failMission(const char* why) {
    mOn = false; markOn = false; mFailT = 2.5f; toast(std::string("MISSION FAILED: ") + why); cars[FOLLOW].active = false;
}
static void completeMission() {
    mOn = false; markOn = false; mDone = curM; doneT = 3.f; bannerT = 3.5f; pl.money += MS[curM].reward; cars[FOLLOW].active = false; mTimer = 0;
    peds.erase(std::remove_if(peds.begin(), peds.end(), [](const Ped& q) { return q.gang && !q.dead; }), peds.end());
    playSfx(5);
}
static void updateMission(float dt) {
    if (mFailT > 0) { mFailT -= dt; if (mFailT <= 0) setupMission(curM); return; }
    if (!mOn) { if (mDone >= 0) { doneT -= dt; if (doneT <= 0) { int d = mDone; mDone = -1; startCut(d, false); } } return; }
    Mission& m = MS[curM]; mT += dt;
    if (mTimer > 0) { mTimer -= dt; if (mTimer <= 0) { failMission("time is up"); return; } }
    switch (m.type) {
    case MT_GOTO: if (d2(pl.p, mark) < 7) completeMission(); break;
    case MT_STEAL:
        if (mStage == 0 && pl.car >= 0) { mStage = 1; mark = IJ(m.ti, m.tj); markOn = true; obj = "Bring the car to Kenzhe's garage"; }
        if (mStage == 1 && pl.car >= 0 && d2(pl.p, mark) < 8) completeMission();
        break;
    case MT_CRATES: case MT_HEIST: if (mCount >= m.count) completeMission(); break;
    case MT_DELIVER:
        if (mStage == 0 && pl.car >= 0 && d2(pl.p, mark) < 9) { mStage = 1; mark = IJ(m.ui, m.uj); mTimer = (float)m.limit; obj = "Deliver the beer to Hotel Steppe - hurry!"; toast("Timer started"); }
        else if (mStage == 1 && pl.car >= 0 && d2(pl.p, mark) < 9) completeMission();
        break;
    case MT_FOLLOW: {
        Car& f = cars[FOLLOW]; mark = cpos(f);
        if (!f.alive) { failMission("the courier is dead"); break; }
        float d = d2(pl.p, mark); if (d > 110) { mFar += dt; if (mFar > 6) failMission("you lost the courier"); } else mFar = 0;
        obj = "Follow the yellow car  " + std::to_string(std::max(0, m.count - (int)mT)) + "s";
        if (mT >= m.count) completeMission();
        break; }
    case MT_KILL: if (mCount >= m.count) completeMission(); break;
    case MT_EVADE: if (mT > 8 && pl.heat <= 0) completeMission(); break;
    case MT_RACE:
        if (mCount < (int)rcPts.size() && d2(pl.p, rcPts[mCount]) < 14) { mCount++; playSfx(2, 1.4f); if (mCount < (int)rcPts.size()) mark = rcPts[mCount]; }
        if (mCount >= (int)rcPts.size()) completeMission();
        break;
    case MT_SURVIVE:
        waveT -= dt;
        if (waveT <= 0) { waveT = 7; for (int k = 0; k < 3; k++) { float a = rf(0, 2 * PI), r = rf(18, 30); bool alongX = fabsf(sinf(a)) > 0.7f; Vector3 c = {mark.x + (alongX ? sinf(a) * r : rf(-5, 5)), 0, mark.z + (alongX ? rf(-5, 5) : cosf(a) * r)}; spawnGang(c, 1, false, 1); } }
        obj = "Survive the attack  " + std::to_string(std::max(0, m.count - (int)mT)) + "s";
        if (mT >= m.count) completeMission();
        break;
    case MT_BOSS: if (bossDead) completeMission(); break;
    }
}

// ================================================================ cutscenes
static void buildCast() {
    cast.clear(); cast.push_back({lookOf("ARMAN"), cF, 0});
    std::vector<std::string> names;
    for (auto& l : cLines) { std::string w = l.who; if (w != "ARMAN" && w != "NARRATOR" && std::find(names.begin(), names.end(), w) == names.end()) names.push_back(w); }
    for (size_t k = 0; k < names.size(); k++) {
        float a = 0.9f + k * 1.6f; Vector3 p = {cF.x + sinf(a) * 2.5f, 0, cF.z + cosf(a) * 2.5f};
        cast.push_back({lookOf(names[k]), p, atan2f(cF.x - p.x, cF.z - p.z)});
    }
    if (cast.size() > 1) cast[0].yaw = atan2f(cast[1].p.x - cF.x, cast[1].p.z - cF.z);
}
static void startCut(int mi, bool intro) {
    Mission& m = MS[mi]; curM = mi; cMis = mi; cNext = intro ? 0 : 1; cLines = intro ? m.intro : m.outro;
    if (pl.car >= 0) { pl.p = add(cars[pl.car].p, {3, 0, 0}); pl.car = -1; }
    if (intro) {
        if (m.si >= 0) pl.p = IJ(m.si, m.sj);
        gHour = m.hour; pl.heat = 0; pl.hp = 100; pl.drunk = 0;
        for (auto& c : cars) if (c.active && !c.owned && d2(cpos(c), pl.p) < 45) { if (c.police) c.active = false; else placeOnRoad(c, pl.p, 100, 400); }
    }
    pl.p.y = 0; cF = pl.p; cIdx = 0; cLineT = 0; cSceneT = 0; cOut = -1; st = S_CUT; stT = 0; phone = false; buildCast();
    for (auto& q : peds) if (!q.gang && d2(q.p, cF) < 8) q.p.x += 10;
}
static void endCut() {
    DisableCursor(); pl.p = cF; gCastDraw = nullptr;
    if (cNext == 0) { st = S_PLAY; setupMission(curM); }
    else {
        int nx = curM + 1;
        if (nx < (int)MS.size()) startCut(nx, true);
        else { st = S_PLAY; mOn = false; markOn = false; curM = -1; toast("FREE ROAM unlocked - enjoy Kokshe Beach!"); }
    }
}
static Camera3D cutCamera() {
    Camera3D c{}; c.up = {0, 1, 0}; c.fovy = 45; c.projection = CAMERA_PERSPECTIVE;
    std::string w = cLines[cIdx].who; int spk = -1;
    for (size_t k = 0; k < cast.size(); k++) if (cast[k].lk && w == cast[k].lk->n) spk = (int)k;
    gSpeaker = spk;
    Vector3 foc = {cF.x, cF.y + 1.4f, cF.z}; float tL = cLineT, a = cSceneT * 0.15f + cIdx;
    if (spk < 0) { c.position = {cF.x + sinf(a) * 55, 32 - tL * 0.6f, cF.z + cosf(a) * 55}; c.target = {cF.x, 6, cF.z}; c.fovy = 55; return c; }
    Vector3 s = cast[spk].p; int shot = (cIdx * 3 + cMis) % 4;
    Vector3 dv = {s.x - cF.x, 0, s.z - cF.z}; float dl = sqrtf(dv.x * dv.x + dv.z * dv.z); if (dl < 0.01f) { dv = {0, 0, 1}; dl = 1; } dv = {dv.x / dl, 0, dv.z / dl};
    Vector3 perp = {dv.z, 0, -dv.x};
    int lis = (spk == 0 && cast.size() > 1) ? 1 : 0; if (lis == spk && shot == 2) shot = 1;
    switch (shot) {
    case 0: c.position = {foc.x + sinf(a) * 9, 2.2f, foc.z + cosf(a) * 9}; c.target = foc; break;
    case 1: { float dd = 2.6f - 0.08f * tL; c.position = {s.x - dv.x * dd + perp.x * 0.9f, 1.9f, s.z - dv.z * dd + perp.z * 0.9f}; c.target = {s.x, 1.85f, s.z}; c.fovy = 38; break; }
    case 2: { Vector3 l = cast[lis].p; Vector3 ds = {s.x - l.x, 0, s.z - l.z}; float n = fmaxf(0.01f, sqrtf(ds.x * ds.x + ds.z * ds.z)); ds = {ds.x / n, 0, ds.z / n};
              c.position = {l.x - ds.x * 0.9f + ds.z * 0.5f, 2.1f, l.z - ds.z * 0.9f - ds.x * 0.5f}; c.target = {s.x, 1.8f, s.z}; c.fovy = 40; break; }
    default: { float r = 8 - 0.3f * tL; c.position = {cF.x + sinf(a) * r, 0.8f, cF.z + cosf(a) * r}; c.target = {s.x, 1.3f, s.z}; break; }
    }
    return c;
}

// ================================================================ gameplay
static void explodeCar(int i) {
    Car& c = cars[i]; c.alive = false; c.dead = 0; c.spd = 0; fxs.push_back({cpos(c), 0}); playSfx(1);
    if (pl.car == i) { pl.car = -1; pl.hp -= 45; pl.p = add(c.p, {3, 0, 0}); }
    else if (d2(cpos(c), pl.p) < 7) pl.hp -= 30;
    if (c.police) crime(1.0f);
}
static void damagePlayer(float a) {
    if (pl.car >= 0) { cars[pl.car].hp -= a * 0.6f; pl.hp -= a * 0.15f; }
    else { float ab = fminf(pl.ar, a * 0.7f); pl.ar -= ab; pl.hp -= a - ab; }
    shake = 0.15f;
}
static void killPed(Ped& q) {
    q.dead = true; q.fall = 0;
    Pick k; k.p = q.p; k.type = 1; k.once = true; picks.push_back(k);
    if (q.gang) { if (mOn && curM >= 0 && MS[curM].type == MT_KILL) mCount++; if (q.boss) bossDead = true; }
    else crime(0.8f);
}
static void shootGun() {
    struct W { float cd, dmg, range, spread; } w[3] = {{0.45f, 12, 2.4f, 0}, {0.35f, 30, 65, 0.01f}, {0.09f, 13, 55, 0.06f}};
    W& s = w[pl.wpn]; if (pl.cd > 0) return;
    if (pl.wpn > 0 && pl.ammo[pl.wpn] <= 0) { toast("Out of ammo"); pl.cd = 0.4f; return; }
    pl.cd = s.cd; if (pl.wpn > 0) { pl.ammo[pl.wpn]--; playSfx(0, rf(0.9f, 1.15f)); } else playSfx(4, 1.6f);
    float yaw = camYaw + rf(-s.spread, s.spread) + sinf(gT * 2) * pl.drunk * 0.002f;
    Vector3 dr = {sinf(yaw), 0, cosf(yaw)}, o = {pl.p.x, 1.4f, pl.p.z};
    pl.yaw = camYaw; float best = s.range; Ped* hit = nullptr;
    for (auto& q : peds) {
        if (q.dead) continue; float vx = q.p.x - pl.p.x, vz = q.p.z - pl.p.z, t = vx * dr.x + vz * dr.z;
        if (t < 0 || t > best) continue;
        float px = vx - dr.x * t, pz = vz - dr.z * t; if (sqrtf(px * px + pz * pz) < 0.9f) { best = t; hit = &q; }
    }
    if (hit) { hit->hp -= s.dmg; hit->flee = 6; if (hit->hp <= 0) killPed(*hit); else if (!hit->gang) crime(0.15f); }
    else for (size_t i = 0; i < cars.size(); i++) {
        Car& c = cars[i]; if (!c.active || !c.alive || (int)i == pl.car) continue;
        Vector3 cp = cpos(c); float vx = cp.x - pl.p.x, vz = cp.z - pl.p.z, t = vx * dr.x + vz * dr.z;
        if (t < 0 || t > best) continue; float px = vx - dr.x * t, pz = vz - dr.z * t;
        if (sqrtf(px * px + pz * pz) < 2.2f) { best = t; c.hp -= s.dmg * 0.8f; if (c.police) crime(0.4f); }
    }
    if (pl.wpn > 0) { trs.push_back({{o.x + dr.x, o.y, o.z + dr.z}, {o.x + dr.x * best, o.y, o.z + dr.z * best}, 0.05f}); crime(0.02f); }
    for (auto& q : peds) if (!q.gang && !q.dead && d2(q.p, pl.p) < 30) q.flee = 6;
}

static void updatePlay(float dt) {
    gT += dt; gHour += dt / 30.f; if (gHour >= 24) gHour -= 24; msgT -= dt; pl.last += dt; shake = fmaxf(0, shake - dt);
    bannerT -= dt; radioT -= dt; pl.cd -= dt;
    gRainT -= dt; if (gRainT <= 0) { gRainOn = !gRainOn; gRainT = rf(60, 150); }
    gRain = Clamp(gRain + (gRainOn ? 1 : -1) * dt * 0.1f, 0, 1);
    Vector2 md = GetMouseDelta(); camYaw -= md.x * 0.0035f; camPit = Clamp(camPit + md.y * 0.003f, 0.08f, 1.3f);
    if (fabsf(md.x) > 0.5f) mouseIdle = 0; else mouseIdle += dt;
    pl.drunk = fmaxf(0, pl.drunk - 1.2f * dt);
    if (IsKeyPressed(KEY_ONE)) pl.wpn = 0;
    if (IsKeyPressed(KEY_TWO)) pl.wpn = 1;
    if (IsKeyPressed(KEY_THREE)) { if (pl.ammo[2] > 0) pl.wpn = 2; else toast("Find an SMG pickup first"); }
    if (IsKeyPressed(KEY_B) && pl.beers > 0) { pl.beers--; pl.drunk = fminf(100, pl.drunk + 25); pl.hp = fminf(100, pl.hp + 12); toast("Kokshe Lager: cold and refreshing"); playSfx(3); }
    if (IsKeyPressed(KEY_Q)) { radio = (radio + 1) % 4; radioT = 4; }

    Vector3 f = {sinf(camYaw), 0, cosf(camYaw)}, l = {cosf(camYaw), 0, -sinf(camYaw)};
    if (pl.car < 0) {
        float mx = (IsKeyDown(KEY_W) ? 1.f : 0.f) - (IsKeyDown(KEY_S) ? 1.f : 0.f), mz = (IsKeyDown(KEY_A) ? 1.f : 0.f) - (IsKeyDown(KEY_D) ? 1.f : 0.f);
        Vector3 mv = {f.x * mx + l.x * mz, 0, f.z * mx + l.z * mz}; float ml = sqrtf(mv.x * mv.x + mv.z * mv.z);
        bool sp = IsKeyDown(KEY_LEFT_SHIFT) && pl.stam > 0;
        if (ml > 0.01f) {
            float ang = atan2f(mv.x, mv.z) + sinf(gT * 2.3f) * pl.drunk * 0.01f; mv = {sinf(ang), 0, cosf(ang)};
            float spd = sp ? 9.f : 5.5f; pl.p.x += mv.x * spd * dt; pl.p.z += mv.z * spd * dt;
            pl.yaw = lerpA(pl.yaw, ang, 12 * dt); pl.anim += spd * dt * 1.6f;
            pl.stam = Clamp(pl.stam + (sp ? -25.f : 10.f) * dt, 0, 100);
        } else pl.stam = fminf(100, pl.stam + 20 * dt);
        if (IsKeyPressed(KEY_SPACE) && pl.p.y <= 0.01f) pl.vy = 7;
        pl.vy -= 22 * dt; pl.p.y = fmaxf(0, pl.p.y + pl.vy * dt); if (pl.p.y == 0) pl.vy = 0;
        resolve(pl.p, 0.5f);
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) shootGun();
        if (IsKeyPressed(KEY_E)) {
            int nr = -1; float bd = 4.8f;
            for (size_t i = 0; i < cars.size(); i++) { Car& c = cars[i]; if (!c.active || !c.alive) continue; float d = d2(cpos(c), pl.p); if (d < bd) { bd = d; nr = (int)i; } }
            int ks = -1; for (size_t i = 0; i < kiosks.size(); i++) if (d2(kiosks[i], pl.p) < 3.f) ks = (int)i;
            if (ks >= 0) { if (pl.money >= 5 && pl.beers < 9) { pl.money -= 5; pl.beers++; toast("Bought Kokshe Lager (-$5)"); playSfx(2, 0.8f); } else toast("Can't buy (money or full)"); }
            else if (nr >= 0) {
                Car& c = cars[nr]; Vector3 np = cpos(c);
                if (!c.owned && (fabsf(c.spd) > 1 || c.police)) crime(c.police ? 1.5f : 0.6f);
                c.owned = true; c.p = np; c.spd = 0; pl.car = nr;
            }
        }
    } else {
        Car& c = cars[pl.car];
        bool U = IsKeyDown(KEY_W) || IsKeyDown(KEY_UP), D = IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN), Lk = IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT), Rk = IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT);
        float th = (U ? 1.f : 0.f) - (D ? 1.f : 0.f), stv = (Lk ? 1.f : 0.f) - (Rk ? 1.f : 0.f), mxs = maxSpeed(c);
        if (th > 0) c.spd += (c.spd < 0 ? 40.f : 20.f) * dt; else if (th < 0) c.spd -= (c.spd > 0 ? 45.f : 12.f) * dt; else c.spd *= 1 - 0.4f * dt;
        if (IsKeyDown(KEY_SPACE)) c.spd *= 1 - 3 * dt;
        c.spd = Clamp(c.spd, -12.f, mxs);
        stv += sinf(gT * 1.5f) * pl.drunk / 200.f;
        c.yaw = wrapA(c.yaw + stv * 2.2f * dt * Clamp(c.spd / 7.f, -1.f, 1.f));
        c.p.x += sinf(c.yaw) * c.spd * dt; c.p.z += cosf(c.yaw) * c.spd * dt;
        if (resolve(c.p, 2.3f)) { float im = fabsf(c.spd); if (im > 9) { c.hp -= im * 0.8f; shake = 0.25f; playSfx(4, 0.7f); } c.spd *= -0.2f; }
        c.p.x = Clamp(c.p.x, -10.f, WORLD + 10.f); c.p.z = Clamp(c.p.z, -10.f, WORLD + 10.f);
        for (size_t i = 0; i < cars.size(); i++) {
            if ((int)i == pl.car) continue; Car& o = cars[i]; if (!o.active || !o.alive) continue;
            Vector3 op = cpos(o); float d = d2(op, c.p);
            if (d < 3.8f) {
                Vector3 n = {c.p.x - op.x, 0, c.p.z - op.z}; float len = fmaxf(0.01f, sqrtf(n.x * n.x + n.z * n.z));
                c.p.x += n.x / len * (3.8f - d); c.p.z += n.z / len * (3.8f - d);
                float im = fabsf(c.spd); if (im > 6) { c.hp -= im * 0.5f; o.hp -= im * 0.9f; o.stun = 1.2f; crime(o.police ? 1.0f : 0.2f); shake = 0.2f; playSfx(4, 0.8f); }
                c.spd *= 0.4f; o.spd = 0;
            }
        }
        for (auto& q : peds) if (!q.dead && fabsf(c.spd) > 6 && d2(q.p, c.p) < 2.4f) { killPed(q); shake = 0.1f; }
        pl.p = {c.p.x, 0, c.p.z}; pl.yaw = c.yaw; pl.anim += dt;
        if (IsKeyPressed(KEY_E)) { pl.p = add(c.p, {l.x * 3.2f, 0, l.z * 3.2f}); resolve(pl.p, 0.6f); c.spd = 0; pl.car = -1; }
        if (mouseIdle > 1.2f && fabsf(c.spd) > 2) camYaw = lerpA(camYaw, c.yaw + (c.spd < 0 ? PI : 0), 3 * dt);
    }
    pl.p.x = Clamp(pl.p.x, -10.f, WORLD + 10.f); pl.p.z = Clamp(pl.p.z, -10.f, WORLD + 10.f);

    // ---- traffic & police
    int ns = stars(), cops = 0;
    for (size_t i = 0; i < cars.size(); i++) {
        Car& c = cars[i]; if (!c.active) continue;
        if (!c.alive) { c.dead += dt; if (c.dead > 20 && (int)i != pl.car && d2(c.p, pl.p) > 120) { if (c.police || (int)i == FOLLOW) c.active = false; else placeOnRoad(c, pl.p, 120, 1e9f); c.alive = true; c.hp = 100; } continue; }
        if (c.hp <= 0) { explodeCar((int)i); continue; }
        if (c.owned) continue;
        if (c.police) { cops++; if (ns == 0 && d2(c.p, pl.p) > 150) { c.active = false; continue; } }
        if (c.stun > 0) { c.stun -= dt; continue; }
        float target = c.police && ns > 0 ? 20.f + ns * 3 : 11.f + (i % 5);
        if ((int)i == FOLLOW) target = 13;
        c.spd += (target - c.spd) * Clamp(dt * 1.5f, 0, 1);
        Vector3 fv = dirv(c.dir); float a0 = (c.dir & 1) ? c.p.x : c.p.z;
        c.p.x += fv.x * c.spd * dt; c.p.z += fv.z * c.spd * dt;
        float a1 = (c.dir & 1) ? c.p.x : c.p.z; int k0 = (int)floorf(a0 / P), k1 = (int)floorf(a1 / P);
        if (k0 != k1) {
            int k = std::max(k0, k1); if (c.dir & 1) c.p.x = k * P; else c.p.z = k * P;
            int kx = (int)roundf(c.p.x / P), kz = (int)roundf(c.p.z / P);
            int opts[4], n = 0;
            for (int d = 0; d < 4; d++) {
                bool ok = d == 0 ? kz < N : d == 2 ? kz > 0 : d == 1 ? kx < N : kx > 0;
                if (ok && d != (c.dir + 2) % 4) opts[n++] = d;
            }
            if (n == 0) opts[n++] = (c.dir + 2) % 4;
            int pick = opts[GetRandomValue(0, n - 1)];
            if (c.police && ns > 0) { float bd = 1e9f; for (int q = 0; q < n; q++) { Vector3 t = add(c.p, {dirv(opts[q]).x * P, 0, dirv(opts[q]).z * P}); float d = d2(t, pl.p); if (d < bd) { bd = d; pick = opts[q]; } } }
            c.dir = pick;
        }
        c.yaw = lerpA(c.yaw, DY[c.dir], 8 * dt);
        if (c.police && ns > 0) { float d = d2(cpos(c), pl.p); if (d < 32) damagePlayer((2.5f + ns) * dt * (d < 8 ? 2.f : 1.f)); }
    }
    copT -= dt;
    if (ns > 0 && cops < ns + 1 && copT <= 0) {
        for (int i = TRAFFIC; i < TRAFFIC + COPS; i++) if (!cars[i].active) { cars[i].active = true; placeOnRoad(cars[i], pl.p, 80, 150); break; }
        copT = 4;
    }
    // ---- peds
    int filming = 0;
    for (auto& q : peds) {
        if (q.dead) { q.fall += dt; continue; }
        float dist = d2(q.p, pl.p); if (dist > 140) continue;
        q.anim += dt * (q.spd > 3 ? 10.f : 6.f); q.film = false;
        if (q.gang) {
            Vector3 dv = {pl.p.x - q.p.x, 0, pl.p.z - q.p.z}; float dl = fmaxf(0.01f, sqrtf(dv.x * dv.x + dv.z * dv.z));
            q.yaw = atan2f(dv.x, dv.z);
            if (dist < 55 && dist > 12) { q.p.x += dv.x / dl * 3.5f * dt; q.p.z += dv.z / dl * 3.5f * dt; }
            q.shoot -= dt;
            if (q.shoot <= 0 && dist < 40) {
                q.shoot = rf(0.6f, 1.4f);
                trs.push_back({{q.p.x, 1.4f, q.p.z}, {pl.p.x, 1.2f, pl.p.z}, 0.06f});
                if (rf(0, 1) < 0.5f) { damagePlayer(q.boss ? 10.f : 6.f); if (dist < 25) playSfx(0, 0.8f); }
            }
        } else {
            if (q.flee > 0) { q.flee -= dt; Vector3 dv = {q.p.x - pl.p.x, 0, q.p.z - pl.p.z}; q.yaw = atan2f(dv.x, dv.z); q.p.x += sinf(q.yaw) * 5.5f * dt; q.p.z += cosf(q.yaw) * 5.5f * dt; }
            else { q.t -= dt; if (q.t <= 0) { q.yaw = rf(-PI, PI); q.t = rf(2, 6); } q.p.x += sinf(q.yaw) * q.spd * dt; q.p.z += cosf(q.yaw) * q.spd * dt; }
            if (pl.heat > 0 && dist < 22) { q.film = true; filming++; }
        }
        if (resolve(q.p, 0.4f)) q.yaw += PI;
        q.p.x = Clamp(q.p.x, 0.f, WORLD); q.p.z = Clamp(q.p.z, 0.f, WORLD);
    }
    if (filming >= 3) pl.heat = fminf(5.f, pl.heat + dt * 0.05f);
    bool copNear = false; for (int i = TRAFFIC; i < TRAFFIC + COPS; i++) if (cars[i].active && d2(cars[i].p, pl.p) < 50) copNear = true;
    if (!copNear && pl.last > 6 && !(mOn && curM >= 0 && MS[curM].type == MT_EVADE && mT < 8)) pl.heat = fmaxf(0, pl.heat - dt / 12.f);
    // ---- pickups
    for (auto& k : picks) {
        if (!k.on) { if (!k.once && k.type != 4) { k.rs -= dt; if (k.rs <= 0) k.on = true; } continue; }
        if (d2(k.p, pl.p) < 1.9f) {
            k.on = false; k.rs = 60;
            switch (k.type) {
            case 0: if (pl.beers < 9) { pl.beers++; toast("Picked up a Kokshe Lager"); playSfx(2); } else k.on = true; break;
            case 1: { int m = GetRandomValue(20, 80); pl.money += m; toast("+$" + std::to_string(m)); playSfx(2, 1.3f); break; }
            case 2: pl.ar = 100; toast("Body armor"); playSfx(2, 0.9f); break;
            case 3: pl.ammo[2] += 90; pl.ammo[1] += 30; toast("SMG and ammo"); playSfx(2, 0.7f); break;
            case 4: mCount++; playSfx(2, 1.2f);
                if (mOn && curM >= 0 && MS[curM].type == MT_HEIST) { toast("Cash bag " + std::to_string(mCount) + "/" + std::to_string(MS[curM].count)); if (mCount == 1) crime(2.f); }
                else if (mOn && curM >= 0) toast("Beer crate " + std::to_string(mCount) + "/" + std::to_string(MS[curM].count));
                break;
            }
        }
    }
    for (auto& t : trs) t.t -= dt;
    trs.erase(std::remove_if(trs.begin(), trs.end(), [](const Tr& t) { return t.t <= 0; }), trs.end());
    for (auto& fx : fxs) fx.t += dt;
    fxs.erase(std::remove_if(fxs.begin(), fxs.end(), [](const Fx& f) { return f.t > 1.2f; }), fxs.end());
    updateMission(dt);
    if (pl.hp <= 0) { st = S_WASTED; wastedT = 0; }
}

// ================================================================ HUD
static void drawMinimap() {
    int x0 = 8, y0 = RH - 112, S = 104; float sc = S / 230.f;
    DrawRectangle(x0 - 2, y0 - 2, S + 4, S + 4, BLACK); DrawRectangle(x0, y0, S, S, {70, 70, 90, 255});
    auto rc = [&](float x, float y, float w, float h, Color c) {
        float ax = fmaxf(x, (float)x0), ay = fmaxf(y, (float)y0), bx = fminf(x + w, (float)(x0 + S)), by = fminf(y + h, (float)(y0 + S));
        if (bx > ax && by > ay) DrawRectangle((int)ax, (int)ay, (int)(bx - ax), (int)(by - ay), c);
    };
    float cx = x0 + S / 2.f, cy = y0 + S / 2.f;
    auto tx = [&](float wx) { return cx + (wx - pl.p.x) * sc; };
    auto tz = [&](float wz) { return cy + (wz - pl.p.z) * sc; };
    int ci = (int)(pl.p.x / P), cj = (int)(pl.p.z / P);
    for (int i = std::max(0, ci - 3); i <= std::min(N - 1, ci + 3); i++) for (int j = std::max(0, cj - 3); j <= std::min(N - 1, cj + 3); j++)
        for (int id : cellB[i][j]) { Bld& b = B[id]; rc(tx(b.x - b.w / 2), tz(b.z - b.d / 2), b.w * sc, b.d * sc, {40, 30, 70, 255}); }
    for (auto& k : kiosks) rc(tx(k.x) - 1, tz(k.z) - 1, 3, 3, {255, 160, 0, 255});
    for (auto& q : picks) if (q.type == 4 && q.on) rc(tx(q.p.x) - 2, tz(q.p.z) - 2, 4, 4, YELLOW);
    for (int i = TRAFFIC; i < TRAFFIC + COPS; i++) if (cars[i].active) rc(tx(cars[i].p.x) - 2, tz(cars[i].p.z) - 2, 4, 4, ((int)(gT * 4) & 1) ? RED : BLUE);
    for (auto& q : peds) if (q.gang && !q.dead) rc(tx(q.p.x) - 1, tz(q.p.z) - 1, 3, 3, {255, 60, 60, 255});
    if (markOn) { float mx2 = Clamp(tx(mark.x), (float)x0 + 3, (float)(x0 + S - 6)), mz2 = Clamp(tz(mark.z), (float)y0 + 3, (float)(y0 + S - 6)); DrawRectangle((int)mx2, (int)mz2, 6, 6, YELLOW); }
    DrawCircle((int)cx, (int)cy, 3, {0, 255, 240, 255});
    DrawLine((int)cx, (int)cy, (int)(cx + sinf(pl.yaw) * 8), (int)(cy + cosf(pl.yaw) * 8), WHITE);
}
static void drawHUD() {
    char b[128];
    snprintf(b, sizeof b, "$%08d", pl.money); T(b, RW - 12 - 150, 10, 26, {110, 230, 120, 255});
    for (int i = 0; i < 5; i++) { if (i < stars()) drawStar2D((float)(RW - 160 + i * 28), 52, 11, 5, ((int)(gT * 4) & 1) ? GOLD : ORANGE); else drawStar2D((float)(RW - 160 + i * 28), 52, 11, 5, {70, 70, 80, 255}); }
    DrawRectangle(RW - 170, 70, 160, 10, {40, 0, 0, 255}); DrawRectangle(RW - 170, 70, (int)(160 * Clamp(pl.hp, 0, 100) / 100), 10, {230, 40, 60, 255});
    DrawRectangle(RW - 170, 84, 160, 6, {0, 20, 40, 255}); DrawRectangle(RW - 170, 84, (int)(1.6f * pl.ar), 6, {60, 140, 255, 255});
    static const char* WN[3] = {"FISTS", "PISTOL", "SMG"};
    snprintf(b, sizeof b, "%s %d", WN[pl.wpn], pl.wpn ? pl.ammo[pl.wpn] : 0); T(b, RW - 170, 96, 16, WHITE);
    snprintf(b, sizeof b, "BEER x%d   DRUNK %d%%", pl.beers, (int)pl.drunk); T(b, RW - 170, 114, 14, {255, 200, 60, 255});
    int hh = (int)gHour, mm = (int)((gHour - hh) * 60); snprintf(b, sizeof b, "%02d:%02d %s", hh, mm, gRain > 0.3f ? "RAIN" : "CLEAR"); T(b, RW - 170, 132, 14, {200, 200, 255, 255});
    drawMinimap();
    if (mOn && curM >= 0) {
        DrawRectangle(122, RH - 66, 460, 56, ColorAlpha(BLACK, 0.55f));
        T(MS[curM].title, 130, RH - 62, 18, {255, 90, 190, 255});
        std::string o = obj; int ty = MS[curM].type;
        if (ty == MT_CRATES || ty == MT_HEIST || ty == MT_KILL || ty == MT_RACE) o += "  " + std::to_string(mCount) + "/" + std::to_string(MS[curM].count);
        T(o.c_str(), 130, RH - 38, 14, WHITE);
    } else if (curM < 0) T("FREE ROAM - Kokshe Beach", 130, RH - 30, 16, WHITE);
    if (mTimer > 0) { snprintf(b, sizeof b, "TIME %d:%02d", (int)mTimer / 60, (int)mTimer % 60); T(b, RW / 2, 12, 24, mTimer < 20 ? RED : WHITE, true); }
    if (msgT > 0) T(msg.c_str(), RW / 2, 44, 18, WHITE, true);
    if (radioT > 0 && radio < 3) { snprintf(b, sizeof b, "%s: %s", RADIO[radio], TRK[radio][((int)(gT / 45)) % 3]); T(b, RW / 2, RH - 24, 14, {0, 255, 240, 255}, true); }
    else if (radioT > 0) T("RADIO OFF", RW / 2, RH - 24, 14, {0, 255, 240, 255}, true);
    if (pl.car < 0 && pl.wpn > 0) { DrawLine(RW / 2 - 6, RH / 2, RW / 2 + 6, RH / 2, WHITE); DrawLine(RW / 2, RH / 2 - 6, RW / 2, RH / 2 + 6, WHITE); }
    for (auto& k : kiosks) if (pl.car < 0 && d2(k, pl.p) < 3.f) T("[E] Buy Kokshe Lager $5", RW / 2, RH / 2 + 40, 16, {255, 200, 60, 255}, true);
    if (pl.car >= 0) { snprintf(b, sizeof b, "%d km/h   CAR %d%%   [Q] radio", (int)(fabsf(cars[pl.car].spd) * 3.6f), (int)Clamp(cars[pl.car].hp, 0, 100)); T(b, RW / 2, RH - 48, 16, WHITE, true); }
    if (bannerT > 0 && mDone >= 0) {
        T("MISSION PASSED", RW / 2 + 3, 150 + 3, 56, {0, 220, 255, 255}, true); T("MISSION PASSED", RW / 2, 150, 56, {255, 80, 190, 255}, true);
        snprintf(b, sizeof b, "$%d", MS[mDone].reward); T(b, RW / 2, 215, 28, {110, 230, 120, 255}, true);
    }
    if (pl.drunk > 0) DrawRectangle(0, 0, RW, RH, ColorAlpha({255, 140, 0, 255}, pl.drunk / 400.f));
    if (gRain > 0.05f) for (int i = 0; i < (int)(120 * gRain); i++) { int x = GetRandomValue(0, RW), y = GetRandomValue(0, RH); DrawLine(x, y, x - 3, y + 12, ColorAlpha({200, 210, 255, 255}, 0.5f)); }
}
static void drawPhone() {
    DrawRectangle(RW / 2 - 130, 60, 260, 360, {20, 20, 30, 255}); DrawRectangleLines(RW / 2 - 130, 60, 260, 360, {255, 90, 190, 255});
    T("QAZAQGRAM PHONE", RW / 2, 72, 18, {0, 255, 240, 255}, true);
    T("1  Order beer delivery  $8", RW / 2 - 115, 110, 14, WHITE);
    T("2  Taxi to objective    $25", RW / 2 - 115, 134, 14, WHITE);
    T("3  Skip 3 hours", RW / 2 - 115, 158, 14, WHITE);
    T("4  Toggle rain", RW / 2 - 115, 182, 14, WHITE);
    T("-- FEED --", RW / 2, 226, 14, {255, 200, 60, 255}, true);
    T("@aibek_pivo: new Lager kiosk by the beach!", RW / 2 - 118, 250, 11, WHITE);
    T("@dana_k: anyone seen a guy in a pink shirt?", RW / 2 - 118, 270, 11, WHITE);
    T("@kokshe_news: police chase near downtown", RW / 2 - 118, 290, 11, WHITE);
    T("@baursak_don: nobody touches my kumys.", RW / 2 - 118, 310, 11, WHITE);
    T("TAB to close", RW / 2, 395, 12, GRAY, true);
}

// ================================================================ main
int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1280, 720, "Grand Rescue Protocol - KZstar Kokshetay");
    InitAudioDevice(); SetAudioStreamBufferSizeDefault(2048); gStream = LoadAudioStream(22050, 16, 1); SetAudioStreamVolume(gStream, 0.7f); PlayAudioStream(gStream);
    for (int i = 0; i < 6; i++) sfx[i] = makeSnd(i);
    SetExitKey(KEY_NULL); SetTargetFPS(60);
    RenderTexture2D rt = LoadRenderTexture(RW, RH); SetTextureFilter(rt.texture, TEXTURE_FILTER_POINT);
    initTextures(); initMissions(); genWorld(); rlSetClipPlanes(0.3, 500.0); gHour = 18.5f;
    bool quit = false;
    while (!WindowShouldClose() && !quit) {
        float dt = fminf(GetFrameTime(), 0.05f); stT += dt;
        Camera3D cam{}; cam.up = {0, 1, 0}; cam.fovy = 60; cam.projection = CAMERA_PERSPECTIVE;
        Vector3 view = pl.p; bool showP = true; gCastDraw = nullptr;
        updateSky();
        if (st == S_SPLASH) { if (stT > 8.5f || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) { st = S_TITLE; stT = 0; } }
        else if (st == S_TITLE) {
            float a = stT * 0.12f; Vector3 c = {WORLD / 2, 0, WORLD / 2}; view = c; showP = false;
            cam.position = {c.x + sinf(a) * 200, 55, c.z + cosf(a) * 200}; cam.target = {c.x, 10, c.z};
            if (IsKeyPressed(KEY_ENTER)) { DisableCursor(); pl = Player(); gHour = 18.5f; startCut(0, true); }
            if (IsKeyPressed(KEY_ESCAPE)) quit = true;
        } else if (st == S_CUT) {
            if (IsCursorHidden()) EnableCursor();
            cSceneT += dt; cLineT += dt; showP = false; gCastDraw = &cast; cam = cutCamera(); view = cF;
            size_t full = std::string(cLines[cIdx].text).size();
            if (cOut >= 0) { cOut -= dt; if (cOut < 0) endCut(); }
            else {
                if (IsKeyPressed(KEY_ESCAPE)) cOut = 0.3f;
                else if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    if (cLineT * 45 < full) cLineT = full / 45.f + 0.01f;
                    else if (cIdx + 1 >= (int)cLines.size()) cOut = 0.5f;
                    else { cIdx++; cLineT = 0; }
                }
            }
        } else if (st == S_PAUSE) {
            if (IsCursorHidden()) EnableCursor();
            if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER)) { st = S_PLAY; DisableCursor(); }
            if (IsKeyPressed(KEY_Q)) quit = true;
        } else if (st == S_WASTED) {
            wastedT += dt;
            if (wastedT > 3.5f) {
                pl.hp = 100; pl.ar = 0; pl.p = START; pl.car = -1; pl.money -= std::min(pl.money, 100); pl.heat = 0; pl.drunk = 0;
                if (mOn) setupMission(curM); st = S_PLAY;
            }
        } else if (st == S_PLAY) {
            if (IsKeyPressed(KEY_ESCAPE)) st = S_PAUSE;
            else if (IsKeyPressed(KEY_TAB)) phone = !phone;
            if (phone) {
                if (IsCursorHidden()) EnableCursor();
                if (IsKeyPressed(KEY_ONE) && pl.money >= 8 && pl.beers < 9) { pl.money -= 8; pl.beers++; toast("Beer delivered"); }
                if (IsKeyPressed(KEY_TWO) && markOn && pl.money >= 25) { pl.money -= 25; pl.p = {mark.x + 9, 0, mark.z + 9}; pl.car = -1; phone = false; toast("Taxi dropped you off"); }
                if (IsKeyPressed(KEY_THREE)) gHour = fmodf(gHour + 3, 24);
                if (IsKeyPressed(KEY_FOUR)) { gRainOn = !gRainOn; gRainT = 90; }
            } else { if (!IsCursorHidden()) DisableCursor(); updatePlay(dt); }
        }
        if (st == S_PLAY || st == S_PAUSE || st == S_WASTED) {
            float dist = pl.car >= 0 ? 11.f : 7.f, pit = camPit; Vector3 tg = {pl.p.x, pl.p.y + 1.6f, pl.p.z};
            Vector3 cp = {tg.x - sinf(camYaw) * cosf(pit) * dist, tg.y + sinf(pit) * dist, tg.z - cosf(camYaw) * cosf(pit) * dist};
            cp.y = fmaxf(cp.y, 0.6f);
            if (shake > 0) { cp.x += rf(-1, 1) * shake; cp.y += rf(-1, 1) * shake; }
            cam.position = cp; cam.target = tg; cam.fovy = 60 + sinf(gT * 1.3f) * pl.drunk * 0.08f;
            if (st == S_WASTED) cam.position.y += wastedT * 2;
            view = pl.p;
        }
        // ---- music: theme in menus/cutscenes, radio only inside cars
        bool musicOn = false; int stn = 3;
        if (st == S_TITLE || st == S_CUT) { musicOn = true; stn = 3; }
        else if (st == S_PLAY && pl.car >= 0 && radio < 3 && !phone) { musicOn = true; stn = radio; }
        musicFeed(stn, musicOn);

        BeginTextureMode(rt);
        ClearBackground(BLACK);
        if (st != S_SPLASH) renderScene(cam, view, showP);
        if (st == S_SPLASH) {
            float t = stT; float al = t < 0.8f ? 0 : t < 2 ? (t - 0.8f) / 1.2f : t < 4.5f ? 1 : t < 5.5f ? 1 - (t - 4.5f) : 0;
            if (t < 5.6f) {
                logoKZ(RW / 2, RH / 2 - 40, 1.1f, al);
                T("kzstar", RW / 2, RH / 2 + 50, 40, ColorAlpha({255, 140, 0, 255}, al), true);
                T("K O K S H E T A Y", RW / 2, RH / 2 + 95, 18, ColorAlpha(WHITE, al), true);
                T("(parody of a certain famous studio)", RW / 2, RH / 2 + 125, 12, ColorAlpha(GRAY, al), true);
            } else {
                float a2 = Clamp((t - 5.6f) / 1.0f, 0, 1) * (t > 8 ? 1 - (t - 8) * 2 : 1);
                T("KZSTAR KOKSHETAY", RW / 2, RH / 2 - 40, 30, ColorAlpha({255, 140, 0, 255}, a2), true);
                T("presents", RW / 2, RH / 2 + 5, 20, ColorAlpha(WHITE, a2), true);
                T("An original parody. Not affiliated with Rockstar Games.", RW / 2, RH - 40, 12, ColorAlpha(GRAY, a2), true);
            }
        } else if (st == S_TITLE) {
            DrawRectangle(0, 0, RW, RH, ColorAlpha(BLACK, 0.25f));
            T("GRAND RESCUE", RW / 2 + 4, 62 + 4, 64, {0, 220, 255, 255}, true); T("GRAND RESCUE", RW / 2, 62, 64, {255, 80, 190, 255}, true);
            T("PROTOCOL", RW / 2 + 4, 132 + 4, 84, {0, 220, 255, 255}, true); T("PROTOCOL", RW / 2, 132, 84, {255, 150, 60, 255}, true);
            if (((int)(stT * 2)) % 2 == 0) T("PRESS ENTER", RW / 2, 270, 26, WHITE, true);
            T("WASD move | MOUSE look | LMB shoot | E use/car | B drink beer | TAB phone | Q radio (in car)", RW / 2, 340, 12, WHITE, true);
            logoKZ(54, RH - 54, 0.5f, 1);
            T("KZstar Kokshetay", 92, RH - 66, 16, {255, 140, 0, 255});
            T("Unofficial parody - not affiliated with Rockstar / Take-Two", 92, RH - 44, 11, GRAY);
        } else if (st == S_CUT) {
            std::string w = cLines[cIdx].who;
            DrawRectangle(0, 0, RW, 50, BLACK); DrawRectangle(0, RH - 120, RW, 120, BLACK);
            if (cSceneT < 4.f && cIdx == 0) {
                float al = cSceneT < 0.8f ? 0 : cSceneT < 1.5f ? (cSceneT - 0.8f) / 0.7f : cSceneT < 3.2f ? 1 : 1 - (cSceneT - 3.2f) / 0.8f;
                char tb[48]; snprintf(tb, sizeof tb, "MISSION %d", cMis + 1);
                T(tb, 60, RH - 230, 16, ColorAlpha({0, 255, 240, 255}, al)); T(MS[cMis].title, 60, RH - 208, 36, ColorAlpha({255, 90, 190, 255}, al));
            }
            Color nc = {255, 200, 60, 255};
            if (w == "ARMAN") nc = {255, 90, 190, 255}; else if (w == "DANA") nc = {220, 120, 255, 255}; else if (w == "NARRATOR") nc = {0, 255, 240, 255}; else if (w == "KENZHE") nc = {90, 170, 255, 255};
            portrait(lookOf(w), 34, RH - 108, 84);
            T(w.c_str(), 140, RH - 108, 20, nc);
            std::string txt = cLines[cIdx].text; std::string shown = txt.substr(0, std::min((size_t)(cLineT * 45), txt.size()));
            TW(shown, 140, RH - 82, 15, WHITE, 76);
            T("SPACE next  |  ESC skip", RW - 210, RH - 22, 11, GRAY);
            float fa = cSceneT < 0.9f ? 1.f - cSceneT / 0.9f : 0.f; if (cOut >= 0) fa = 1.f - Clamp(cOut / 0.5f, 0.f, 1.f);
            if (fa > 0) DrawRectangle(0, 0, RW, RH, ColorAlpha(BLACK, fa));
        } else {
            drawHUD();
            if (phone) drawPhone();
            if (st == S_PAUSE) { DrawRectangle(0, 0, RW, RH, ColorAlpha(BLACK, 0.6f)); T("PAUSED", RW / 2, 170, 48, WHITE, true); T("ENTER - resume    Q - quit", RW / 2, 240, 20, WHITE, true); }
            if (st == S_WASTED) { DrawRectangle(0, 0, RW, RH, ColorAlpha({80, 0, 0, 255}, Clamp(wastedT / 2, 0, 0.6f))); T("WASTED", RW / 2, 190, 80, {230, 30, 40, 255}, true); }
        }
        EndTextureMode();

        BeginDrawing(); ClearBackground(BLACK);
        float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight(), sc = fminf(sw / RW, sh / RH);
        Rectangle src = {0, 0, (float)RW, -(float)RH}, dst = {(sw - RW * sc) / 2, (sh - RH * sc) / 2, RW * sc, RH * sc};
        DrawTexturePro(rt.texture, src, dst, {0, 0}, 0, WHITE);
        EndDrawing();
    }
    UnloadAudioStream(gStream); for (int i = 0; i < 6; i++) UnloadSound(sfx[i]);
    UnloadRenderTexture(rt); CloseAudioDevice(); CloseWindow();
    return 0;
}
