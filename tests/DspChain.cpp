#include <PluginProcessor.h>
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <cmath>
#include <vector>

// End-to-end check of the wet path: a harmonic-rich 220Hz "vocal-like" tone
// with pitch = +12st must come out with a fundamental close to 440Hz.
//
// The source has to be harmonically rich: the PSOLA engine preserves the
// spectral envelope (that's the whole point of a formant-preserving shifter),
// so a *pure* sine shifted up simply loses its energy — there is nothing at
// the new harmonic positions for the envelope to light up, exactly like a real
// voice keeps its formants in place. F0 is measured with the plugin's own YIN
// detector, which is reliable on rich signals where zero-crossings are not.
TEST_CASE ("Pitch shift chain", "[dsp]")
{
    constexpr double sampleRate = 44100.0;
    constexpr int blockSize = 512;
    constexpr double inputHz = 220.0;
    constexpr int totalSeconds = 2;
    constexpr int numHarmonics = 10;

    PluginProcessor processor;

    auto* pitch = processor.apvts.getParameter (ParamIDs::pitch);
    REQUIRE (pitch != nullptr);
    pitch->setValueNotifyingHost (pitch->convertTo0to1 (12.0f));

    processor.setPlayConfigDetails (2, 2, sampleRate, blockSize);
    processor.prepareToPlay (sampleRate, blockSize);

    REQUIRE (processor.getLatencySamples() > 0);

    juce::AudioBuffer<float> buffer (2, blockSize);
    juce::MidiBuffer midi;
    std::vector<float> output;
    output.reserve ((size_t) (sampleRate * totalSeconds));

    double phase = 0.0;
    const double phaseInc = 2.0 * juce::MathConstants<double>::pi * inputHz / sampleRate;
    const int numBlocks = (int) (sampleRate * totalSeconds / blockSize);

    for (int b = 0; b < numBlocks; ++b)
    {
        for (int i = 0; i < blockSize; ++i)
        {
            // glottal-pulse-like source: 1/n harmonic rolloff
            double s = 0.0;
            for (int n = 1; n <= numHarmonics; ++n)
                s += std::sin (phase * (double) n) / (double) n;

            phase += phaseInc;
            const auto sample = (float) (0.25 * s);
            buffer.setSample (0, i, sample);
            buffer.setSample (1, i, sample);
        }

        processor.processBlock (buffer, midi);

        const float* out = buffer.getReadPointer (0);
        output.insert (output.end(), out, out + blockSize);
    }

    // Analyse the steady-state tail, well past the reported latency
    const int analysisLength = 16384;
    REQUIRE ((int) output.size() > processor.getLatencySamples() + analysisLength);

    const float* tail = output.data() + output.size() - (size_t) analysisLength;

    float sumSquares = 0.0f;
    for (int i = 0; i < analysisLength; ++i)
        sumSquares += tail[i] * tail[i];

    const float rms = std::sqrt (sumSquares / (float) analysisLength);
    CHECK (rms > 0.03f); // signal actually made it through

    // Measure the output fundamental with the plugin's own YIN detector
    juce::AudioBuffer<float> tailBuffer (1, analysisLength);
    tailBuffer.copyFrom (0, 0, tail, analysisLength);

    PitchDetector meter;
    meter.prepare (sampleRate);
    meter.processBlock (tailBuffer);

    const float measuredHz = meter.getPitchHz();
    CHECK (measuredHz > 440.0f * 0.94f);
    CHECK (measuredHz < 440.0f * 1.06f);
}

