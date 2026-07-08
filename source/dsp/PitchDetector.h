#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

// Monophonic fundamental-frequency detector based on the YIN algorithm
// (de Cheveigné & Kawahara, 2002). Feed it audio blocks; it analyses a sliding
// window every hop and exposes the latest estimate. All allocations happen in
// prepare(); processBlock() is realtime-safe.
class PitchDetector
{
public:
    PitchDetector() = default;

    void prepare (double newSampleRate);
    void reset();

    void processBlock (const juce::AudioBuffer<float>& buffer);

    // Latest estimate in Hz, or -1 when the input is silent/unvoiced
    float getPitchHz() const noexcept { return pitchHz; }

private:
    void analyze();

    std::vector<float> ring;    // circular input history (mono mixdown)
    std::vector<float> window;  // unrolled analysis frame, oldest -> newest
    std::vector<float> cmndf;   // cumulative mean normalised difference function

    double sampleRate = 48000.0;
    int windowSize = 0;
    int maxLag = 0;
    int minLag = 2;
    int hopSize = 0;
    int writePos = 0;
    int samplesSinceAnalysis = 0;
    int samplesInRing = 0;
    float pitchHz = -1.0f;
    float lastVoicedLag = -1.0f; // octave-jump continuity guard
    int unvoicedFrames = 0;

    static constexpr float yinThreshold = 0.15f;
    static constexpr float minFrequencyHz = 60.0f;  // voice range (PSOLA latency scales with 1/minF0)
    static constexpr float maxFrequencyHz = 1000.0f;
    static constexpr double silenceGate = 1.0e-6; // mean-square, ~-60dBFS

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchDetector)
};
