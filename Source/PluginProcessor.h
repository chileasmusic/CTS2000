#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "Engine.h"

class CTS2000Processor : public juce::AudioProcessor
{
public:
    struct Patch
    {
        juce::String name { "INIT" };
        std::vector<int> v;   // kZones * NUM_PARAMS valeurs : v[zone * NUM_PARAMS + id]
    };

    CTS2000Processor();
    ~CTS2000Processor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // --- presets (2 jeux de 48 : INTERNAL et CARTRIDGE ; banques A = 0..23, B = 24..47) ---
    Patch makeInit() const;
    Patch capture (const juce::String& name) const;
    void apply (const Patch&);
    void recallPreset (int set, int index);
    void storeCurrent (int set, int index);
    void initCurrent();
    bool saveBank (const juce::File&) const;
    bool loadBank (const juce::File&);
    static juce::File tablesFolder();

    cts::Engine engine;
    juce::MidiKeyboardState keyState;
    juce::AudioParameterFloat* masterVol = nullptr;

    Patch sets[2][cts::kPresetsPerSet];
    int curSet = 0, curIndex = 0;
    juce::String curName { "INIT" };
    bool writeEnabled = false;

private:
    std::unique_ptr<juce::XmlElement> toXml() const;
    void fromXml (const juce::XmlElement&);
    void loadUserTables();
    void makeDemoPresets();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CTS2000Processor)
};
