#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "dsp/PitchDetector.h"
#include "dsp/PitchFormantEngine.h"
#include "dsp/TubeSaturator.h"

#if (MSVC)
#include "ipps.h"
#endif

namespace ParamIDs
{
    static constexpr auto pitch = "pitch";
    static constexpr auto formant = "formant";
    static constexpr auto drive = "drive";
    static constexpr auto mix = "mix";
    static constexpr auto mode = "mode";
    static constexpr auto link = "link";
}

class PluginProcessor : public juce::AudioProcessor
{
public:
    PluginProcessor();
    ~PluginProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // The LCD's dice button: rolls musical random values for pitch/formant/
    // drive/mix without touching the preset list. Message thread only.
    void randomiseParameters();

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    using ShelfFilter = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                                       juce::dsp::IIR::Coefficients<float>>;

    PitchFormantEngine pitchEngine;
    PitchDetector pitchDetector;
    TubeSaturator saturator;
    std::unique_ptr<juce::dsp::DryWetMixer<float>> dryWetMixer;

    // Subtle always-on tilt (low shelf up, high shelf down) that gives the wet
    // path its analog-leaning signature
    ShelfFilter warmthLowShelf, warmthHighShelf;

    // Cached raw parameter values, safe to read from the audio thread
    std::atomic<float>* pitchParam = nullptr;
    std::atomic<float>* formantParam = nullptr;
    std::atomic<float>* driveParam = nullptr;
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* modeParam = nullptr;
    std::atomic<float>* linkParam = nullptr;

    // Smoothing state (audio thread only)
    float targetTransposeSemitones = 0.0f;
    float smoothedTransposeSemitones = 0.0f;
    float smoothedFormantSemitones = 0.0f;
    float smoothedDrive01 = 0.0f;
    float smoothedDetectedMidi = 0.0f; // Robot mode: steadies the correction against detector jitter

    int currentProgram = 0;
    juce::Random randomPresetGenerator;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};
