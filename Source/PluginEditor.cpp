#include "PluginEditor.h"

using namespace cts;

namespace
{
juce::String line16 (const juce::String& s) { return s.substring (0, 16); }
}

CTS2000Editor::CTS2000Editor (CTS2000Processor& p)
    : juce::AudioProcessorEditor (&p), proc (p),
      keyboard (p.keyState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    lnf.setColourScheme (juce::LookAndFeel_V4::getMidnightColourScheme());
    lnf.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff3a3f46));
    lnf.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff6fbf4a));
    lnf.setColour (juce::TextButton::textColourOnId, juce::Colours::black);
    setLookAndFeel (&lnf);

    // ---- 6 sliders de DATA ENTRY (A..F) : cliquer-glisser, ou double-clic sur la valeur pour la taper ----
    for (int j = 0; j < kSliders; ++j)
    {
        auto& s = slid[j];
        s.setSliderStyle (juce::Slider::LinearVertical);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 58, 18);
        s.setRange (0, 1, 1);
        s.setEnabled (false);
        s.onValueChange = [this, j] { sliderMoved (j); };
        addAndMakeVisible (s);

        slidName[j].setJustificationType (juce::Justification::centred);
        slidName[j].setFont (juce::Font (10.f));
        addAndMakeVisible (slidName[j]);
    }

    // ---- 24 boutons de familles / presets ----
    for (int i = 0; i < kFamilies; ++i)
    {
        famBtn[i].setButtonText (juce::String (i + 1) + "\n" + kFam[i].label);
        famBtn[i].onClick = [this, i] { pressFamily (i); };
        addAndMakeVisible (famBtn[i]);
    }

    internalBtn.onClick  = [this] { proc.recallPreset (0, proc.curIndex); mode = Mode::Standard; family = -1; modified = false; writePending = initPending = false; updateSliders(); refreshLcd(); };
    cartridgeBtn.onClick = [this] { proc.recallPreset (1, proc.curIndex); mode = Mode::Standard; family = -1; modified = false; writePending = initPending = false; updateSliders(); refreshLcd(); };
    bankBtn.onClick = [this]
    {
        bankB = ! bankB;
        bankBtn.setButtonText (bankB ? "BANK B" : "BANK A");
        proc.recallPreset (proc.curSet, (bankB ? kBankSize : 0) + proc.curIndex % kBankSize);
        modified = false;
        refreshLcd();
    };
    modeBtn.onClick = [this] { toggleMode(); };
    zoneBtn.onClick = [this]
    {
        globalEdit = ! globalEdit;
        zoneBtn.setToggleState (globalEdit, juce::dontSendNotification);
        if (mode == Mode::Edit) { updateSliders(); refreshLcd(); }
    };
    initBtn.onClick  = [this] { onInit(); };
    writeBtn.onClick = [this] { onWrite(); };
    enterBtn.onClick = [this] { onEnter(); };
    downBtn.onClick  = [this] { stepFine (-1); };
    upBtn.onClick    = [this] { stepFine (+1); };
    saveBtn.onClick  = [this] { saveBank(); };
    loadBtn.onClick  = [this] { loadBank(); };
    for (auto* b : { &internalBtn, &cartridgeBtn, &bankBtn, &modeBtn, &zoneBtn, &initBtn, &writeBtn, &enterBtn, &downBtn, &upBtn, &saveBtn, &loadBtn })
        addAndMakeVisible (*b);

    // ---- molettes, volume, clavier ----
    pitchWheel.setSliderStyle (juce::Slider::LinearVertical);
    pitchWheel.setRange (-1.0, 1.0, 0.0);
    pitchWheel.setValue (0.0, juce::dontSendNotification);
    pitchWheel.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    pitchWheel.onValueChange = [this] { proc.engine.setPitchBend ((float) pitchWheel.getValue()); };
    pitchWheel.onDragEnd = [this] { pitchWheel.setValue (0.0, juce::sendNotificationSync); };   // retour au centre
    modWheel.setSliderStyle (juce::Slider::LinearVertical);
    modWheel.setRange (0.0, 1.0, 0.0);
    modWheel.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    modWheel.onValueChange = [this] { proc.engine.setModWheel ((float) modWheel.getValue()); };
    addAndMakeVisible (pitchWheel);
    addAndMakeVisible (modWheel);

    volume.setSliderStyle (juce::Slider::LinearHorizontal);
    volume.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    volAttach = std::make_unique<juce::SliderParameterAttachment> (*proc.masterVol, volume, nullptr);
    addAndMakeVisible (volume);

    for (auto* l : { &volLabel, &pitchLabel, &modLabel })
    {
        l->setJustificationType (juce::Justification::centred);
        l->setFont (juce::Font (11.f));
        addAndMakeVisible (*l);
    }

    keyboard.setAvailableRange (36, 96);
    keyboard.setKeyWidth (32.f);
    keyboard.setLowestVisibleKey (36);
    keyboard.setScrollButtonsVisible (false);
    keyboard.setWantsKeyboardFocus (false);
    addAndMakeVisible (keyboard);
    proc.keyState.addListener (this);

    bankB = proc.curIndex >= kBankSize;
    bankBtn.setButtonText (bankB ? "BANK B" : "BANK A");
    setSize (1330, 520);
    updateSliders();
    refreshLcd();
}

