#pragma once

#include "PluginProcessor.h"
#include "PeachyLookAndFeel.h"

//==============================================================================
class PluginEditor : public juce::AudioProcessorEditor,
                     private juce::Timer,
                     private juce::AsyncUpdater,
                     private juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void setupKnob (juce::Slider& slider, int numTicks, bool vuStyle);
    void layoutKnobColumn (juce::Rectangle<int> column, juce::Slider& slider, juce::Label& name, LcdValueLabel& value);
    void updateModeButtons (int modeIndex);
    void refreshLcd();
    void showPresetMenu();
    void stepPreset (int delta);
    void handleLinkedParameterChange (const juce::String& changedID);
    void timerCallback() override { refreshLcd(); }

    // AudioProcessorValueTreeState::Listener can fire on the audio thread
    // (host automation). triggerAsyncUpdate() is realtime-safe (no allocation,
    // unlike MessageManager::callAsync), so the callback only stores which
    // parameter moved and defers all UI work to handleAsyncUpdate().
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void handleAsyncUpdate() override;

    std::atomic<int> pendingLinkSource { -1 }; // 0 = pitch, 1 = formant, 2 = link

    PluginProcessor& processorRef;
    PeachyLookAndFeel peachyLookAndFeel;

    // ─ Panel 1: Pitch & Formant ─
    juce::Slider pitchSlider, formantSlider;
    juce::Label pitchNameLabel { {}, "PITCH" }, formantNameLabel { {}, "FORMANT" };
    LcdValueLabel pitchValueLabel, formantValueLabel;
    juce::ToggleButton linkButton { "LINK" };
    juce::Rectangle<int> linkLedBounds; // independent LED circle above the button

    // ─ Panel 2: Mode ─
    juce::TextButton modeTransposeButton { "TRANSPOSE" }, modeQuantizeButton { "QUANTIZE" }, modeRobotButton { "ROBOT" };

    // ─ Panel 3: Drive & Mix ("Color & Output") ─
    juce::Slider driveSlider, mixSlider;
    juce::Label driveNameLabel { {}, "DRIVE" }, mixNameLabel { {}, "MIX" };
    LcdValueLabel driveValueLabel, mixValueLabel;

    // ─ Header LCD (single block, clickable — opens the preset menu) ─
    LcdDisplay headerLcd;

    // Panel/header/footer chrome bounds, computed in resized(), painted in paint()
    juce::Rectangle<int> headerBounds, panelPitchFormant, panelMode, panelDriveMix, footerBounds;

    std::unique_ptr<SliderAttachment> pitchAttachment, formantAttachment, driveAttachment, mixAttachment;
    std::unique_ptr<ButtonAttachment> linkAttachment;
    std::unique_ptr<juce::ParameterAttachment> modeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
