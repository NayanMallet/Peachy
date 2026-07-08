#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

// Monophonic TD-PSOLA (Time-Domain Pitch-Synchronous Overlap-Add) pitch and
// formant shifter, in the spirit of the H3000-style voice processors: analysis
// marks are placed one pitch period apart (phase-aligned to the previous mark
// by cross-correlation), two-period Hann grains are overlap-added on a new
// synthesis grid spaced period/ratio apart, and the result is normalised by
// the accumulated window sum — which makes the engine *exactly* transparent at
// ratio 1 and keeps vocal phase rock-solid.
//
// - Formant control resamples the grain contents (spectral-envelope scaling)
//   independently of the pitch grid.
// - Unvoiced input (consonants, breaths) passes through at ratio 1, so
//   transients are never doubled or skipped.
// - Strictly monophonic by design: input is summed to mono, the processed
//   voice is written to every output channel.
//
// All allocations happen in prepare(); process() is realtime-safe.
class PitchFormantEngine
{
public:
    PitchFormantEngine() = default;

    void prepare (double newSampleRate, int newNumChannels, int newMaxBlockSize);
    void reset();

    void setTransposeSemitones (float semitones);
    void setFormantSemitones (float semitones);

    // Feed the latest F0 estimate every block (from the YIN detector).
    // hz <= 0 means unvoiced/silent.
    void setDetectedPitch (float hz) noexcept;

    int getLatencySamples() const noexcept { return latencySamples; }

    void process (juce::AudioBuffer<float>& buffer);

private:
    struct PitchMark
    {
        juce::int64 position = 0;
        float period = 0.0f;
        bool voiced = false;
    };

    void processChunk (const float* monoIn, float* monoOut, int numSamples);
    void placeAnalysisMarks();
    void renderReadyGrains();

    float readInput (double sourcePos) const noexcept;      // linear interpolation, clamped
    float readInputAt (juce::int64 pos) const noexcept;     // integer read, zero outside history
    int nearestMarkIndex (double pos) noexcept;
    const PitchMark& markAt (int index) const noexcept
    {
        return marks[(size_t) ((markHead + index) % markCapacity)];
    }
    void pushMark (const PitchMark& m);

    // configuration
    double sampleRate = 48000.0;
    int numChannels = 2;
    int maxBlock = 512;
    int latencySamples = 0;
    int maxPeriod = 800, minPeriod = 48, fallbackPeriod = 240;
    int ringMask = 0;

    // parameters
    float pitchRatioTarget = 1.0f, formantRatioTarget = 1.0f;
    float pitchRatioSm = 1.0f, formantRatioSm = 1.0f;
    float detectedPeriod = 240.0f;
    float periodSm = 240.0f;
    bool voiced = false;

    // rings (absolute sample positions masked into power-of-two buffers)
    std::vector<float> inputRing, accumRing, wsumRing;
    juce::int64 inWrite = 0;   // total input samples written
    juce::int64 outRead = 0;   // next output position to emit (inWrite - latency)

    // pitch-mark ring
    static constexpr int markCapacity = 1024;
    std::vector<PitchMark> marks;
    int markHead = 0, markCount = 0, searchHint = 0;
    juce::int64 nextAnalysisPos = 0;
    juce::int64 lastVoicedMark = -1;
    double nextSynthPos = 0.0;

    std::vector<float> monoScratch, outScratch;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchFormantEngine)
};
