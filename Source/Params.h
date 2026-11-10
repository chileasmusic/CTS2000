#pragma once
// Modèle de paramètres du CTS-2000, d'après le manuel (famille 1 à 24).
// Source unique : la macro CTS_PARAMS génère l'enum, les noms, les plages et les valeurs par défaut.
// Les plages marquées "(manuel)" viennent du manuel ; les autres sont déduites (voir README).
#include <string>

namespace cts
{
constexpr int kZones = 5;
constexpr int kSliders = 6;
constexpr int kFamilies = 24;
constexpr int kBankSize = 24;
constexpr int kPresetsPerSet = 48;

enum class Kind { Num, OnOff, Wave, Dest, Pan, Inf16, Inf32 };

// X(id, nom LCD, type, min, max, défaut)
#define CTS_PARAMS(X) \
  /* F1  OSCILLATOR 1 / CROSS WAVE 1 : sliders A..F */ \
  X(O1_OCT,   "OSC1 OCTAVE",   Num,   0,  5,  2)   /* (manuel) 0-5 */ \
  X(O1_W1,    "OSC1 1ST WAVE", Num,   0, 61,  1)   /* (manuel) 0-61 */ \
  X(O1_DLY,   "DELAY TIME",    Inf16, 1, 16, 16)   /* (manuel) 1-15 + inf */ \
  X(O1_T1,    "TIME 1",        Num,   1, 16,  8)   /* (manuel) 1-16 */ \
  X(O1_BP,    "BREAK POINT",   Num,   1, 16,  8)   /* (manuel) 1-16 */ \
  X(O1_T2,    "TIME 2",        Num,   1, 16,  8)   /* (manuel) 1-16 */ \
  /* F2  OSCILLATOR 1 / CROSS WAVE 2 */ \
  X(O1_W2,    "TO 2ND WAVE",   Num,   0, 61,  1) \
  X(O1_W3,    "TO 3RD WAVE",   Num,   0, 61,  1) \
  X(O1_DLY2,  "DELAY TIME",    Inf16, 1, 16, 16) \
  X(O1_T1B,   "TIME 1",        Num,   1, 16,  8) \
  X(O1_BPB,   "BREAK POINT",   Num,   1, 16,  8) \
  X(O1_T2B,   "TIME 2",        Num,   1, 16,  8) \
  /* F3  OSCILLATOR 2 / CROSS WAVE 1 */ \
  X(O2_OCT,   "OSC2 OCTAVE",   Num,   0,  5,  2) \
  X(O2_W1,    "OSC2 1ST WAVE", Num,   0, 61,  1) \
  X(O2_DLY,   "DELAY TIME",    Inf16, 1, 16, 16) \
  X(O2_T1,    "TIME 1",        Num,   1, 16,  8) \
  X(O2_BP,    "BREAK POINT",   Num,   1, 16,  8) \
  X(O2_T2,    "TIME 2",        Num,   1, 16,  8) \
  /* F4  OSCILLATOR 2 / CROSS WAVE 2 */ \
  X(O2_W2,    "TO 2ND WAVE",   Num,   0, 61,  1) \
  X(O2_W3,    "TO 3RD WAVE",   Num,   0, 61,  1) \
  X(O2_DLY2,  "DELAY TIME",    Inf16, 1, 16, 16) \
  X(O2_T1B,   "TIME 1",        Num,   1, 16,  8) \
  X(O2_BPB,   "BREAK POINT",   Num,   1, 16,  8) \
  X(O2_T2B,   "TIME 2",        Num,   1, 16,  8) \
  /* F5  OSC 1 TUNE & FREQ EG */ \
  X(T1_SEMI,  "OSC1 SEMITON",  Num,  -5,  5,  0)   /* (manuel) -5..+5 */ \
  X(T1_FINE,  "OSC1 DETUNE",   Num, -31, 31,  0) \
  X(T1_EGLVL, "FREQ EG LEVEL", Num, -15, 15,  0) \
  X(T1_ATK,   "ATTACK TIME 1", Num,   1, 16,  1) \
  X(T1_DEC,   "DECAY TIME 1",  Num,   1, 16,  8)   /* (manuel) 1-16 */ \
  /* F6  OSC 2 TUNE & FREQ EG */ \
  X(T2_SEMI,  "OSC2 SEMITON",  Num,  -5,  5,  0) \
  X(T2_FINE,  "OSC2 DETUNE",   Num, -31, 31,  2) \
  X(T2_EGLVL, "FREQ EG LEVEL", Num, -15, 15,  0) \
  X(T2_ATK,   "ATTACK TIME 2", Num,   1, 16,  1) \
  X(T2_DEC,   "DECAY TIME 2",  Num,   1, 16,  8) \
  /* F7  OSC LEVELS */ \
  X(LV_O1,    "OSC 1 LEVEL",   Num,   0,  3,  3)   /* (manuel) 0-3 */ \
  X(LV_O2,    "OSC 2 LEVEL",   Num,   0,  3,  3) \
  /* F8  DCA 1 ENVELOPE */ \
  X(A1_ATK,   "ATTACK TIME 1", Num,   1, 32,  1)   /* (manuel) 1-32 */ \
  X(A1_DEC,   "DECAY TIME 1",  Num,   1, 32,  1) \
  X(A1_BRK,   "BREAK POINT",   Num,   0, 63, 63) \
  X(A1_SLP,   "SLOPE TIME",    Num,   1, 32,  1) \
  X(A1_SUS,   "SUSTAIN LEVEL", Num,   0, 63, 63) \
  X(A1_REL,   "RELEASE/PED.",  Inf32, 1, 32,  8)   /* (manuel) 1-31 + inf */ \
  /* F9  DCA 2 ENVELOPE */ \
  X(A2_ATK,   "ATTACK TIME 2", Num,   1, 32,  1) \
  X(A2_DEC,   "DECAY TIME 2",  Num,   1, 32,  1) \
  X(A2_BRK,   "BREAK POINT",   Num,   0, 63, 63) \
  X(A2_SLP,   "SLOPE TIME",    Num,   1, 32,  1) \
  X(A2_SUS,   "SUSTAIN LEVEL", Num,   0, 63, 63) \
  X(A2_REL,   "RELEASE/PED.",  Inf32, 1, 32,  8) \
  /* F10 DCF ENVELOPE */ \
  X(FD_ATK,   "ATTACK TIME F", Num,   1, 32,  1) \
  X(FD_DEC,   "DECAY TIME F",  Num,   1, 32,  8) \
  X(FD_BRK,   "BREAK POINT F", Num,   1, 64, 64)   /* (manuel) 1-64 */ \
  X(FD_SLP,   "SLOPE TIME F",  Num,   1, 32,  1) \
  X(FD_SUS,   "SUSTAIN LEVEL", Num,   1, 64, 64) \
  X(FD_REL,   "RELEASE/PED.",  Inf32, 1, 32,  8) \
  /* F11 FILTER */ \
  X(FL_CUT,   "CUTOFF",        Num,   0,128,128)   /* (manuel) 0-128 */ \
  X(FL_RES,   "RESONANCE",     Num,   0, 31,  0) \
  X(FL_EG,    "ENVELOPE LEVEL",Num, -63, 63,  0)   /* (manuel) -63..+63 */ \
  X(FL_KBD,   "KYBD TRACKING", Num,  -8,  8,  0) \
  /* F12 PAN POT & LEVEL */ \
  X(PAN,      "STEREO PAN POT",Pan,  -7,  7,  0)   /* (manuel) 7L..7R */ \
  X(LEVEL,    "ZONE LEVEL",    Num,   0, 31, 24) \
  /* F13 LFO 1 -> OSC 1 */ \
  X(L1_WAVE,  "LFO WAVE",      Wave,  0,  3,  0) \
  X(L1_RATE,  "LFO SPEED",     Num,   1, 32, 12) \
  X(L1_DLY,   "DELAY TIME",    Num,   0, 15,  0) \
  X(L1_INIT,  "INITIAL LEVEL", Num,   0, 15,  0) \
  X(L1_FIN,   "FINAL LEVEL",   Num,   0, 15,  0)   /* (manuel) 0-15 */ \
  /* F14 LFO 2 -> OSC 2 */ \
  X(L2_WAVE,  "LFO 2 WAVE",    Wave,  0,  3,  0) \
  X(L2_RATE,  "LFO 2 SPEED",   Num,   1, 32, 12) \
  X(L2_DLY,   "DELAY TIME",    Num,   0, 15,  0) \
  X(L2_INIT,  "INITIAL LEVEL", Num,   0, 15,  0) \
  X(L2_FIN,   "FINAL LEVEL",   Num,   0, 15,  0) \
  /* F15 LFO 3 -> DCA/DCF */ \
  X(L3_WAVE,  "LFO3 WAVE",     Wave,  0,  3,  0) \
  X(L3_RATE,  "LFO 3 SPEED",   Num,   1, 32, 12) \
  X(L3_DLY,   "DELAY TIME",    Num,   0, 15,  0) \
  X(L3_INIT,  "INITIAL LEVEL", Num,   0, 15,  0) \
  X(L3_FIN,   "FINAL LEVEL",   Num,   0, 15,  0) \
  X(L3_DEST,  "LFO3 DEST",     Dest,  0,  2,  2) \
  /* F16 WHEELS */ \
  X(W_P1,     "OSC 1 PITCH WH",Num,   1,  3,  1)   /* (manuel) 1-3 */ \
  X(W_P2,     "OSC 2 PITCH WH",Num,   1,  3,  1) \
  X(W_M1,     "LFO 1 MOD WH",  OnOff, 0,  1,  0) \
  X(W_M2,     "LFO 2 MOD WH",  OnOff, 0,  1,  0) \
  X(W_M3,     "LFO 3 MOD WH",  OnOff, 0,  1,  0)   /* (manuel) off/on */ \
  /* F17 VELOCITY */ \
  X(V_A1,     "DCA1 INIT.LEVEL",Num,  1, 32,  8)   /* (manuel) 1-32 */ \
  X(V_A2,     "DCA2 INIT.LEVEL",Num,  1, 32,  8) \
  X(V_F,      "DCF INIT. LEVEL",Num,  1, 32, 16)   /* (manuel) 1-32 */ \
  X(V_A2T,    "DCA2 THRS ON",  Num,   1, 16,  1) \
  /* F18 KEYBOARD MODE & PORTAMENTO */ \
  X(K_MONO,   "MONOPHONIC MODE",OnOff,0,  1,  0)   /* (manuel) off/on */ \
  X(K_PORT,   "PORTAMENTO",    OnOff, 0,  1,  0) \
  X(K_PSPD,   "PORTA SPEED",   Num,   1, 16,  6)   /* (manuel) 1-16 */ \
  /* F20 TRANSPOSE */ \
  X(TRANSP,   "GLOBAL TRANSP.",Num,  -6,  5,  0)

#define CTS_ENUM(id, n, k, mn, mx, df) id,
enum ParamId : int { CTS_PARAMS(CTS_ENUM) NUM_PARAMS };
#undef CTS_ENUM

struct ParamDef { const char* name; Kind kind; int mn, mx, def; };

#define CTS_DEF(id, n, k, mn, mx, df) { n, Kind::k, mn, mx, df },
inline constexpr ParamDef kDefs[NUM_PARAMS] = { CTS_PARAMS(CTS_DEF) };
#undef CTS_DEF

struct Family { const char* label; const char* lcd1; const char* lcd2; int first; int count; };

// Les 24 boutons de la façade. count = 0 : fonction spéciale (pas de paramètres de son).
inline constexpr Family kFam[kFamilies] = {
    { "OSC1\nCROSS 1",  "OSCILLATOR 1",   "CROSS WAVE 1",   O1_OCT,  6 },
    { "OSC1\nCROSS 2",  "OSCILLATOR 1",   "CROSS WAVE 2",   O1_W2,   6 },
    { "OSC2\nCROSS 1",  "OSCILLATOR 2",   "CROSS WAVE 1",   O2_OCT,  6 },
    { "OSC2\nCROSS 2",  "OSCILLATOR 2",   "CROSS WAVE 2",   O2_W2,   6 },
    { "OSC1\nTUNE/EG",  "OSC 1",          "TUNE-FREQ EG",   T1_SEMI, 5 },
    { "OSC2\nTUNE/EG",  "OSC 2",          "TUNE-FREQ EG",   T2_SEMI, 5 },
    { "OSC\nLEVELS",    "OSC. LEVELS",    "",               LV_O1,   2 },
    { "DCA 1\nENVELOPE","DCA1 ENVELOPE",  "GENERATOR",      A1_ATK,  6 },
    { "DCA 2\nENVELOPE","DCA2 ENVELOPE",  "GENERATOR",      A2_ATK,  6 },
    { "DCF\nENVELOPE",  "DCF ENVELOPE",   "GENERATOR",      FD_ATK,  6 },
    { "FILTER",         "FILTER",         "",               FL_CUT,  4 },
    { "PAN POT\n& LEVEL","PAN POT",       "& LEVEL",        PAN,     2 },
    { "LFO 1\nOSC 1",   "LFO 1",          "TO OSC. 1",      L1_WAVE, 5 },
    { "LFO 2\nOSC 2",   "LFO 2",          "TO OSC. 2",      L2_WAVE, 5 },
    { "LFO 3\nDCA/DCF", "LFO 3",          "TO DCA/DCF",     L3_WAVE, 6 },
    { "WHEELS",         "WHEELS",         "PITCH-MODUL.",   W_P1,    5 },
    { "VELOCITY",       "KYBD VELOCITY",  "RESPONSE",       V_A1,    4 },
    { "K.MODE\n& PORT.","KEYBOARD MODE",  "& PORTAMENTO",   K_MONO,  3 },
    { "MIDI",           "MIDI",           "FUNCTIONS",      0,       0 },
    { "TRANSPOSE",      "TRANSPOSE",      "",               TRANSP,  1 },
    { "WRITE\n& COPY",  "WRITE / COPY",   "",               0,       0 },
    { "ZONE\nCOPY",     "ZONE COPY",      "",               0,       0 },
    { "PRESET\nNAME",   "PRESET NAME",    "",               0,       0 },
    { "EDIT/\nCOMPARE", "EDIT/COMPARE",   "",               0,       0 },
};

inline std::string valueText(int id, int v)
{
    const ParamDef& d = kDefs[id];
    switch (d.kind)
    {
        case Kind::OnOff: return v ? "ON" : "OFF";
        case Kind::Wave:  { static const char* w[] = { "TRI", "SAW", "SQU", "RND" }; return w[v & 3]; }
        case Kind::Dest:  { static const char* w[] = { "DCA", "DCF", "BOTH" }; return w[v < 0 ? 0 : (v > 2 ? 2 : v)]; }
        case Kind::Pan:   return v == 0 ? std::string("C") : (v < 0 ? std::to_string(-v) + "L" : std::to_string(v) + "R");
        case Kind::Inf16: return v >= 16 ? std::string("INF") : std::to_string(v);
        case Kind::Inf32: return v >= 32 ? std::string("INF") : std::to_string(v);
        default:          return std::to_string(v);
    }
}
} // namespace cts
