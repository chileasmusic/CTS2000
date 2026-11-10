#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace cts;

namespace
{
juce::String serializeInts (const std::vector<int>& v)
{
    juce::StringArray sa;
    for (int x : v) sa.add (juce::String (x));
    return sa.joinIntoString (" ");
}

bool parseInts (const juce::String& s, std::vector<int>& out)
{
    auto toks = juce::StringArray::fromTokens (s, " ", "");
    toks.removeEmptyStrings();
    if ((size_t) toks.size() != (size_t) kZones * NUM_PARAMS) return false;
    out.clear();
    for (auto& t : toks) out.push_back (t.getIntValue());
    return true;
}
} // namespace

CTS2000Processor::CTS2000Processor()
    : juce::AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    addParameter (masterVol = new juce::AudioParameterFloat (juce::ParameterID { "master", 1 }, "Master Volume",
                                                              juce::NormalisableRange<float> (0.f, 1.f), 0.75f));
    loadUserTables();
    for (auto& set : sets)
        for (auto& p : set) p = makeInit();
    makeDemoPresets();
    recallPreset (0, 0);
}

juce::File CTS2000Processor::tablesFolder()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("CTS2000Tables");
}

// Place des fichiers 00.wav .. 63.wav (un cycle mono) dans ce dossier pour remplacer les tables de substitution.
void CTS2000Processor::loadUserTables()
{
    const auto dir = tablesFolder();
    dir.createDirectory();
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();

    for (int t = 0; t < kNumTables; ++t)
    {
        const auto f = dir.getChildFile (juce::String (t).paddedLeft ('0', 2) + ".wav");
        if (! f.existsAsFile()) continue;
        std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (f));
        if (r == nullptr || r->lengthInSamples < 16) continue;

        const int len = (int) std::min<juce::int64> (r->lengthInSamples, 1 << 22);
        juce::AudioBuffer<float> b (1, len);
        r->read (&b, 0, len, 0, true, false);
        const float* src = b.getReadPointer (0);

        std::vector<float> cyc (kTableLen);
        for (int i = 0; i < kTableLen; ++i)
        {
            const double pos = (double) i * len / kTableLen;
            const int i0 = (int) pos;
            const float fr = (float) (pos - i0);
            cyc[(size_t) i] = src[i0] + (src[std::min (i0 + 1, len - 1)] - src[i0]) * fr;
        }
        engine.bank.build (t, cyc.data());
    }
}

// ------------------------------------------------------------------ presets
CTS2000Processor::Patch CTS2000Processor::makeInit() const
{
    Patch p;
    p.name = "INIT";
    p.v.resize ((size_t) kZones * NUM_PARAMS);
    for (int z = 0; z < kZones; ++z)
        for (int i = 0; i < NUM_PARAMS; ++i) p.v[(size_t) z * NUM_PARAMS + i] = kDefs[i].def;
    return p;
}

CTS2000Processor::Patch CTS2000Processor::capture (const juce::String& name) const
{
    Patch p;
    p.name = name;
    p.v.resize ((size_t) kZones * NUM_PARAMS);
    for (int z = 0; z < kZones; ++z)
        for (int i = 0; i < NUM_PARAMS; ++i) p.v[(size_t) z * NUM_PARAMS + i] = engine.getParam (z, i);
    return p;
}

void CTS2000Processor::apply (const Patch& p)
{
    if (p.v.size() != (size_t) kZones * NUM_PARAMS) return;
    for (int z = 0; z < kZones; ++z)
        for (int i = 0; i < NUM_PARAMS; ++i) engine.setParam (z, i, p.v[(size_t) z * NUM_PARAMS + i]);
    curName = p.name;
}

void CTS2000Processor::recallPreset (int set, int index)
{
    curSet = juce::jlimit (0, 1, set);
    curIndex = juce::jlimit (0, kPresetsPerSet - 1, index);
    apply (sets[curSet][curIndex]);
}

void CTS2000Processor::storeCurrent (int set, int index)
{
    sets[juce::jlimit (0, 1, set)][juce::jlimit (0, kPresetsPerSet - 1, index)] = capture (curName);
}

