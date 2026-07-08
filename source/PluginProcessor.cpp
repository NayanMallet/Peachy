#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

namespace
{
    constexpr int modeTranspose = 0;
    constexpr int modeQuantize = 1;
    constexpr int modeRobot = 2;

    // Robot mode pins the voice to this note (C3 ~ 130.8Hz), offset by the pitch param
    constexpr float robotBaseMidiNote = 48.0f;

    struct FactoryPreset
    {
        const char* name;
        float pitch, formant, drive, mix;
        int mode; // 0 = Transpose, 1 = Quantize, 2 = Robot
        bool link;
    };

    // A curated bank, Soundtoys-style: every entry is meant to sound good on a
    // clean monophonic vocal straight away. Grouped by use (utility → formant
    // character → harmony → hard-tune → robot → FX). Values chosen for the sweet
    // spots of this engine: formant-only moves stay natural, small pitch shifts
    // give clean harmonies, drive rides the auto-makeup so it never dumps level,
    // and mix < 100 with drive gives parallel saturation. `link` is only used
    // where pitch and formant genuinely track together (chipmunk / monster).
    //                     name              pitch  formant  drive    mix   mode           link
    constexpr FactoryPreset factoryPresets[] = {
        // — utility / warmth —
        { "Init (Flat)",       0.0f,  0.0f,  0.0f, 100.0f, modeTranspose, false },
        { "Tube Warmth",       0.0f,  0.0f, 20.0f, 100.0f, modeTranspose, false },
        { "Parallel Crush",    0.0f,  0.0f, 58.0f,  45.0f, modeTranspose, false },
        // — formant character (pitch preserved, natural) —
        { "Chest & Body",      0.0f, -3.0f, 10.0f, 100.0f, modeTranspose, false },
        { "Bright & Young",    0.0f,  3.0f,  7.0f, 100.0f, modeTranspose, false },
        { "Gender Flip Fem",   4.0f,  3.0f,  6.0f, 100.0f, modeTranspose, false },
        { "Gender Flip Masc", -4.0f, -3.0f, 10.0f, 100.0f, modeTranspose, false },
        // — harmony / thickening —
        { "Octave Stack",     12.0f,  0.0f,  5.0f,  45.0f, modeTranspose, false },
        { "Sub Octave",      -12.0f,  0.0f,  9.0f,  42.0f, modeTranspose, false },
        { "Fifth Harmony",     7.0f,  0.0f,  0.0f,  45.0f, modeTranspose, false },
        { "Natural Double",    0.2f, -0.8f,  6.0f,  48.0f, modeTranspose, false },
        // — hard-tune (Quantize) —
        { "Clean Auto-Tune",   0.0f,  0.0f,  6.0f, 100.0f, modeQuantize,  false },
        { "Trap Ad-Lib",       0.0f,  1.0f, 18.0f, 100.0f, modeQuantize,  false },
        { "Drill Grit",        0.0f, -1.0f, 30.0f, 100.0f, modeQuantize,  false },
        { "Pop Tune Sheen",    0.0f,  0.6f, 10.0f, 100.0f, modeQuantize,  false },
        // — robot (locked to C3 + pitch) —
        { "Classic Robot",     0.0f,  0.0f, 22.0f, 100.0f, modeRobot,     false },
        { "Deep Machine",     -5.0f, -4.0f, 44.0f, 100.0f, modeRobot,     false },
        { "High Droid",        7.0f,  2.0f, 15.0f, 100.0f, modeRobot,     false },
        // — character FX (pitch + formant linked) —
        { "Chipmunk",          9.0f,  9.0f, 12.0f, 100.0f, modeTranspose, true  },
        { "Monster",          -8.0f, -8.0f, 36.0f, 100.0f, modeTranspose, true  },
    };

    constexpr int numFixedPresets = (int) (sizeof (factoryPresets) / sizeof (factoryPresets[0]));

    float hzToMidiNote (float hz)
    {
        return 69.0f + 12.0f * std::log2 (hz / 440.0f);
    }
}

//==============================================================================
PluginProcessor::PluginProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    pitchParam = apvts.getRawParameterValue (ParamIDs::pitch);
    formantParam = apvts.getRawParameterValue (ParamIDs::formant);
    driveParam = apvts.getRawParameterValue (ParamIDs::drive);
    mixParam = apvts.getRawParameterValue (ParamIDs::mix);
    modeParam = apvts.getRawParameterValue (ParamIDs::mode);
    linkParam = apvts.getRawParameterValue (ParamIDs::link);
}

PluginProcessor::~PluginProcessor()
{
}

//==============================================================================
const juce::String PluginProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PluginProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PluginProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PluginProcessor::getNumPrograms()
{
    return numFixedPresets;
}

int PluginProcessor::getCurrentProgram()
{
    return currentProgram;
}

void PluginProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= getNumPrograms())
        return;

    currentProgram = index;

    // Going through the parameters (rather than poking DSP state) keeps the host,
    // the editor attachments and the audio thread in sync. The audible transition
    // is click-free because every parameter is smoothed downstream: transpose and
    // drive by the one-pole smoothers in processBlock, mix by the DryWetMixer ramp.
    auto setParam = [this] (const char* paramID, float value)
    {
        if (auto* param = apvts.getParameter (paramID))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    };

    const auto& preset = factoryPresets[index];
    setParam (ParamIDs::pitch, preset.pitch);
    setParam (ParamIDs::formant, preset.formant);
    setParam (ParamIDs::drive, preset.drive);
    setParam (ParamIDs::mix, preset.mix);
    setParam (ParamIDs::mode, (float) preset.mode);
    setParam (ParamIDs::link, preset.link ? 1.0f : 0.0f);
}

void PluginProcessor::randomiseParameters()
{
    auto setParam = [this] (const char* paramID, float value)
    {
        if (auto* param = apvts.getParameter (paramID))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    };

    // Rolled fresh on every click, within ranges that stay musical rather than
    // jarring; never mutates the fixed preset table.
    setParam (ParamIDs::pitch, randomPresetGenerator.nextFloat() * 14.0f - 7.0f);    // -7 .. +7 st
    setParam (ParamIDs::formant, randomPresetGenerator.nextFloat() * 8.0f - 4.0f);   // -4 .. +4 st
    setParam (ParamIDs::drive, randomPresetGenerator.nextFloat() * 60.0f);           // 0 .. 60 %
    setParam (ParamIDs::mix, 40.0f + randomPresetGenerator.nextFloat() * 60.0f);     // 40 .. 100 %
    setParam (ParamIDs::mode, (float) modeTranspose);
    setParam (ParamIDs::link, 0.0f);
}

const juce::String PluginProcessor::getProgramName (int index)
{
    if (index >= 0 && index < numFixedPresets)
        return factoryPresets[index].name;

    return {};
}

void PluginProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName); // factory presets are fixed
}

//==============================================================================
void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const int numChannels = juce::jmax (1, getTotalNumOutputChannels());

    pitchEngine.prepare (sampleRate, numChannels, samplesPerBlock);
    pitchDetector.prepare (sampleRate);

    const juce::dsp::ProcessSpec spec { sampleRate,
                                        (juce::uint32) samplesPerBlock,
                                        (juce::uint32) numChannels };
    saturator.prepare (spec);

    const float totalLatency = (float) pitchEngine.getLatencySamples()
                             + saturator.getLatencySamples();

    dryWetMixer = std::make_unique<juce::dsp::DryWetMixer<float>> ((int) std::ceil (totalLatency) + 8);
    dryWetMixer->prepare (spec);
    dryWetMixer->setMixingRule (juce::dsp::DryWetMixingRule::linear);
    dryWetMixer->setWetLatency (totalLatency); // keeps dry/wet phase-aligned

    // Gentle tilt: +1.2dB of low warmth, -1dB off the very top
    warmthLowShelf.state = juce::dsp::IIR::Coefficients<float>::makeLowShelf (
        sampleRate, 140.0f, 0.6f, juce::Decibels::decibelsToGain (1.2f));
    warmthHighShelf.state = juce::dsp::IIR::Coefficients<float>::makeHighShelf (
        sampleRate, 9500.0f, 0.6f, juce::Decibels::decibelsToGain (-1.0f));
    warmthLowShelf.prepare (spec);
    warmthHighShelf.prepare (spec);
    warmthLowShelf.reset();
    warmthHighShelf.reset();

    // Report the internal latency so hosts (Logic etc.) can compensate (PDC)
    setLatencySamples ((int) std::lround (totalLatency));

    targetTransposeSemitones = 0.0f;
    smoothedTransposeSemitones = 0.0f;
    smoothedFormantSemitones = 0.0f;
    smoothedDrive01 = 0.0f;
    smoothedDetectedMidi = 0.0f;
}

