#pragma once
// Moteur de synthèse "Cross Table" : C++ pur, sans dépendance à JUCE (testable seul).
#include "Params.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <vector>

namespace cts
{
constexpr int kTableLen  = 2048;
constexpr int kMips      = 10;
constexpr int kNumTables = 64;
constexpr int kVoices    = 8;

// Banque de tables d'onde à un cycle, avec versions limitées en bande (mipmaps) contre l'aliasing.
class WaveBank
{
public:
    WaveBank();
    void build (int index, const float* cycle);          // cycle : kTableLen échantillons
    const float* get (int index, int mip) const;
private:
    std::vector<float> data_;
};

// Tables de substitution générées (PAS les tables de la ROM d'origine).
void buildDefaultBank (WaveBank&);

struct Env
{
    enum St { Off, Atk, Dec, Slp, Sus, Rel };
    St st = Off;
    float v = 0, brk = 1, sus = 1, aR = 0, dR = 0, sR = 0, rR = 0;
    void start (float atk, float dec, float slp, float brk_, float sus_, double sr);
    void release (float sec, double sr);
    float tick();
};

struct OscState { float ph = 0; int st = 0; float t = 0, m = 0; };

struct Lfo
{
    float ph = 0, t = 0, held = 0;
    uint32_t rng = 1u;
    float tick (int wave, float hz, float dt, float delaySec, float lvlInit, float lvlFinal);
};

struct Voice
{
    bool active = false, keyDown = false, sustained = false, osc2On = true;
    int note = 0, zone = 0;
    uint64_t age = 0;
    float curNote = 0, vel = 0, glide = 1;
    int p[NUM_PARAMS] = {};
    Env a1, a2, fe, pe1, pe2;
    OscState o[2];
    Lfo l[3];
    float s[4] = { 0, 0, 0, 0 };

    void start (const int* params, int note_, int zone_, float vel_, float startNote, double sr, uint64_t age_);
    void release (double sr);
    void releaseFast (double sr);
    void render (float* L, float* R, int n, const WaveBank&, double sr, float pitchBend, float modWheel);
};

class Engine
{
public:
    Engine();
    void prepare (double sampleRate);
    void noteOn (int note, float velocity);
    void noteOff (int note);
    void allNotesOff();
    void setSustain (bool on);
    void setPitchBend (float v)  { pitchBend_.store (v); }
    void setModWheel (float v)   { modWheel_.store (v); }
    void render (float* left, float* right, int numSamples);

    int  getParam (int zone, int id) const { return params_[zone][id].load (std::memory_order_relaxed); }
    void setParam (int zone, int id, int value);
    static int zoneOfNote (int note);

    WaveBank bank;

private:
    Voice* allocVoice();
    std::atomic<int> params_[kZones][NUM_PARAMS];
    std::array<Voice, kVoices> voices_;
    std::atomic<float> pitchBend_ { 0.f }, modWheel_ { 0.f };
    double sr_ = 44100.0;
    bool sustain_ = false;
    uint64_t counter_ = 0;
    int lastNote_[kZones];
};
} // namespace cts