CTS2000Editor::~CTS2000Editor()
{
    proc.keyState.removeListener (this);
    setLookAndFeel (nullptr);
}

// ------------------------------------------------------------------ layout
void CTS2000Editor::resized()
{
    for (int j = 0; j < kSliders; ++j)
    {
        const int x = 24 + j * 62;
        slidName[j].setBounds (x, 42, 60, 40);
        slid[j].setBounds (x, 98, 60, 250);
    }

    lcdRect = { 400, 24, 360, 76 };

    const int bw = 66, bh = 36, gap = 6, x0 = 400;
    juce::TextButton* row1[] = { &internalBtn, &cartridgeBtn, &bankBtn, &modeBtn, &zoneBtn };
    juce::TextButton* row2[] = { &initBtn, &writeBtn, &enterBtn, &downBtn, &upBtn };
    for (int i = 0; i < 5; ++i)
    {
        row1[i]->setBounds (x0 + i * (bw + gap), 116, bw, bh);
        row2[i]->setBounds (x0 + i * (bw + gap), 158, bw, bh);
    }
    saveBtn.setBounds (400, 206, 172, 28);
    loadBtn.setBounds (588, 206, 172, 28);
    volLabel.setBounds (400, 252, 60, 22);
    volume.setBounds (462, 252, 298, 22);

    const int cw = 82, ch = 58, cg = 4, gx = 790, gy = 24;
    for (int i = 0; i < kFamilies; ++i)
        famBtn[i].setBounds (gx + (i % 6) * (cw + cg), gy + (i / 6) * (ch + cg), cw, ch);

    pitchWheel.setBounds (26, 356, 44, 140);
    modWheel.setBounds (80, 356, 44, 140);
    pitchLabel.setBounds (20, 496, 56, 18);
    modLabel.setBounds (74, 496, 56, 18);
    keyboard.setBounds (150, 360, 1170, 150);
}

void CTS2000Editor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1d2025));
    g.setColour (juce::Colour (0xff2a2f36));
    g.fillRoundedRectangle (10.f, 10.f, 370.f, 345.f, 6.f);
    g.fillRoundedRectangle (385.f, 10.f, 390.f, 235.f, 6.f);
    g.fillRoundedRectangle (780.f, 10.f, 540.f, 255.f, 6.f);

    g.setColour (juce::Colour (0xff9aa3ad));
    g.setFont (juce::Font (12.f, juce::Font::bold));
    g.drawText ("DATA ENTRY  -  clic / glisser, ou double-clic sur la valeur pour la saisir", 16, 14, 360, 16, juce::Justification::centredLeft);
    g.drawText ("CTS 2000  -  emulation Cross Table Sampled (tables de substitution)", 785, 270, 520, 16, juce::Justification::centredLeft);
    for (int j = 0; j < kSliders; ++j)
        g.drawText (juce::String::charToString ((juce::juce_wchar) ('A' + j)), 24 + j * 62, 84, 60, 14, juce::Justification::centred);

    // LCD 2 x 16
    g.setColour (juce::Colour (0xff25331a));
    g.fillRoundedRectangle (lcdRect.toFloat(), 4.f);
    g.setColour (juce::Colour (0xffd2f08a));
    g.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 26.f, juce::Font::plain));
    g.drawText (line16 (lcd1), lcdRect.withTrimmedLeft (14).withHeight (lcdRect.getHeight() / 2), juce::Justification::centredLeft);
    g.drawText (line16 (lcd2), lcdRect.withTrimmedLeft (14).withTrimmedTop (lcdRect.getHeight() / 2), juce::Justification::centredLeft);
}

// -------------------------------------------------------------------- LCD
void CTS2000Editor::setLcd (const juce::String& a, const juce::String& b) { lcd1 = a; lcd2 = b; repaint (lcdRect); }

void CTS2000Editor::flash (const juce::String& a, const juce::String& b)
{
    setLcd (a, b);
    juce::Component::SafePointer<CTS2000Editor> safe (this);
    juce::Timer::callAfterDelay (1500, [safe] { if (safe != nullptr) safe->refreshLcd(); });
}