void PluginProcessor::releaseResources()
{
    pitchEngine.reset();
    pitchDetector.reset();
    saturator.reset();

    if (dryWetMixer != nullptr)
        dryWetMixer->reset();
}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    const int numSamples = buffer.getNumSamples();

    if (numSamples == 0 || dryWetMixer == nullptr)
        return;

    const float pitchSemitones = pitchParam->load();
    const bool linked = linkParam->load() > 0.5f;
    // Link locks the formant shift onto the pitch value
    const float formantSemitones = linked ? pitchSemitones : formantParam->load();
    const float drive01 = driveParam->load() * 0.01f;
    const float mix01 = mixParam->load() * 0.01f;
    const int mode = (int) std::lround (modeParam->load());

    dryWetMixer->setWetMixProportion (mix01);
    juce::dsp::AudioBlock<float> block (buffer);
    dryWetMixer->pushDrySamples (block);

    // --- Pitch detection: the PSOLA engine needs F0 in every mode ---
    pitchDetector.processBlock (buffer);
    const float detectedHz = pitchDetector.getPitchHz();
    pitchEngine.setDetectedPitch (detectedHz);

    // --- Per-mode transpose target (held on unvoiced input to avoid jumps) ---
    if (mode == modeQuantize)
    {
        if (detectedHz > 0.0f)
        {
            // Hard-tune: snap the sung note to the nearest semitone, pitch adds an offset
            const float midiNote = hzToMidiNote (detectedHz);
            targetTransposeSemitones = pitchSemitones + (std::round (midiNote) - midiNote);
        }
    }
    else if (mode == modeRobot)
    {
        if (detectedHz > 0.0f)
        {
            // Cancel the melody: shift whatever comes in onto one fixed target note.
            // The detected pitch is lightly smoothed (~2 blocks) so detector jitter
            // doesn't wobble the lock, while real vibrato is still fully cancelled.
            const float midiNote = hzToMidiNote (detectedHz);
            smoothedDetectedMidi = smoothedDetectedMidi <= 0.0f
                                       ? midiNote
                                       : smoothedDetectedMidi + 0.4f * (midiNote - smoothedDetectedMidi);

            const float targetMidi = robotBaseMidiNote + pitchSemitones;
            targetTransposeSemitones = juce::jlimit (-36.0f, 36.0f, targetMidi - smoothedDetectedMidi);
        }
    }
    else
    {
        targetTransposeSemitones = pitchSemitones;
    }

    if (mode != modeRobot)
        smoothedDetectedMidi = 0.0f;

    // Retune speed: near-instant for hard-tune (the PSOLA grain windows keep it
    // click-free), a touch slower for Robot, knob-friendly for Transpose.
    const float retuneSeconds = mode == modeQuantize ? 0.0015f
                              : mode == modeRobot ? 0.003f
                                                  : 0.005f;
    const float smoothing = 1.0f - std::exp (-(float) numSamples / ((float) getSampleRate() * retuneSeconds));
    const float paramSmoothing = 1.0f - std::exp (-(float) numSamples / ((float) getSampleRate() * 0.005f));
    smoothedTransposeSemitones += smoothing * (targetTransposeSemitones - smoothedTransposeSemitones);
    smoothedFormantSemitones += paramSmoothing * (formantSemitones - smoothedFormantSemitones);
    smoothedDrive01 += paramSmoothing * (drive01 - smoothedDrive01);

    // --- Pitch/formant shifting (TD-PSOLA) ---
    pitchEngine.setTransposeSemitones (smoothedTransposeSemitones);
    pitchEngine.setFormantSemitones (smoothedFormantSemitones);
    pitchEngine.process (buffer);

    // --- Saturation ---
    saturator.setDrive (smoothedDrive01);
    saturator.process (buffer);

    // --- Warmth tilt: part of the wet path's signature, always on ---
    juce::dsp::ProcessContextReplacing<float> warmthContext (block);
    warmthLowShelf.process (warmthContext);
    warmthHighShelf.process (warmthContext);

    // --- Dry/wet ---
    dryWetMixer->mixWetSamples (block);
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout PluginProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Host automation lanes and generic editors read the parameter's own
    // string function. Format floats with at most 2 decimals, trimming only the
    // fractional zeros (so 2.30 -> "2.3", 45.00 -> "45", never 20.00 -> "2"),
    // followed by the unit. valueFromString parses the leading number back.
    auto stringFromValue = [] (const juce::String& unit)
    {
        return [unit] (float value, int) -> juce::String
        {
            juce::String text (value, 2);

            if (text.containsChar ('.'))
                text = text.trimCharactersAtEnd ("0").trimCharactersAtEnd (".");

            return unit.isEmpty() ? text : text + " " + unit;
        };
    };
    auto valueFromString = [] (const juce::String& text) { return text.getFloatValue(); };

    auto floatAttributes = [&] (const juce::String& unit)
    {
        return juce::AudioParameterFloatAttributes()
            .withLabel (unit)
            .withStringFromValueFunction (stringFromValue (unit))
            .withValueFromStringFunction (valueFromString);
    };

    // 0.1 step: one decimal place is precise enough and much nicer to dial in
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::pitch, 1 },
        "Pitch",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f),
        0.0f,
        floatAttributes ("st")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::formant, 1 },
        "Formant",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f),
        0.0f,
        floatAttributes ("st")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::drive, 1 },
        "Drive",
        juce::NormalisableRange<float> (0.0f, 100.0f),
        0.0f,
        floatAttributes ("%")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::mix, 1 },
        "Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f),
        100.0f,
        floatAttributes ("%")));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParamIDs::mode, 1 },
        "Mode",
        juce::StringArray { "Transpose", "Quantize", "Robot" },
        0));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParamIDs::link, 1 },
        "Link",
        false));

    return { params.begin(), params.end() };
}

//==============================================================================
bool PluginProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor (*this);
}

//==============================================================================
void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();

    // Remember which factory preset the session was on, so the LCD shows the
    // right name after reload (parameter values are the real state; this is
    // display metadata only).
    state.setProperty ("program", currentProgram, nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        const auto state = juce::ValueTree::fromXml (*xml);
        currentProgram = juce::jlimit (0, getNumPrograms() - 1,
                                       (int) state.getProperty ("program", 0));
        apvts.replaceState (state);
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