// The "zero saccade" guarantee: at pitch 0 / formant 0, the PSOLA engine must
// reconstruct the voice with perfect phase coherence — the output is the input
// delayed by exactly getLatencySamples(), nothing else. This drives the
// phase-locked synthesis grid; if the lock regresses, this test fails loudly.
TEST_CASE ("PSOLA engine is transparent at unity", "[dsp]")
{
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 480;
    constexpr int totalSamples = (int) sampleRate * 2;
    constexpr int numHarmonics = 8;

    PitchFormantEngine engine;
    PitchDetector detector;
    engine.prepare (sampleRate, 1, blockSize);
    detector.prepare (sampleRate);
    engine.setTransposeSemitones (0.0f);
    engine.setFormantSemitones (0.0f);

    std::vector<float> input ((size_t) totalSamples), output ((size_t) totalSamples);

    double phase = 0.0;
    const double phaseInc = 2.0 * juce::MathConstants<double>::pi * 180.0 / sampleRate;
    for (int i = 0; i < totalSamples; ++i)
    {
        double s = 0.0;
        for (int n = 1; n <= numHarmonics; ++n)
            s += std::sin (phase * (double) n) / (double) n;
        phase += phaseInc;
        input[(size_t) i] = (float) (0.3 * s);
    }

    juce::AudioBuffer<float> buffer (1, blockSize);

    for (int start = 0; start + blockSize <= totalSamples; start += blockSize)
    {
        buffer.copyFrom (0, 0, input.data() + start, blockSize);
        detector.processBlock (buffer);
        engine.setDetectedPitch (detector.getPitchHz());
        engine.process (buffer);
        std::copy (buffer.getReadPointer (0), buffer.getReadPointer (0) + blockSize,
                   output.begin() + start);
    }

    const int latency = engine.getLatencySamples();
    REQUIRE (latency > 0);

    // steady-state tail: compare output against the input delayed by the
    // reported latency, sample for sample
    double errorEnergy = 0.0, signalEnergy = 0.0;
    for (int m = totalSamples - 24000; m < totalSamples; ++m)
    {
        const float expected = input[(size_t) (m - latency)];
        const float diff = output[(size_t) m] - expected;
        errorEnergy += (double) diff * (double) diff;
        signalEnergy += (double) expected * (double) expected;
    }

    REQUIRE (signalEnergy > 0.0);
    const double relativeError = std::sqrt (errorEnergy / signalEnergy);
    CHECK (relativeError < 0.05);
}

// Automation lanes read the parameter's own text function: at most 2 decimals,
// no superfluous trailing zeros, with the unit — and never the "20.00 -> 2" trap.
TEST_CASE ("Parameter automation strings are clean", "[params]")
{
    PluginProcessor processor;

    auto textFor = [&] (const char* id, float value)
    {
        // Call through RangedAudioParameter* (getText is public there; it's the
        // AudioParameterFloat override that's private)
        auto* p = processor.apvts.getParameter (id);
        REQUIRE (p != nullptr);
        return p->getText (p->convertTo0to1 (value), 32);
    };

    CHECK (textFor (ParamIDs::pitch, 2.3f) == "2.3 st");
    CHECK (textFor (ParamIDs::pitch, -5.0f) == "-5 st");
    CHECK (textFor (ParamIDs::pitch, 0.0f) == "0 st");
    CHECK (textFor (ParamIDs::formant, 12.0f) == "12 st");
    CHECK (textFor (ParamIDs::drive, 20.0f) == "20 %");   // the trailing-zero trap
    CHECK (textFor (ParamIDs::mix, 100.0f) == "100 %");
    CHECK (textFor (ParamIDs::mix, 47.5f) == "47.5 %");
}

namespace
{
    // Runs a sine through the full chain and returns the steady-state tail RMS
    float measureOutputRms (float driveValue)
    {
        constexpr double sampleRate = 44100.0;
        constexpr int blockSize = 512;

        PluginProcessor processor;

        auto* drive = processor.apvts.getParameter (ParamIDs::drive);
        REQUIRE (drive != nullptr);
        drive->setValueNotifyingHost (drive->convertTo0to1 (driveValue));

        processor.setPlayConfigDetails (2, 2, sampleRate, blockSize);
        processor.prepareToPlay (sampleRate, blockSize);

        juce::AudioBuffer<float> buffer (2, blockSize);
        juce::MidiBuffer midi;
        std::vector<float> output;

        double phase = 0.0;
        const double phaseInc = 2.0 * juce::MathConstants<double>::pi * 200.0 / sampleRate;
        const int numBlocks = (int) (sampleRate * 2 / blockSize);

        for (int b = 0; b < numBlocks; ++b)
        {
            for (int i = 0; i < blockSize; ++i)
            {
                const auto sample = (float) (0.3 * std::sin (phase));
                phase += phaseInc;
                buffer.setSample (0, i, sample);
                buffer.setSample (1, i, sample);
            }

            processor.processBlock (buffer, midi);

            const float* out = buffer.getReadPointer (0);
            output.insert (output.end(), out, out + blockSize);
        }

        const int analysisLength = 16384;
        const float* tail = output.data() + output.size() - (size_t) analysisLength;

        float sumSquares = 0.0f;
        for (int i = 0; i < analysisLength; ++i)
            sumSquares += tail[i] * tail[i];

        return std::sqrt (sumSquares / (float) analysisLength);
    }
}

