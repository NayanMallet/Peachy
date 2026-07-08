#include "TubeSaturator.h"

#include <cmath>

float TubeSaturator::triodeShape (float v) noexcept
{
    // Asymmetric triode-style transfer: the positive half compresses gently
    // (plate saturation), the negative half hits an earlier, harder knee
    // (grid conduction). Both halves keep unity slope at the origin, so the
    // asymmetry adds even harmonics without a level step.
    if (v >= 0.0f)
        return std::tanh (v);

    constexpr float kneeHardness = 2.1f;
    return std::tanh (kneeHardness * v) * (1.0f / kneeHardness);
}

void TubeSaturator::prepare (const juce::dsp::ProcessSpec& spec)
{
    baseRate = spec.sampleRate;

    // Polyphase IIR halfbands keep latency low; integer latency makes the
    // total plugin latency exact for host delay compensation
    oversampling = std::make_unique<juce::dsp::Oversampling<float>> (
        (size_t) spec.numChannels,
        oversamplingOrder,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true,
        true);

    oversampling->initProcessing (spec.maximumBlockSize);

    // Envelope follower at the oversampled rate: ~1ms attack, ~60ms release
    const double osRate = spec.sampleRate * 4.0;
    envAttackCoeff = 1.0f - (float) std::exp (-1.0 / (0.0012 * osRate));
    envReleaseCoeff = 1.0f - (float) std::exp (-1.0 / (0.060 * osRate));

    // Matched pre/de-emphasis pair (exact inverses, so drive at 0 stays flat)
    auto preCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
        spec.sampleRate, emphasisFreq, 0.8f, juce::Decibels::decibelsToGain (emphasisDb));
    auto deCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
        spec.sampleRate, emphasisFreq, 0.8f, juce::Decibels::decibelsToGain (-emphasisDb));

    for (int ch = 0; ch < maxChannels; ++ch)
    {
        preEmphasis[(size_t) ch].coefficients = preCoeffs;
        deEmphasis[(size_t) ch].coefficients = deCoeffs;
    }

    // Make-up smoothing time constants, expressed per block
    const double blocksPerSecond = spec.sampleRate / (double) juce::jmax (1u, spec.maximumBlockSize);
    inputLevelDecay = (float) std::exp (-1.0 / (0.150 * blocksPerSecond));
    makeupSmoothing = 1.0f - (float) std::exp (-1.0 / (0.080 * blocksPerSecond));

    // One-pole DC blocker at ~15Hz
    dcCoeff = juce::jlimit (0.9f, 0.9999f,
                            1.0f - (float) (juce::MathConstants<double>::twoPi * 15.0 / spec.sampleRate));

    reset();
}

void TubeSaturator::reset()
{
    if (oversampling != nullptr)
        oversampling->reset();

    for (auto& f : preEmphasis)
        f.reset();
    for (auto& f : deEmphasis)
        f.reset();

    envelope.fill (0.0f);
    inputLevel = 0.0f;
    makeupSm = 1.0f;
    dcX1.fill (0.0f);
    dcY1.fill (0.0f);
}

float TubeSaturator::getLatencySamples() const
{
    return oversampling != nullptr ? oversampling->getLatencyInSamples() : 0.0f;
}

void TubeSaturator::process (juce::AudioBuffer<float>& buffer)
{
    if (oversampling == nullptr)
        return; // prepare() hasn't run

    const int numSamples = buffer.getNumSamples();
    const int numChannels = juce::jmin (buffer.getNumChannels(), maxChannels);

    // ---- pre-emphasis (base rate): push the upper mids into the tube ----
    float blockPeak = 0.0f;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* data = buffer.getWritePointer (ch);
        auto& filter = preEmphasis[(size_t) ch];

        for (int i = 0; i < numSamples; ++i)
        {
            const float y = filter.processSample (data[i]);
            data[i] = y;
            blockPeak = juce::jmax (blockPeak, std::abs (y));
        }

        filter.snapToZero();
    }

    inputLevel = juce::jmax (blockPeak, inputLevel * inputLevelDecay);

    // ---- the tube stage, at 4x (always processed: latency stays constant) ----
    juce::dsp::AudioBlock<float> block (buffer);
    auto upsampled = oversampling->processSamplesUp (block);

    const float gain = juce::Decibels::decibelsToGain (drive01 * maxDriveDb);
    const float biasDrive = biasAmount * drive01;

    for (size_t ch = 0; ch < upsampled.getNumChannels() && ch < (size_t) maxChannels; ++ch)
    {
        float* data = upsampled.getChannelPointer (ch);
        float env = envelope[ch];
        const size_t osSamples = upsampled.getNumSamples();

        for (size_t i = 0; i < osSamples; ++i)
        {
            const float x = data[i];
            const float ax = std::abs (x);

            // dynamic bias: the operating point rides the vocal envelope
            env += (ax > env ? envAttackCoeff : envReleaseCoeff) * (ax - env);
            const float bias = biasDrive * env;

            data[i] = triodeShape (gain * (x + bias)) - triodeShape (gain * bias);
        }

        envelope[ch] = env;
    }

    oversampling->processSamplesDown (block);

    // ---- adaptive make-up: probe the static curve with a sine at the tracked
    //      input level and rescale so perceived RMS stays constant ----
    {
        const float amp = juce::jlimit (1.0e-3f, 1.5f, inputLevel);
        const float bias = biasDrive * amp * 0.9f;

        float acc = 0.0f;
        for (int k = 0; k < 16; ++k)
        {
            const float phase = (juce::MathConstants<float>::twoPi * ((float) k + 0.5f)) / 16.0f;
            const float probe = amp * std::sin (phase);
            const float shaped = triodeShape (gain * (probe + bias)) - triodeShape (gain * bias);
            acc += shaped * shaped;
        }

        const float outRms = std::sqrt (acc / 16.0f + 1.0e-12f);
        const float inRms = amp * 0.70710678f;

        // The probe models the shaper but not the de-emphasis stage or the DC
        // blocker, which both shave energy it can't see (upper-mid distortion
        // products, bias-induced offset) — compensate proportionally to drive
        // so perceived loudness stays flat across the knob. Tuned by ear + RMS
        // measurement against the drive-at-zero path.
        const float deEmphasisLoss = 1.0f + 0.22f * drive01;
        const float target = juce::jlimit (0.25f, 8.0f, deEmphasisLoss * inRms / juce::jmax (1.0e-6f, outRms));
        makeupSm += makeupSmoothing * (target - makeupSm);
    }

    buffer.applyGain (makeupSm);

    // ---- de-emphasis (base rate): pull the tilt back out, keep the harmonics ----
    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* data = buffer.getWritePointer (ch);
        auto& filter = deEmphasis[(size_t) ch];

        for (int i = 0; i < numSamples; ++i)
            data[i] = filter.processSample (data[i]);

        filter.snapToZero();
    }

    // ---- DC blocker: asymmetry + bias shift the average level ----
    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* data = buffer.getWritePointer (ch);
        float x1 = dcX1[(size_t) ch];
        float y1 = dcY1[(size_t) ch];

        for (int i = 0; i < numSamples; ++i)
        {
            const float x = data[i];
            const float y = x - x1 + dcCoeff * y1;
            data[i] = y;
            x1 = x;
            y1 = y;
        }

        dcX1[(size_t) ch] = x1;
        dcY1[(size_t) ch] = y1;
    }
}
