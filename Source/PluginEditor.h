#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

class CTS2000Editor : public juce::AudioProcessorEditor,
                      private juce::MidiKeyboardState::Listener
{
public:
    explicit CTS2000Editor (CTS2000Processor&);
    ~CTS2000Editor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    enum class Mode { Standard, Edit };

    // MidiKeyboardState::Listener
    void handleNoteOn (juce::MidiKeyboardState*, int, int note, float) override;
    void handleNoteOff (juce::MidiKeyboardState*, int, int, float) override {}

    void setLcd (const juce::String& a, const juce::String& b);
    void refreshLcd();
    void showStandard();
    void showFamily();
    void flash (const juce::String& a, const juce::String& b);

    void toggleMode();
    void pressFamily (int index);
    void selectPreset (int n);
    void stepFine (int dir);
    void onEnter();
    void onWrite();
    void onInit();
    void updateSliders();
    void sliderMoved (int j);
    void renamePreset();
    void saveBank();
    void loadBank();
    void zoneChosenByKey (int note);

    CTS2000Processor& proc;
    juce::LookAndFeel_V4 lnf;

    Mode mode = Mode::Standard;
    bool globalEdit = false, bankB = false, modified = false, writePending = false, initPending = false, inUpdate = false;
    int family = -1, zone = 0, lastSlider = 0;
    int oldVal[cts::kSliders] = {};
    int baseVal[cts::kZones][cts::kSliders] = {};

    juce::String lcd1, lcd2;
    juce::Rectangle<int> lcdRect;

    juce::Slider slid[cts::kSliders];
    juce::Label slidName[cts::kSliders];
    juce::TextButton famBtn[cts::kFamilies];
    juce::TextButton internalBtn { "INTERNAL" }, cartridgeBtn { "CARTRIDGE" }, bankBtn { "BANK A" },
                     modeBtn { "PRESET/\nPARAM." }, zoneBtn { "ZONE/\nGLOBAL" },
                     initBtn { "INITIALIZE\nPRESET" }, writeBtn { "WRITE" }, enterBtn { "ENTER" },
                     downBtn { "DOWN" }, upBtn { "UP" },
                     saveBtn { "SAVE BANK..." }, loadBtn { "LOAD BANK..." };
    juce::Slider pitchWheel, modWheel, volume;
    juce::Label volLabel { {}, "VOLUME" }, pitchLabel { {}, "PITCH" }, modLabel { {}, "MOD" };
    std::unique_ptr<juce::SliderParameterAttachment> volAttach;
    juce::MidiKeyboardComponent keyboard;
    std::unique_ptr<juce::FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CTS2000Editor)
};