void CTS2000Processor::initCurrent()
{
    apply (makeInit());
    curName = "";
}

// Quelques patchs de démonstration ORIGINAUX (ce ne sont PAS les presets d'usine du CTS-2000).
void CTS2000Processor::makeDemoPresets()
{
    auto setAll = [] (Patch& p, int id, int val) { for (int z = 0; z < kZones; ++z) p.v[(size_t) z * NUM_PARAMS + id] = val; };
    {
        auto& p = sets[0][0]; p.name = "DEMO SAW PAD";
        setAll (p, O2_W1, 1); setAll (p, T2_FINE, 5); setAll (p, T1_FINE, -5);
        setAll (p, A1_ATK, 14); setAll (p, A2_ATK, 14); setAll (p, A1_REL, 18); setAll (p, A2_REL, 18);
        setAll (p, FL_CUT, 80); setAll (p, FL_RES, 6); setAll (p, FL_EG, 25); setAll (p, FD_ATK, 14); setAll (p, FD_DEC, 20);
        setAll (p, FD_BRK, 40); setAll (p, FD_SUS, 40); setAll (p, FD_REL, 18);
        setAll (p, L1_FIN, 2); setAll (p, L1_RATE, 14); setAll (p, L1_DLY, 8); setAll (p, L2_FIN, 2); setAll (p, L2_RATE, 13); setAll (p, L2_DLY, 8);
    }
    {
        auto& p = sets[0][1]; p.name = "DEMO SQUARE BASS";
        setAll (p, O1_W1, 2); setAll (p, O2_W1, 1); setAll (p, O1_OCT, 1); setAll (p, O2_OCT, 1);
        setAll (p, K_MONO, 1); setAll (p, FL_CUT, 40); setAll (p, FL_RES, 8); setAll (p, FL_EG, 40);
        setAll (p, FD_DEC, 10); setAll (p, FD_BRK, 8); setAll (p, FD_SUS, 8); setAll (p, A1_REL, 8); setAll (p, A2_REL, 8);
    }
    {
        auto& p = sets[0][2]; p.name = "DEMO CROSS SAW>SQ";
        setAll (p, O1_W1, 1); setAll (p, O1_W2, 2); setAll (p, O1_W3, 3); setAll (p, O1_DLY, 4); setAll (p, O1_T1, 8); setAll (p, O1_BP, 8);
        setAll (p, O1_DLY2, 9); setAll (p, O1_T1B, 9); setAll (p, O1_BPB, 8);
        setAll (p, O2_W1, 0); setAll (p, O2_W2, 7); setAll (p, O2_DLY, 5); setAll (p, O2_T1, 10);
        setAll (p, FL_CUT, 100); setAll (p, A1_REL, 14); setAll (p, A2_REL, 14);
    }
    {
        auto& p = sets[0][3]; p.name = "DEMO PLUCK";
        setAll (p, O1_W1, 8); setAll (p, O2_W1, 4); setAll (p, A1_DEC, 12); setAll (p, A1_BRK, 8); setAll (p, A1_SUS, 8);
        setAll (p, A2_DEC, 12); setAll (p, A2_BRK, 8); setAll (p, A2_SUS, 8);
        setAll (p, FL_CUT, 50); setAll (p, FL_EG, 50); setAll (p, FD_DEC, 9); setAll (p, FD_BRK, 6); setAll (p, FD_SUS, 6);
        setAll (p, V_F, 6); setAll (p, V_A1, 4);
    }
    {
        auto& p = sets[0][4]; p.name = "DEMO ORGAN";
        setAll (p, O1_W1, 6); setAll (p, O2_W1, 6); setAll (p, O2_OCT, 3); setAll (p, T2_FINE, 3);
        setAll (p, A1_REL, 4); setAll (p, A2_REL, 4); setAll (p, FL_CUT, 120);
    }
}

// ----------------------------------------------------------------- audio
void CTS2000Processor::prepareToPlay (double sampleRate, int) { engine.prepare (sampleRate); }

