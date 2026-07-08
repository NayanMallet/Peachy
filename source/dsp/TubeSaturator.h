#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>

// Decapitator-style tube drive stage at 4x oversampling:
//
// - Pre-emphasis boosts the upper mids into the tube, de-emphasis pulls them
//   back out afterwards — the distortion products generated in between give
//   the aggressive-but-warm voicing of a driven triode input stage.
// - Strongly asymmetric transfer curve (gentle plate compression on the
//   positive half, an earlier and harder grid-conduction knee on the negative
//   half) for a dense even+odd harmonic series.
// - Dynamic bias: the operating point rides an envelope follower (fast attack,
//   slow release), so the curve breathes with the vocal — a hysteresis-like
//   memory that static waveshapers don't have.
// - Adaptive make-up gain: each block, the static curve is probed with a
//   16-point sine at the tracked input level and the output is rescaled so
//   perceived RMS stays put — drive at 100% wrecks the harmonics, not the fader.
class TubeSaturator
{
public:
    TubeSaturator() = default;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    void setDrive (float newDrive01) noexcept { drive01 = juce::jlimit (0.0f, 1.0f, newDrive01); }

    float getLatencySamples() const;

    void process (juce::AudioBuffer<float>& buffer);

private:
    static float triodeShape (float v) noexcept;

    static constexpr size_t oversamplingOrder = 2; // 2^2 = 4x
    static constexpr int maxChannels = 2;
    static constexpr float emphasisFreq = 1700.0f; // upper-mid tilt into/out of the tube
    static constexpr float emphasisDb = 5.0f;
    static constexpr float maxDriveDb = 30.0f;     // input gain range of the drive knob
    static constexpr float biasAmount = 0.30f;     // dynamic-bias depth at full drive

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    std::array<juce::dsp::IIR::Filter<float>, maxChannels> preEmphasis, deEmphasis;

    float drive01 = 0.0f;
    double baseRate = 48000.0;

    // envelope follower state (oversampled rate)
    float envAttackCoeff = 0.0f, envReleaseCoeff = 0.0f;
    std::array<float, maxChannels> envelope {};

    // make-up gain state (base rate, per block)
    float inputLevel = 0.0f;
    float inputLevelDecay = 0.99f;
    float makeupSm = 1.0f;
    float makeupSmoothing = 0.1f;

    // DC blocker (the asymmetry + bias shift the average level)
    float dcCoeff = 0.998f;
    std::array<float, maxChannels> dcX1 {}, dcY1 {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TubeSaturator)
};