// Regression: drive used to lower the output level; the make-up gain must keep
// (or slightly raise) the perceived volume as drive goes up
TEST_CASE ("Drive make-up gain keeps level", "[dsp]")
{
    const float cleanRms = measureOutputRms (0.0f);
    const float drivenRms = measureOutputRms (100.0f);

    CHECK (cleanRms > 0.05f);
    CHECK (drivenRms >= cleanRms * 0.9f);
}

TEST_CASE ("Factory presets", "[presets]")
{
    PluginProcessor processor;

    // 20 fixed presets (random values live behind the LCD dice button instead)
    REQUIRE (processor.getNumPrograms() == 20);
    CHECK (processor.getProgramName (0) == "Init (Flat)");
    CHECK (processor.getProgramName (1) == "Tube Warmth");
    CHECK (processor.getProgramName (19) == "Monster");

    // Every fixed preset name must be non-empty and unique
    std::vector<juce::String> names;
    for (int i = 0; i < processor.getNumPrograms(); ++i)
    {
        const auto name = processor.getProgramName (i);
        CHECK (name.isNotEmpty());
        CHECK (std::find (names.begin(), names.end(), name) == names.end());
        names.push_back (name);
    }

    auto paramIs = [&processor] (const char* paramID, float expected)
    {
        return std::abs (processor.apvts.getRawParameterValue (paramID)->load() - expected) < 1.0e-4f;
    };

    processor.setCurrentProgram (0); // Init (Flat)
    CHECK (processor.getCurrentProgram() == 0);
    CHECK (paramIs (ParamIDs::mode, 0.0f));
    CHECK (paramIs (ParamIDs::pitch, 0.0f));
    CHECK (paramIs (ParamIDs::drive, 0.0f));
    CHECK (paramIs (ParamIDs::mix, 100.0f));

    processor.setCurrentProgram (16); // Deep Machine (robot)
    CHECK (paramIs (ParamIDs::mode, 2.0f));
    CHECK (paramIs (ParamIDs::pitch, -5.0f));

    processor.setCurrentProgram (18); // Chipmunk (linked)
    CHECK (paramIs (ParamIDs::link, 1.0f));
    CHECK (paramIs (ParamIDs::pitch, 9.0f));
}

TEST_CASE ("Program index survives a state round-trip", "[presets]")
{
    PluginProcessor source;
    source.setCurrentProgram (11); // Clean Auto-Tune

    juce::MemoryBlock state;
    source.getStateInformation (state);

    PluginProcessor restored;
    restored.setStateInformation (state.getData(), (int) state.getSize());

    CHECK (restored.getCurrentProgram() == 11);
    CHECK (restored.getProgramName (restored.getCurrentProgram()) == "Clean Auto-Tune");
}

TEST_CASE ("Dice randomisation stays within musical ranges", "[presets]")
{
    PluginProcessor processor;

    for (int i = 0; i < 25; ++i)
    {
        processor.randomiseParameters();

        const float pitch = processor.apvts.getRawParameterValue (ParamIDs::pitch)->load();
        const float formant = processor.apvts.getRawParameterValue (ParamIDs::formant)->load();
        const float drive = processor.apvts.getRawParameterValue (ParamIDs::drive)->load();
        const float mix = processor.apvts.getRawParameterValue (ParamIDs::mix)->load();

        // The 0.1 parameter step snaps values as start + n*0.1f, which can land
        // a float epsilon above the mathematical bound (e.g. 7.0000019) — hence
        // the tolerance.
        constexpr float tol = 1.0e-3f;
        CHECK (pitch >= -7.0f - tol);
        CHECK (pitch <= 7.0f + tol);
        CHECK (formant >= -4.0f - tol);
        CHECK (formant <= 4.0f + tol);
        CHECK (drive >= 0.0f - tol);
        CHECK (drive <= 60.0f + tol);
        CHECK (mix >= 40.0f - tol);
        CHECK (mix <= 100.0f + tol);
    }
}