void CTS2000Editor::showStandard()
{
    juce::String a = (proc.curSet == 0 ? "INT" : "CRT");
    a << " PRESET " << (proc.curIndex % kBankSize + 1) << (proc.curIndex >= kBankSize ? "B" : "A");
    if (modified) a << " *";
    setLcd (a, proc.curName);
}

void CTS2000Editor::showFamily()
{
    if (family < 0)
    {
        setLcd (globalEdit ? "SELECT GLOBAL" : "SELECT ZONE", "PARAMETERS");
        return;
    }
    const auto& f = kFam[family];
    if (globalEdit) setLcd (f.lcd1, f.lcd2);
    else            setLcd (juce::String (f.lcd1).paddedRight (' ', 15).substring (0, 15) + juce::String (zone), f.lcd2);
}

void CTS2000Editor::refreshLcd()
{
    if (mode == Mode::Standard) showStandard(); else showFamily();

    internalBtn.setToggleState (proc.curSet == 0, juce::dontSendNotification);
    cartridgeBtn.setToggleState (proc.curSet == 1, juce::dontSendNotification);
    modeBtn.setToggleState (mode == Mode::Edit, juce::dontSendNotification);
    for (int i = 0; i < kFamilies; ++i)
    {
        const bool on = mode == Mode::Edit ? (i == family) : (i == proc.curIndex % kBankSize && bankB == (proc.curIndex >= kBankSize));
        famBtn[i].setToggleState (on, juce::dontSendNotification);
    }
}

// ----------------------------------------------------------------- actions
void CTS2000Editor::toggleMode()
{
    mode = (mode == Mode::Standard) ? Mode::Edit : Mode::Standard;
    family = -1;
    writePending = initPending = false;
    updateSliders();
    refreshLcd();
}

void CTS2000Editor::selectPreset (int n)
{
    writePending = initPending = false;
    proc.recallPreset (proc.curSet, (bankB ? kBankSize : 0) + n - 1);
    modified = false;
    refreshLcd();
}

void CTS2000Editor::pressFamily (int i)
{
    if (mode == Mode::Standard) { selectPreset (i + 1); return; }

    if (kFam[i].count > 0)
    {
        family = i;
        updateSliders();
        refreshLcd();
        return;
    }
    switch (i)
    {
        case 18: flash ("MIDI", "HANDLED BY HOST"); break;
        case 20: proc.writeEnabled = ! proc.writeEnabled; flash ("WRITE", proc.writeEnabled ? "ENABLE" : "DISABLE"); break;
        case 22: renamePreset(); break;
        default: flash (kFam[i].lcd1, "NOT IMPLEMENTED"); break;
    }
}

void CTS2000Editor::stepFine (int dir)
{
    if (mode == Mode::Standard)
    {
        const int idx = (proc.curIndex + dir + kPresetsPerSet) % kPresetsPerSet;
        proc.recallPreset (proc.curSet, idx);
        bankB = idx >= kBankSize;
        bankBtn.setButtonText (bankB ? "BANK B" : "BANK A");
        modified = false;
        refreshLcd();
    }
    else if (family >= 0 && lastSlider < kFam[family].count)
    {
        auto& s = slid[lastSlider];
        s.setValue (s.getValue() + dir, juce::sendNotificationSync);
    }
}

void CTS2000Editor::onInit()
{
    initPending = true; writePending = false;
    setLcd ("PRESS ENTER", "TO INITIALISE");
}

void CTS2000Editor::onWrite()
{
    if (writePending) { writePending = false; flash ("WRITE ABORTED", ""); return; }
    if (! proc.writeEnabled) { flash ("WRITE DISABLE", "SELECT PARAM.21"); return; }
    writePending = true; initPending = false;
    setLcd ("ENTER TO RECORD", "WRITE TO ESCAPE");
}

void CTS2000Editor::onEnter()
{
    if (initPending)
    {
        initPending = false;
        proc.initCurrent();
        modified = true;
        updateSliders();
        flash ("THIS PRESET HAS", "BEEN INITIALIZED");
    }
    else if (writePending)
    {
        writePending = false;
        proc.storeCurrent (proc.curSet, proc.curIndex);
        modified = false;
        flash ("WRITE COMPLETED", "");
    }
}

void CTS2000Editor::renamePreset()
{
    auto* w = new juce::AlertWindow ("Preset name", "Max 16 characters", juce::MessageBoxIconType::NoIcon);
    w->addTextEditor ("name", proc.curName);
    w->addButton ("OK", 1);
    w->addButton ("Cancel", 0);
    juce::Component::SafePointer<CTS2000Editor> safe (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([safe, w] (int result)
    {
        if (result == 1 && safe != nullptr)
        {
            safe->proc.curName = w->getTextEditorContents ("name").substring (0, 16);
            safe->modified = true;
            safe->refreshLcd();
        }
    }), true);
}

