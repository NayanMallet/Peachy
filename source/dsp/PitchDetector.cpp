#include "PitchDetector.h"

#include <cmath>

void PitchDetector::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;

    // The correlation only needs to reach one period of the lowest trackable
    // voice; the window is twice that. A short ~5ms hop gives the hard-tune
    // modes their near-instant retune response.
    maxLag = juce::jmax (32, (int) std::ceil (sampleRate / (double) minFrequencyHz)) + 2;
    windowSize = juce::nextPowerOfTwo (2 * maxLag);
    minLag = juce::jmax (2, (int) (sampleRate / (double) maxFrequencyHz));
    hopSize = juce::jmax (64, (int) std::lround (sampleRate * 0.005));

    ring.assign ((size_t) windowSize, 0.0f);
    window.assign ((size_t) windowSize, 0.0f);
    cmndf.assign ((size_t) maxLag, 0.0f);

    reset();
}

void PitchDetector::reset()
{
    std::fill (ring.begin(), ring.end(), 0.0f);
    writePos = 0;
    samplesSinceAnalysis = 0;
    samplesInRing = 0;
    pitchHz = -1.0f;
    lastVoicedLag = -1.0f;
    unvoicedFrames = 0;
}

void PitchDetector::processBlock (const juce::AudioBuffer<float>& buffer)
{
    if (windowSize == 0)
        return; // prepare() hasn't run

    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    if (numChannels == 0)
        return;

    const float* const* channels = buffer.getArrayOfReadPointers();
    const float channelScale = 1.0f / (float) numChannels;
    float* ringData = ring.data();

    for (int i = 0; i < numSamples; ++i)
    {
        float mono = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            mono += channels[ch][i];

        ringData[writePos] = mono * channelScale;

        if (++writePos == windowSize)
            writePos = 0;

        samplesInRing = juce::jmin (samplesInRing + 1, windowSize);

        if (++samplesSinceAnalysis >= hopSize && samplesInRing >= windowSize)
        {
            samplesSinceAnalysis = 0;
            analyze();
        }
    }
}

void PitchDetector::analyze()
{
    // Unroll the ring so the frame reads oldest -> newest
    float* w = window.data();
    const float* r = ring.data();

    for (int i = 0; i < windowSize; ++i)
    {
        int idx = writePos + i;
        if (idx >= windowSize)
            idx -= windowSize;
        w[i] = r[idx];
    }

    double energy = 0.0;
    for (int i = 0; i < windowSize; ++i)
        energy += (double) w[i] * (double) w[i];

    if (energy / (double) windowSize < silenceGate)
    {
        pitchHz = -1.0f;
        if (++unvoicedFrames > 25)
            lastVoicedLag = -1.0f;
        return;
    }

    // Difference function + cumulative mean normalisation (YIN steps 2-3)
    float* c = cmndf.data();
    c[0] = 1.0f;
    float runningSum = 0.0f;

    for (int lag = 1; lag < maxLag; ++lag)
    {
        float sum = 0.0f;
        const float* a = w;
        const float* b = w + lag;

        for (int i = 0; i < maxLag; ++i)
        {
            const float d = a[i] - b[i];
            sum += d * d;
        }

        runningSum += sum;
        c[lag] = runningSum > 0.0f ? sum * (float) lag / runningSum : 1.0f;
    }

    // Absolute threshold (step 4): first dip below threshold, walked to its local minimum
    int bestLag = -1;

    for (int lag = minLag; lag < maxLag - 1; ++lag)
    {
        if (c[lag] < yinThreshold)
        {
            while (lag + 1 < maxLag - 1 && c[lag + 1] < c[lag])
                ++lag;

            bestLag = lag;
            break;
        }
    }

    if (bestLag <= 0)
    {
        pitchHz = -1.0f;
        if (++unvoicedFrames > 25)
            lastVoicedLag = -1.0f;
        return;
    }

    // Octave-jump guard: if the winning lag jumped far from the last stable
    // voiced lag but a comparable dip exists near it, prefer continuity.
    // Keeps sustained notes (and hard-tune corrections) from flickering.
    if (lastVoicedLag > 0.0f)
    {
        const float jump = (float) bestLag / lastVoicedLag;

        if (jump > 1.6f || jump < 0.625f)
        {
            const int lo = juce::jmax (minLag, (int) (lastVoicedLag * 0.82f));
            const int hi = juce::jmin (maxLag - 2, (int) (lastVoicedLag * 1.22f));
            int localMin = -1;

            for (int lag = lo; lag <= hi; ++lag)
                if (localMin < 0 || c[lag] < c[localMin])
                    localMin = lag;

            if (localMin > 0 && c[localMin] < juce::jmax (yinThreshold * 1.35f, c[bestLag] * 1.3f))
                bestLag = localMin;
        }
    }

    // Parabolic interpolation around the minimum (step 5)
    float refinedLag = (float) bestLag;

    if (bestLag > 0 && bestLag < maxLag - 1)
    {
        const float y0 = c[bestLag - 1];
        const float y1 = c[bestLag];
        const float y2 = c[bestLag + 1];
        const float denom = y0 - 2.0f * y1 + y2;

        if (std::abs (denom) > 1.0e-12f)
            refinedLag += 0.5f * (y0 - y2) / denom;
    }

    const float f0 = (float) (sampleRate / (double) refinedLag);

    if (f0 >= minFrequencyHz && f0 <= maxFrequencyHz)
    {
        pitchHz = f0;
        lastVoicedLag = refinedLag;
        unvoicedFrames = 0;
    }
    else
    {
        pitchHz = -1.0f;
        if (++unvoicedFrames > 25)
            lastVoicedLag = -1.0f;
    }
}
