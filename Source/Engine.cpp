#include "Engine.h"
#include <algorithm>
#include <cmath>
#include <complex>

namespace cts
{
namespace
{
constexpr double kPi = 3.14159265358979323846;
using cd = std::complex<double>;

void fft (cd* a, int n, bool inverse)
{
    for (int i = 1, j = 0; i < n; ++i)
    {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap (a[i], a[j]);
    }
    for (int len = 2; len <= n; len <<= 1)
    {
        const double ang = 2.0 * kPi / len * (inverse ? 1 : -1);
        const cd wl (std::cos (ang), std::sin (ang));
        for (int i = 0; i < n; i += len)
        {
            cd w (1.0, 0.0);
            for (int j = 0; j < len / 2; ++j)
            {
                const cd u = a[i + j], v = a[i + j + len / 2] * w;
                a[i + j] = u + v;
                a[i + j + len / 2] = u - v;
                w *= wl;
            }
        }
    }
    if (inverse) for (int i = 0; i < n; ++i) a[i] /= n;
}

// --- temps (les échelles sont des estimations : voir README) ---
inline float envTime (int i)  { return 0.002f * std::pow (1.33f, (float) (i - 1)); }          // 1..32 -> 2 ms .. ~14 s
inline float relTime (int i)  { return i >= 32 ? 90.f : envTime (i); }                         // 32 = INF
inline float xfTime  (int i)  { return 0.004f * std::pow (1.62f, (float) (i - 1)); }          // 1..16 -> 4 ms .. ~5 s
inline float xfDelay (int i)  { return 0.01f * std::pow (1.6f, (float) (i - 1)); }            // 1..15 -> 10 ms .. ~7 s
inline float lfoHz   (int r)  { return 0.1f * std::pow (1.17f, (float) (r - 1)); }
inline float lfoDly  (int d)  { return d <= 0 ? 0.f : 0.05f * std::pow (1.45f, (float) d); }
inline float portaTime (int s){ return 0.01f * std::pow (1.5f, (float) (s - 1)); }
inline float wheelRange (int v) { return v <= 1 ? 2.f : (v == 2 ? 5.f : 12.f); }

inline float readTable (const float* t, float ph)
{
    const float x = ph * (float) kTableLen;
    const int i = (int) x;
    const float f = x - (float) i;
    return t[i] + (t[i + 1] - t[i]) * f;
}

// avance la machine à états de crossfade d'un oscillateur (OSC1 : base = O1_OCT, OSC2 : base = O2_OCT)
void advanceXf (OscState& o, const int* p, int b, float dt)
{
    switch (o.st)
    {
        case 0: // attente (delay) avant le 1er crossfade
            if (p[b + 2] < 16) { o.t += dt; if (o.t >= xfDelay (p[b + 2])) { o.st = 1; o.m = 0; } }
            break;
        case 1: case 3:
        {
            const int bp = o.st == 1 ? p[b + 4] : p[b + 10];
            const int t1 = o.st == 1 ? p[b + 3] : p[b + 9];
            const int t2 = o.st == 1 ? p[b + 5] : p[b + 11];
            const float f1 = (17 - bp) / 16.f;      // BP=16 -> TIME 1 sans effet ; BP=1 -> TIME 2 sans effet (manuel)
            const float rate = o.m < f1 ? f1 / xfTime (t1) : (1.f - f1) / xfTime (t2);
            o.m += rate * dt;
            if (o.m >= 1.f) { o.m = 1.f; o.t = 0; o.st = (o.st == 1) ? 2 : 4; }
            break;
        }
        case 2: // attente avant le 2e crossfade
            if (p[b + 8] < 16) { o.t += dt; if (o.t >= xfDelay (p[b + 8])) { o.st = 3; o.m = 0; } }
            break;
        default: break;
    }
}

float oscTick (OscState& o, const int* p, int b, const WaveBank& bank, float semis, double sr)
{
    advanceXf (o, p, b, 1.f / (float) sr);
    const float f = 440.f * std::exp2 ((semis - 69.f) / 12.f);
    const float inc = std::min (0.45f, f / (float) sr);
    const float maxH = 0.5f / std::max (inc, 1e-6f);
    int mip = 0;
    while (mip < kMips - 1 && (float) (1024 >> mip) > maxH) ++mip;

    auto clampW = [] (int w) { return std::max (0, std::min (kNumTables - 1, w)); };
    const int w1 = clampW (p[b + 1]), w2 = clampW (p[b + 6]), w3 = clampW (p[b + 7]);
    int wa, wb; float mm;
    if (o.st <= 1)      { wa = w1; wb = w2; mm = o.st == 0 ? 0.f : o.m; }
    else if (o.st <= 3) { wa = w2; wb = w3; mm = o.st == 2 ? 0.f : o.m; }
    else                { wa = wb = w3; mm = 0.f; }

    const float a = readTable (bank.get (wa, mip), o.ph);
    const float c = wa == wb ? a : readTable (bank.get (wb, mip), o.ph);
    o.ph += inc;
    if (o.ph >= 1.f) o.ph -= 1.f;
    return a + (c - a) * mm;
}
} // namespace

// ------------------------------------------------------------------ WaveBank
WaveBank::WaveBank() { data_.assign ((size_t) kNumTables * kMips * (kTableLen + 1), 0.f); }

const float* WaveBank::get (int index, int mip) const
{
    return &data_[((size_t) index * kMips + (size_t) mip) * (kTableLen + 1)];
}

void WaveBank::build (int index, const float* cycle)
{
    std::vector<cd> X (kTableLen), Y (kTableLen);
    double mean = 0;
    for (int i = 0; i < kTableLen; ++i) mean += cycle[i];
    mean /= kTableLen;
    double peak = 1e-9;
    for (int i = 0; i < kTableLen; ++i) peak = std::max (peak, std::fabs (cycle[i] - mean));
    const double g = 0.9 / peak;
    for (int i = 0; i < kTableLen; ++i) X[i] = (cycle[i] - mean) * g;
    fft (X.data(), kTableLen, false);

    for (int m = 0; m < kMips; ++m)
    {
        const int maxH = std::min (1023, 1024 >> m);
        std::fill (Y.begin(), Y.end(), cd (0, 0));
        for (int h = 1; h <= maxH; ++h) { Y[h] = X[h]; Y[kTableLen - h] = X[kTableLen - h]; }
        fft (Y.data(), kTableLen, true);
        float* dst = &data_[((size_t) index * kMips + (size_t) m) * (kTableLen + 1)];
        for (int i = 0; i < kTableLen; ++i) dst[i] = (float) Y[i].real();
        dst[kTableLen] = dst[0];
    }
}

void buildDefaultBank (WaveBank& bank)
{
    std::vector<cd> X (kTableLen);
    std::vector<float> cyc (kTableLen);
    uint32_t rng = 12345u;
    auto rnd = [&rng] { rng = rng * 1664525u + 1013904223u; return (rng >> 8) / 16777216.f; };

    for (int t = 0; t < kNumTables; ++t)
    {
        std::vector<float> amp (1024, 0.f), ph (1024, 0.f);
        switch (t)
        {
            case 0:  amp[1] = 1.f; break;                                                           // sinus
            case 1:  for (int h = 1; h < 1024; ++h) amp[h] = 1.f / h; break;                        // dent de scie
            case 2:  for (int h = 1; h < 1024; h += 2) amp[h] = 1.f / h; break;                     // carré
            case 3:  for (int h = 1; h < 1024; h += 2) { amp[h] = 1.f / (h * h); ph[h] = ((h / 2) % 2) ? (float) kPi : 0.f; } break; // triangle
            case 4:  for (int h = 1; h < 1024; ++h) amp[h] = std::fabs (std::sin ((float) kPi * h * 0.25f)) / h; break;   // impulsion 25 %
            case 5:  for (int h = 1; h < 1024; ++h) amp[h] = std::fabs (std::sin ((float) kPi * h * 0.125f)) / h; break;  // impulsion 12,5 %
            case 6:  { const int hs[] = { 1, 2, 3, 4, 6, 8 }; const float ws[] = { 1.f, .8f, .6f, .5f, .35f, .25f };
                       for (int i = 0; i < 6; ++i) amp[hs[i]] = ws[i]; } break;                    // orgue
            case 7:  { const int hs[] = { 1, 3, 4, 6, 7, 9, 11, 13 };
                       for (int h : hs) amp[h] = 1.f / std::pow ((float) h, 0.7f); } break;        // cloche (approx.)
            case 8:  for (int h = 1; h < 1024; ++h) amp[h] = 1.f / std::pow ((float) h, 1.4f); break; // cordes
            case 9:  for (int h = 1; h < 1024; h += 2) amp[h] = 1.f / std::pow ((float) h, 1.2f); break; // clarinette
            case 10: // voyelle
                for (int h = 1; h < 60; ++h)
                    amp[h] = std::exp (-0.5f * std::pow ((h - 3.f) / 1.2f, 2.f)) + 0.7f * std::exp (-0.5f * std::pow ((h - 9.f) / 1.5f, 2.f));
                break;
            case 11: for (int h = 1; h < 1024; ++h) { amp[h] = 1.f / std::sqrt ((float) h); ph[h] = rnd() * 6.28318f; } break; // bruit bouclé
            default:
            {
                const float tilt = 0.6f + 0.15f * (float) (t % 6);
                const int top = 40 + (t * 37) % 400;
                for (int h = 1; h <= top; ++h)
                {
                    amp[h] = std::pow ((float) h, -tilt) * (0.5f + 0.5f * std::sin (0.37f * h * (float) (t % 7 + 1) + (float) t));
                    ph[h] = std::sin ((float) (h * t)) * 3.f;
                }
            }
        }
        std::fill (X.begin(), X.end(), cd (0, 0));
        for (int h = 1; h < 1024; ++h)
        {
            if (amp[h] == 0.f) continue;
            const cd c = std::polar ((double) amp[h] * kTableLen / 2.0, (double) ph[h] - kPi / 2.0);   // a*sin(h*theta + phi)
            X[h] = c;
            X[kTableLen - h] = std::conj (c);
        }
        fft (X.data(), kTableLen, true);
        for (int i = 0; i < kTableLen; ++i) cyc[i] = (float) X[i].real();
        bank.build (t, cyc.data());
    }
}

// ----------------------------------------------------------------------- Env
void Env::start (float atk, float dec, float slp, float brk_, float sus_, double sr)
{
    brk = brk_; sus = sus_; v = 0; st = Atk;
    aR = 1.f / (std::max (atk, 1e-4f) * (float) sr);
    dR = (1.f - brk) / (std::max (dec, 1e-4f) * (float) sr);
    sR = std::fabs (sus - brk) / (std::max (slp, 1e-4f) * (float) sr);
}

void Env::release (float sec, double sr)
{
    if (st == Off) return;
    st = Rel;
    rR = std::max (v, 1e-4f) / (std::max (sec, 1e-3f) * (float) sr);
}

float Env::tick()
{
    switch (st)
    {
        case Atk: v += aR; if (v >= 1.f) { v = 1.f; st = Dec; } break;
        case Dec: if (v <= brk) st = Slp; else { v -= dR; if (v <= brk) { v = brk; st = Slp; } } break;
        case Slp:
            if (v > sus)      { v -= sR; if (v <= sus) { v = sus; st = Sus; } }
            else if (v < sus) { v += sR; if (v >= sus) { v = sus; st = Sus; } }
            else st = Sus;
            break;
        case Rel: v -= rR; if (v <= 0.f) { v = 0.f; st = Off; } break;
        default: break;
    }
    return v;
}

// ----------------------------------------------------------------------- Lfo
float Lfo::tick (int wave, float hz, float dt, float delaySec, float lvlInit, float lvlFinal)
{
    t += dt;
    const float frac = delaySec > 0.f ? std::min (1.f, t / delaySec) : 1.f;
    const float lvl = (lvlInit + (lvlFinal - lvlInit) * frac) / 15.f;
    ph += hz * dt;
    bool wrapped = false;
    if (ph >= 1.f) { ph -= 1.f; wrapped = true; }
    float w = 0.f;
    switch (wave)
    {
        case 0:  w = ph < 0.5f ? 4.f * ph - 1.f : 3.f - 4.f * ph; break;
        case 1:  w = 2.f * ph - 1.f; break;
        case 2:  w = ph < 0.5f ? 1.f : -1.f; break;
        default:
            if (wrapped || t <= dt) { rng = rng * 1664525u + 1013904223u; held = (float) (rng >> 8) / 16777216.f * 2.f - 1.f; }
            w = held; break;
    }
    return w * lvl;
}

// --------------------------------------------------------------------- Voice
void Voice::start (const int* params, int note_, int zone_, float vel_, float startNote, double sr, uint64_t age_)
{
    std::copy (params, params + NUM_PARAMS, p);
    note = note_; zone = zone_; vel = vel_; age = age_;
    keyDown = true; sustained = false; active = true;
    curNote = p[K_PORT] ? startNote : (float) note_;
    for (auto& os : o) os = OscState{};
    for (int i = 0; i < 3; ++i) { l[i] = Lfo{}; l[i].rng = 7919u * (uint32_t) (i + 1) + (uint32_t) (age_ * 2654435761u); }
    for (auto& x : s) x = 0.f;

    a1.start (envTime (p[A1_ATK]), envTime (p[A1_DEC]), envTime (p[A1_SLP]), p[A1_BRK] / 63.f, p[A1_SUS] / 63.f, sr);
    osc2On = vel * 16.f >= (float) p[V_A2T];
    if (osc2On) a2.start (envTime (p[A2_ATK]), envTime (p[A2_DEC]), envTime (p[A2_SLP]), p[A2_BRK] / 63.f, p[A2_SUS] / 63.f, sr);
    else { a2.st = Env::Off; a2.v = 0.f; }
    fe.start (envTime (p[FD_ATK]), envTime (p[FD_DEC]), envTime (p[FD_SLP]), p[FD_BRK] / 64.f, p[FD_SUS] / 64.f, sr);
    pe1.start (xfTime (p[T1_ATK]), xfTime (p[T1_DEC]), 0.001f, 0.f, 0.f, sr);
    pe2.start (xfTime (p[T2_ATK]), xfTime (p[T2_DEC]), 0.001f, 0.f, 0.f, sr);

    const float tg = p[K_PORT] ? portaTime (p[K_PSPD]) : 0.f;
    glide = tg > 0.f ? 1.f - std::exp (-1.f / (tg * (float) sr * 0.3f)) : 1.f;
}

void Voice::release (double sr)
{
    keyDown = false; sustained = false;
    a1.release (relTime (p[A1_REL]), sr);
    a2.release (relTime (p[A2_REL]), sr);
    fe.release (relTime (p[FD_REL]), sr);
    pe1.release (0.05f, sr);
    pe2.release (0.05f, sr);
}

void Voice::releaseFast (double sr)
{
    keyDown = false; sustained = false;
    a1.release (0.015f, sr); a2.release (0.015f, sr); fe.release (0.015f, sr);
    pe1.release (0.015f, sr); pe2.release (0.015f, sr);
}

void Voice::render (float* L, float* R, int n, const WaveBank& bank, double sr, float pitchBend, float modWheel)
{
    const float dt = 1.f / (float) sr;
    const float lv1 = p[LV_O1] / 3.f, lv2 = p[LV_O2] / 3.f;
    const float pbR1 = wheelRange (p[W_P1]), pbR2 = wheelRange (p[W_P2]);
    const float zoneLvl = p[LEVEL] / 31.f;
    const float pan01 = (p[PAN] + 7) / 14.f;
    const float gl = std::cos (pan01 * 1.5707963f) * 0.35f * zoneLvl;
    const float gr = std::sin (pan01 * 1.5707963f) * 0.35f * zoneLvl;
    const float initA1 = (p[V_A1] - 1) / 31.f, initA2 = (p[V_A2] - 1) / 31.f, initF = (p[V_F] - 1) / 31.f;
    const float velA1 = initA1 + (1.f - initA1) * vel;
    const float velA2 = initA2 + (1.f - initA2) * vel;
    const float dynF  = initF + (1.f - initF) * vel;
    const float k = p[FL_RES] / 31.f * 4.0f;
    const float lfoHz1 = lfoHz (p[L1_RATE]), lfoHz2 = lfoHz (p[L2_RATE]), lfoHz3 = lfoHz (p[L3_RATE]);
    const float dly1 = lfoDly (p[L1_DLY]), dly2 = lfoDly (p[L2_DLY]), dly3 = lfoDly (p[L3_DLY]);
    const int dest = p[L3_DEST];

    for (int i = 0; i < n; ++i)
    {
        curNote += ((float) note - curNote) * glide;
        const float pe1v = pe1.tick(), pe2v = pe2.tick();

        const float m1 = p[W_M1] ? modWheel : 1.f, m2 = p[W_M2] ? modWheel : 1.f, m3 = p[W_M3] ? modWheel : 1.f;
        const float lf1 = l[0].tick (p[L1_WAVE], lfoHz1, dt, dly1, (float) p[L1_INIT], (float) p[L1_FIN]) * m1;
        const float lf2 = l[1].tick (p[L2_WAVE], lfoHz2, dt, dly2, (float) p[L2_INIT], (float) p[L2_FIN]) * m2;
        const float lf3 = l[2].tick (p[L3_WAVE], lfoHz3, dt, dly3, (float) p[L3_INIT], (float) p[L3_FIN]) * m3;

        const float base = curNote + (float) p[TRANSP];
        const float s1 = base + (float) (p[O1_OCT] - 2) * 12.f + (float) p[T1_SEMI] + p[T1_FINE] * 0.02f
                       + (float) p[T1_EGLVL] * pe1v + pitchBend * pbR1 + lf1 * 2.f;
        const float s2 = base + (float) (p[O2_OCT] - 2) * 12.f + (float) p[T2_SEMI] + p[T2_FINE] * 0.02f
                       + (float) p[T2_EGLVL] * pe2v + pitchBend * pbR2 + lf2 * 2.f;

        const float o1 = oscTick (o[0], p, O1_OCT, bank, s1, sr);
        const float o2 = oscTick (o[1], p, O2_OCT, bank, s2, sr);
        const float e1 = a1.tick(), e2 = a2.tick();
        const float trem = dest != 1 ? std::max (0.f, 1.f + 0.5f * lf3) : 1.f;
        const float mix = (o1 * lv1 * e1 * velA1 + o2 * lv2 * e2 * velA2) * trem;

        // --- filtre passe-bas résonant 4 pôles 24 dB/oct (modèle type CEM 3389, saturation tanh) ---
        const float fv = fe.tick();
        float cutU = (float) p[FL_CUT] * (0.5f + 0.5f * dynF) + (float) p[FL_EG] * fv * dynF
                   + (float) p[FL_KBD] * 0.125f * (curNote - 60.f) * 1.0667f
                   + (dest != 0 ? lf3 * 40.f : 0.f);
        cutU = std::max (0.f, std::min (128.f, cutU));
        const float fc = std::min (20.f * std::exp2 (cutU / 12.8f), 0.45f * (float) sr);
        const float wc = 2.f * 3.1415927f * fc / (2.f * (float) sr);
        const float g = wc / (1.f + wc);
        for (int sub = 0; sub < 2; ++sub)
        {
            const float u = std::tanh (mix * (1.f + 0.3f * k) - k * s[3]);
            s[0] += g * (u - s[0]);
            s[1] += g * (s[0] - s[1]);
            s[2] += g * (s[1] - s[2]);
            s[3] += g * (s[2] - s[3]);
        }
        L[i] += s[3] * gl;
        R[i] += s[3] * gr;

        if (a1.st == Env::Off && a2.st == Env::Off) { active = false; break; }
    }
}

// -------------------------------------------------------------------- Engine
Engine::Engine()
{
    for (int z = 0; z < kZones; ++z)
    {
        lastNote_[z] = -1;
        for (int i = 0; i < NUM_PARAMS; ++i) params_[z][i].store (kDefs[i].def);
    }
    buildDefaultBank (bank);
}

void Engine::prepare (double sampleRate) { sr_ = sampleRate; allNotesOff(); }

void Engine::setParam (int zone, int id, int value)
{
    value = std::max (kDefs[id].mn, std::min (kDefs[id].mx, value));
    params_[zone][id].store (value, std::memory_order_relaxed);
}

int Engine::zoneOfNote (int note)
{
    return std::max (0, std::min (kZones - 1, (note - 36) / 12 - (note < 36 ? 1 : 0)));
}

Voice* Engine::allocVoice()
{
    for (auto& v : voices_) if (! v.active) return &v;
    Voice* best = nullptr;
    for (auto& v : voices_)
        if (! v.keyDown && (best == nullptr || v.age < best->age)) best = &v;
    if (best == nullptr)
        for (auto& v : voices_) if (best == nullptr || v.age < best->age) best = &v;
    return best;
}

void Engine::noteOn (int note, float velocity)
{
    const int z = zoneOfNote (note);
    int p[NUM_PARAMS];
    for (int i = 0; i < NUM_PARAMS; ++i) p[i] = getParam (z, i);

    float startNote = (float) note;
    if (p[K_MONO])
        for (auto& v : voices_) if (v.active && v.zone == z) v.releaseFast (sr_);
    if (p[K_PORT] && lastNote_[z] >= 0) startNote = (float) lastNote_[z];
    lastNote_[z] = note;

    if (Voice* v = allocVoice())
        v->start (p, note, z, std::max (0.f, std::min (1.f, velocity)), startNote, sr_, ++counter_);
}

void Engine::noteOff (int note)
{
    for (auto& v : voices_)
        if (v.active && v.keyDown && v.note == note)
        {
            if (sustain_) { v.keyDown = false; v.sustained = true; }
            else v.release (sr_);
        }
}

void Engine::setSustain (bool on)
{
    sustain_ = on;
    if (! on) for (auto& v : voices_) if (v.active && v.sustained) v.release (sr_);
}

void Engine::allNotesOff() { for (auto& v : voices_) { v.active = false; v.keyDown = false; v.sustained = false; } }

void Engine::render (float* left, float* right, int numSamples)
{
    std::fill (left, left + numSamples, 0.f);
    std::fill (right, right + numSamples, 0.f);
    const float pb = pitchBend_.load(), mw = modWheel_.load();
    for (auto& v : voices_)
        if (v.active) v.render (left, right, numSamples, bank, sr_, pb, mw);
}
} // namespace cts