void CTS2000Editor::saveBank()
{
    chooser = std::make_unique<juce::FileChooser> ("Save bank", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                                                                    .getChildFile ("CTS2000_bank.cts2000"), "*.cts2000");
    juce::Component::SafePointer<CTS2000Editor> safe (this);
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [safe] (const juce::FileChooser& fc)
                          {
                              if (safe == nullptr || fc.getResult() == juce::File()) return;
                              safe->flash (safe->proc.saveBank (fc.getResult()) ? "BANK SAVED" : "SAVE FAILED", "");
                          });
}

void CTS2000Editor::loadBank()
{
    chooser = std::make_unique<juce::FileChooser> ("Load bank", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory), "*.cts2000");
    juce::Component::SafePointer<CTS2000Editor> safe (this);
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [safe] (const juce::FileChooser& fc)
                          {
                              if (safe == nullptr || fc.getResult() == juce::File()) return;
                              const bool ok = safe->proc.loadBank (fc.getResult());
                              safe->modified = false;
                              safe->updateSliders();
                              safe->flash (ok ? "BANK LOADED" : "LOAD FAILED", "");
                          });
}

// ---------------------------------------------------------------- sliders
void CTS2000Editor::updateSliders()
{
    inUpdate = true;
    for (int j = 0; j < kSliders; ++j)
    {
        auto& s = slid[j];
        const bool on = mode == Mode::Edit && family >= 0 && j < kFam[family].count;
        s.setEnabled (on);
        if (on)
        {
            const int id = kFam[family].first + j;
            const auto& d = kDefs[id];
            if (globalEdit)
            {
                const int span = d.mx - d.mn;
                s.setRange (-span, span, 1);
                s.setValue (0, juce::dontSendNotification);
                s.textFromValueFunction = [] (double v) { const int i = (int) std::lround (v); return juce::String (i > 0 ? "+" : "") + juce::String (i); };
            }
            else
            {
                s.setRange (d.mn, d.mx, 1);
                s.setValue (proc.engine.getParam (zone, id), juce::dontSendNotification);
                s.textFromValueFunction = [id] (double v) { return juce::String (valueText (id, (int) std::lround (v))); };
            }
            s.updateText();
            slidName[j].setText (d.name, juce::dontSendNotification);
            oldVal[j] = proc.engine.getParam (zone, id);
            for (int z = 0; z < kZones; ++z) baseVal[z][j] = proc.engine.getParam (z, id);
        }
        else
        {
            s.setRange (0, 1, 1);
            s.setValue (0, juce::dontSendNotification);
            s.textFromValueFunction = nullptr;
            s.updateText();
            slidName[j].setText ("", juce::dontSendNotification);
        }
    }
    inUpdate = false;
}

void CTS2000Editor::sliderMoved (int j)
{
    if (inUpdate || mode != Mode::Edit || family < 0 || j >= kFam[family].count) return;
    const int id = kFam[family].first + j;
    const auto& d = kDefs[id];
    const int v = (int) std::lround (slid[j].getValue());
    lastSlider = j;
    modified = true;

    if (globalEdit)
    {
        for (int z = 0; z < kZones; ++z) proc.engine.setParam (z, id, baseVal[z][j] + v);
        setLcd (d.name, "GLOBAL = " + juce::String (v > 0 ? "+" : "") + juce::String (v));
    }
    else
    {
        proc.engine.setParam (zone, id, v);
        const int nv = proc.engine.getParam (zone, id);
        setLcd (juce::String (d.name).paddedRight (' ', 15).substring (0, 15) + juce::String (zone),
                "OLD " + juce::String (valueText (id, oldVal[j])) + " NEW " + juce::String (valueText (id, nv)));
    }
}

// Appuyer sur une touche du clavier choisit la zone à éditer (comme sur l'original).
void CTS2000Editor::handleNoteOn (juce::MidiKeyboardState*, int, int note, float)
{
    juce::Component::SafePointer<CTS2000Editor> safe (this);
    juce::MessageManager::callAsync ([safe, note] { if (safe != nullptr) safe->zoneChosenByKey (note); });
}

void CTS2000Editor::zoneChosenByKey (int note)
{
    if (mode != Mode::Edit || globalEdit) return;
    const int z = Engine::zoneOfNote (note);
    if (z == zone) return;
    zone = z;
    updateSliders();
    refreshLcd();
}
