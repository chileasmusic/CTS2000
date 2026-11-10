// Test du moteur sans JUCE : g++ -std=c++17 -O2 -I../Source engine_test.cpp ../Source/Engine.cpp
#include "Engine.h"
#include <cstdio>
#include <cmath>
using namespace cts;

static bool render(Engine& e, int total, float& peak, bool& finite, double* rms = nullptr)
{
    std::vector<float> L(512), R(512);
    peak = 0; finite = true; double acc = 0; long n = 0;
    for (int done = 0; done < total; done += 512) {
        e.render(L.data(), R.data(), 512);
        for (int i = 0; i < 512; ++i) { float s = L[i] + R[i]; if (!std::isfinite(s)) finite = false; peak = std::max(peak, std::fabs(s)); acc += s*s; ++n; }
    }
    if (rms) *rms = std::sqrt(acc / n);
    return finite;
}

int main()
{
    int fails = 0;
    auto check = [&](bool ok, const char* what) { std::printf("%s  %s\n", ok ? "OK  " : "FAIL", what); if (!ok) ++fails; };

    { Engine e; e.prepare(48000); e.noteOn(60, 0.8f); float pk; bool fin; double r;
      render(e, 48000, pk, fin, &r);
      check(fin && pk > 0.05f && pk < 1.5f, "INIT patch : note audible, finie, pas de saturation");
      std::printf("      pic=%.3f rms=%.3f\n", pk, r); }

    { Engine e; e.prepare(48000); e.noteOn(60, 1.f); e.noteOff(60);
      float pk; bool fin; render(e, 48000 * 2, pk, fin);
      std::vector<float> L(512), R(512); e.render(L.data(), R.data(), 512);
      float tail = 0; for (float x : L) tail = std::max(tail, std::fabs(x));
      check(fin && tail < 1e-4f, "release : la voix se termine, silence après"); }

    { Engine e; e.prepare(44100);
      for (int z = 0; z < kZones; ++z) { e.setParam(z, FL_RES, 31); e.setParam(z, FL_CUT, 70); e.setParam(z, FL_EG, 60); }
      for (int n = 36; n < 96; n += 5) e.noteOn(n, 1.f);
      float pk; bool fin; render(e, 44100 * 2, pk, fin);
      check(fin && pk < 20.f, "résonance max + 8 voix : stable (pas de NaN/explosion)"); std::printf("      pic=%.3f\n", pk); }

    { Engine e; e.prepare(48000);
      for (int z = 0; z < kZones; ++z) { e.setParam(z, O1_W1, 1); e.setParam(z, O1_W2, 2); e.setParam(z, O1_W3, 3);
          e.setParam(z, O1_DLY, 2); e.setParam(z, O1_T1, 5); e.setParam(z, O1_BP, 16); e.setParam(z, O1_DLY2, 16); e.setParam(z, LV_O2, 0); }
      e.noteOn(60, 1.f);
      std::vector<float> a(512), b(512);
      double early = 0, late = 0;
      for (int blk = 0; blk < 400; ++blk) { e.render(a.data(), b.data(), 512);
          if (blk == 2)   for (float x : a) early += x * x;
          if (blk == 390) for (float x : a) late += x * x; }
      check(std::fabs(early - late) / (early + 1e-9) > 0.05, "crossfade scie -> carré : le timbre évolue dans le temps");
      std::printf("      énergie début=%.2f fin=%.2f\n", early, late); }

    { Engine e; e.prepare(48000);
      for (int z = 0; z < kZones; ++z) e.setParam(z, K_MONO, 1);
      e.noteOn(60, 1.f); e.noteOn(64, 1.f); e.noteOn(67, 1.f);
      float pk; bool fin; render(e, 4096, pk, fin);
      check(fin, "mode monophonique : OK"); }

    { Engine e; e.prepare(48000);
      for (int z = 0; z < kZones; ++z) e.setParam(z, V_A2T, 16);
      e.noteOn(60, 0.2f); float pk; bool fin; render(e, 4096, pk, fin);
      check(fin && pk > 0, "seuil DCA2 (vélocité) : OK"); }

    { Engine e; e.prepare(48000); e.setParam(0, PAN, -7); e.setParam(0, LEVEL, 31); e.noteOn(40, 1.f);
      std::vector<float> L(2048), R(2048); e.render(L.data(), R.data(), 2048);
      double l = 0, r = 0; for (int i = 0; i < 2048; ++i) { l += L[i]*L[i]; r += R[i]*R[i]; }
      check(l > 20 * r, "pan 7L : signal à gauche"); }

    check(Engine::zoneOfNote(36) == 0 && Engine::zoneOfNote(47) == 0 && Engine::zoneOfNote(48) == 1 && Engine::zoneOfNote(96) == 4 && Engine::zoneOfNote(20) == 0, "zones : C=36..B=47 -> zone 0, 48 -> zone 1, 96 -> zone 4");
    check(NUM_PARAMS > 80, "nombre de paramètres par zone");
    std::printf("NUM_PARAMS=%d\n", (int) NUM_PARAMS);
    std::printf(fails ? "\n%d ECHEC(S)\n" : "\nTOUS LES TESTS PASSENT\n", fails);
    return fails;
}