bool CTS2000Processor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void CTS2000Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    keyState.processNextMidiBuffer (midi, 0, n, true);

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : buffer.getWritePointer (0);
    int pos = 0;

    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        const int t = juce::jlimit (0, n, meta.samplePosition);
        if (t > pos) { engine.render (L + pos, R + pos, t - pos); pos = t; }

        if (m.isNoteOn())            engine.noteOn (m.getNoteNumber(), m.getFloatVelocity());
        else if (m.isNoteOff())      engine.noteOff (m.getNoteNumber());
        else if (m.isPitchWheel())   engine.setPitchBend ((m.getPitchWheelValue() - 8192) / 8192.f);
        else if (m.isController())
        {
            const int cc = m.getControllerNumber(), val = m.getControllerValue();
            if (cc == 1)                    engine.setModWheel (val / 127.f);
            else if (cc == 64)              engine.setSustain (val >= 64);
            else if (cc == 120 || cc == 123) engine.allNotesOff();
        }
    }
    if (pos < n) engine.render (L + pos, R + pos, n - pos);

    buffer.applyGain (masterVol->get());
}

juce::AudioProcessorEditor* CTS2000Processor::createEditor() { return new CTS2000Editor (*this); }

// ---------------------------------------------------------- état / fichiers
std::unique_ptr<juce::XmlElement> CTS2000Processor::toXml() const
{
    auto root = std::make_unique<juce::XmlElement> ("CTS2000");
    root->setAttribute ("set", curSet);
    root->setAttribute ("idx", curIndex);
    root->setAttribute ("vol", (double) masterVol->get());
    auto* live = root->createNewChildElement ("LIVE");
    const auto cur = capture (curName);
    live->setAttribute ("name", cur.name);
    live->setAttribute ("data", serializeInts (cur.v));
    for (int s = 0; s < 2; ++s)
        for (int i = 0; i < kPresetsPerSet; ++i)
        {
            auto* e = root->createNewChildElement ("P");
            e->setAttribute ("s", s);
            e->setAttribute ("i", i);
            e->setAttribute ("name", sets[s][i].name);
            e->setAttribute ("data", serializeInts (sets[s][i].v));
        }
    return root;
}

void CTS2000Processor::fromXml (const juce::XmlElement& root)
{
    if (! root.hasTagName ("CTS2000")) return;
    for (auto* e : root.getChildWithTagNameIterator ("P"))
    {
        const int s = e->getIntAttribute ("s"), i = e->getIntAttribute ("i");
        if (s < 0 || s > 1 || i < 0 || i >= kPresetsPerSet) continue;
        std::vector<int> v;
        if (! parseInts (e->getStringAttribute ("data"), v)) continue;
        sets[s][i].name = e->getStringAttribute ("name");
        sets[s][i].v = std::move (v);
    }
}

void CTS2000Processor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = toXml()) copyXmlToBinary (*xml, dest);
}

void CTS2000Processor::setStateInformation (const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, size));
    if (xml == nullptr || ! xml->hasTagName ("CTS2000")) return;
    fromXml (*xml);
    curSet = juce::jlimit (0, 1, xml->getIntAttribute ("set"));
    curIndex = juce::jlimit (0, kPresetsPerSet - 1, xml->getIntAttribute ("idx"));
    *masterVol = (float) xml->getDoubleAttribute ("vol", 0.75);
    if (auto* live = xml->getChildByName ("LIVE"))
    {
        Patch p;
        if (parseInts (live->getStringAttribute ("data"), p.v)) { p.name = live->getStringAttribute ("name"); apply (p); }
    }
}

bool CTS2000Processor::saveBank (const juce::File& f) const
{
    auto xml = toXml();
    return xml != nullptr && xml->writeTo (f);
}

bool CTS2000Processor::loadBank (const juce::File& f)
{
    std::unique_ptr<juce::XmlElement> xml (juce::XmlDocument::parse (f));
    if (xml == nullptr || ! xml->hasTagName ("CTS2000")) return false;
    fromXml (*xml);
    recallPreset (curSet, curIndex);
    return true;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new CTS2000Processor(); }
