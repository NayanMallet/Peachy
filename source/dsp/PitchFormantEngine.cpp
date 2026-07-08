#include "PitchFormantEngine.h"

#include <cmath>

namespace
{
    constexpr float minF0Hz = 60.0f;   // lowest trackable voice
    constexpr float maxF0Hz = 1000.0f;

    // Engine-side safety clamps (the processor already limits its requests)
    constexpr float minPitchRatio = 0.25f;
    constexpr float maxPitchRatio = 4.0f;
    constexpr float minFormantRatio = 0.5f;
    constexpr float maxFormantRatio = 2.0f;

    int nextPowerOfTwo (int n)
    {
        int v = 1;
        while (v < n)
            v <<= 1;
        return v;
    }
}

void PitchFormantEngine::prepare (double newSampleRate, int newNumChannels, int newMaxBlockSize)
{
    sampleRate = newSampleRate;
    numChannels = juce::jlimit (1, 2, newNumChannels);
    maxBlock = juce::jmax (16, newMaxBlockSize);

    maxPeriod = (int) std::ceil (sampleRate / (double) minF0Hz);
    minPeriod = juce::jmax (16, (int) std::floor (sampleRate / (double) maxF0Hz));
    fallbackPeriod = juce::jmax (minPeriod, (int) std::lround (sampleRate * 0.005));

    // Grain rendering for a synthesis mark at time s needs input up to
    // s + period*(1 + formantRatio) <= s + 3*maxPeriod, so with output lagging
    // input by 3*maxPeriod every grain is renderable exactly when it is due.
    latencySamples = 3 * maxPeriod + 32;

    const int ringSize = nextPowerOfTwo (latencySamples + 4 * maxPeriod + maxBlock + 256);
    ringMask = ringSize - 1;
    inputRing.assign ((size_t) ringSize, 0.0f);
    accumRing.assign ((size_t) ringSize, 0.0f);
    wsumRing.assign ((size_t) ringSize, 0.0f);

    marks.assign ((size_t) markCapacity, {});
    monoScratch.assign ((size_t) maxBlock, 0.0f);
    outScratch.assign ((size_t) maxBlock, 0.0f);

    reset();
}

void PitchFormantEngine::reset()
{
    std::fill (inputRing.begin(), inputRing.end(), 0.0f);
    std::fill (accumRing.begin(), accumRing.end(), 0.0f);
    std::fill (wsumRing.begin(), wsumRing.end(), 0.0f);

    inWrite = 0;
    outRead = -(juce::int64) latencySamples;
    markHead = 0;
    markCount = 0;
    searchHint = 0;
    nextAnalysisPos = 0;
    lastVoicedMark = -1;
    nextSynthPos = 0.0;
    periodSm = (float) fallbackPeriod;
    detectedPeriod = (float) fallbackPeriod;
    voiced = false;
    pitchRatioSm = pitchRatioTarget;
    formantRatioSm = formantRatioTarget;
}

void PitchFormantEngine::setTransposeSemitones (float semitones)
{
    pitchRatioTarget = juce::jlimit (minPitchRatio, maxPitchRatio, std::exp2 (semitones / 12.0f));
}

void PitchFormantEngine::setFormantSemitones (float semitones)
{
    formantRatioTarget = juce::jlimit (minFormantRatio, maxFormantRatio, std::exp2 (semitones / 12.0f));
}

void PitchFormantEngine::setDetectedPitch (float hz) noexcept
{
    voiced = hz > 0.0f;

    if (voiced)
        detectedPeriod = juce::jlimit ((float) minPeriod, (float) maxPeriod,
                                       (float) (sampleRate / (double) hz));
}

void PitchFormantEngine::process (juce::AudioBuffer<float>& buffer)
{
    const int total = buffer.getNumSamples();
    const int chans = juce::jmin (buffer.getNumChannels(), numChannels);

    if (total == 0 || chans == 0 || ringMask == 0)
        return;

    int done = 0;

    while (done < total)
    {
        const int chunk = juce::jmin (maxBlock, total - done);

        // strictly monophonic processing: sum the input to mono...
        float* mono = monoScratch.data();
        const float scale = 1.0f / (float) chans;

        for (int i = 0; i < chunk; ++i)
        {
            float s = 0.0f;
            for (int ch = 0; ch < chans; ++ch)
                s += buffer.getReadPointer (ch)[done + i];
            mono[i] = s * scale;
        }

        processChunk (mono, outScratch.data(), chunk);

        // ...and write the processed voice to every output channel
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            std::copy (outScratch.data(), outScratch.data() + chunk, buffer.getWritePointer (ch) + done);

        done += chunk;
    }
}

void PitchFormantEngine::processChunk (const float* monoIn, float* monoOut, int n)
{
    // 1) append input to the history ring
    for (int i = 0; i < n; ++i)
        inputRing[(size_t) ((inWrite + (juce::int64) i) & (juce::int64) ringMask)] = monoIn[i];
    inWrite += n;

    // 2) extend the analysis mark grid, then render every synthesis grain
    //    whose input context is now available
    placeAnalysisMarks();
    renderReadyGrains();

    // 3) emit output, normalising by the accumulated window sum (exact COLA)
    for (int i = 0; i < n; ++i)
    {
        float v = 0.0f;

        if (outRead >= 0)
        {
            const auto idx = (size_t) (outRead & (juce::int64) ringMask);
            const float ws = wsumRing[idx];
            v = ws > 1.0e-4f ? accumRing[idx] / ws : accumRing[idx];
            accumRing[idx] = 0.0f;
            wsumRing[idx] = 0.0f;
        }

        monoOut[i] = v;
        ++outRead;
    }
}

void PitchFormantEngine::placeAnalysisMarks()
{
    for (;;)
    {
        // track the detected period (fast when voiced, drifting back to the
        // fallback spacing across unvoiced stretches)
        if (voiced)
            periodSm += 0.4f * (detectedPeriod - periodSm);
        else
            periodSm += 0.05f * ((float) fallbackPeriod - periodSm);

        const float p = juce::jlimit ((float) minPeriod, (float) maxPeriod, periodSm);

        const int refineRadius = voiced ? juce::jlimit (2, 160, (int) (p * 0.18f)) : 0;
        const int corrHalf = voiced ? juce::jmin ((int) (p * 0.5f), 400) : 0;

        if (nextAnalysisPos + (juce::int64) (refineRadius + corrHalf + 2) > inWrite - 1)
            break; // not enough lookahead yet

        juce::int64 markPos = nextAnalysisPos;

        // Phase-align the new mark to the previous voiced one by maximising the
        // local cross-correlation: this is what keeps neighbouring grains
        // coherent and the shifted voice free of graininess.
        if (voiced && lastVoicedMark >= 0 && corrHalf > 8
            && markPos - lastVoicedMark < (juce::int64) (3.0f * p))
        {
            double best = -1.0e30;
            int bestOffset = 0;

            for (int d = -refineRadius; d <= refineRadius; ++d)
            {
                double acc = 0.0;
                for (int k = -corrHalf; k <= corrHalf; k += 2)
                    acc += (double) readInputAt (lastVoicedMark + (juce::int64) k)
                         * (double) readInputAt (markPos + (juce::int64) (d + k));

                if (acc > best)
                {
                    best = acc;
                    bestOffset = d;
                }
            }

            markPos += (juce::int64) bestOffset;
        }

        pushMark ({ markPos, p, voiced });

        if (voiced)
            lastVoicedMark = markPos;

        nextAnalysisPos = markPos + (juce::int64) std::lround ((double) p);
    }
}

void PitchFormantEngine::renderReadyGrains()
{
    // safety: never let the synthesis head fall behind the output head
    if (nextSynthPos < (double) (outRead - (juce::int64) maxPeriod))
        nextSynthPos = (double) juce::jmax ((juce::int64) 0, outRead);

    for (;;)
    {
        // drop marks that can no longer contribute
        while (markCount > 0
               && markAt (0).position < outRead - (juce::int64) (2 * maxPeriod))
        {
            markHead = (markHead + 1) % markCapacity;
            --markCount;
            if (searchHint > 0)
                --searchHint;
        }

        if (markCount < 2)
            return;

        // wait until the mark grid extends past the synthesis head AND the
        // nearest mark has a successor, so both the mapping and the *actual*
        // local mark spacing are settled choices
        const int nearestIdx = nearestMarkIndex (nextSynthPos);

        if (nearestIdx >= markCount - 1
            || (double) markAt (markCount - 1).position < nextSynthPos)
            return;

        const auto& a = markAt (nearestIdx);
        const auto& following = markAt (nearestIdx + 1);

        // The distance the synthesis head must cover to stay in lockstep with
        // the analysis grid is the *measured* spacing between marks — not the
        // nominal period, which the correlation refinement can shift by ±18%.
        // Using the nominal value made the nearest-mark mapping repeat/skip
        // grains even at ratio 1, garbling the voice.
        const float localSpacing = juce::jlimit ((float) minPeriod, (float) (2 * maxPeriod),
                                                 (float) (following.position - a.position));

        // grain long enough to always bridge the gap to the next mark
        const float halfLen = juce::jlimit ((float) minPeriod, (float) maxPeriod,
                                            juce::jmax (a.period, localSpacing));

        // availability check first (worst-case formant step), so an early return
        // never double-applies the per-grain parameter smoothing below
        const float worstStep = juce::jmax (formantRatioTarget, formantRatioSm);
        if (a.position + (juce::int64) std::ceil ((double) halfLen * (double) worstStep) + 2 > inWrite - 1)
            return; // grain source not fully available yet

        // per-grain parameter smoothing: fast enough for near-instant hard-tune
        // (a few pitch periods), yet click-free because every step happens on a
        // windowed grain boundary
        formantRatioSm += 0.3f * (formantRatioTarget - formantRatioSm);
        const float grainRatioTarget = a.voiced ? pitchRatioTarget : 1.0f;
        pitchRatioSm += 0.6f * (grainRatioTarget - pitchRatioSm);

        const int halfLenInt = (int) halfLen;
        const float formantStep = formantRatioSm;
        const float ratio = juce::jmax (minPitchRatio, pitchRatioSm);

        const auto outCentre = (juce::int64) std::llround (nextSynthPos);
        const float invHalfLen = 1.0f / halfLen;

        for (int k = -halfLenInt; k <= halfLenInt; ++k)
        {
            const auto op = outCentre + (juce::int64) k;
            if (op < 0 || op < outRead)
                continue;

            // Hann window over [-halfLen, halfLen]; reading the source with a
            // step of formantStep rescales the spectral envelope (formants)
            // without touching the synthesis pitch grid
            const float w = 0.5f + 0.5f * std::cos (juce::MathConstants<float>::pi * (float) k * invHalfLen);
            const float s = readInput ((double) a.position + (double) k * (double) formantStep);

            const auto idx = (size_t) (op & (juce::int64) ringMask);
            accumRing[idx] += w * s;
            wsumRing[idx] += w;
        }

        // Phase-locked advance. Near ratio 1 the synthesis grid is pulled onto
        // the analysis grid (the offset `err` decays to zero), which makes the
        // overlap-add reconstruction *exact*: output = input delayed by the
        // reported latency — perfect phase coherence, transients untouched.
        // The lock fades out above ~3% detune so it never biases a real shift.
        const double err = nextSynthPos - (double) a.position;
        const double lock = juce::jmax (0.0, 1.0 - std::abs ((double) ratio - 1.0) * 33.0) * 0.5;

        nextSynthPos += (double) localSpacing / (double) ratio - err * lock;
    }
}

float PitchFormantEngine::readInput (double sourcePos) const noexcept
{
    const double newest = (double) (inWrite - 1);
    const double oldest = juce::jmax (0.0, (double) (inWrite - (juce::int64) (ringMask - 8)));
    const double p = juce::jlimit (oldest, juce::jmax (oldest, newest), sourcePos);

    const auto i0 = (juce::int64) p;
    const float frac = (float) (p - (double) i0);
    const float s0 = inputRing[(size_t) (i0 & (juce::int64) ringMask)];
    const float s1 = inputRing[(size_t) ((i0 + 1) & (juce::int64) ringMask)];
    return s0 + frac * (s1 - s0);
}

float PitchFormantEngine::readInputAt (juce::int64 pos) const noexcept
{
    if (pos < 0 || pos >= inWrite || pos < inWrite - (juce::int64) (ringMask - 8))
        return 0.0f;

    return inputRing[(size_t) (pos & (juce::int64) ringMask)];
}

int PitchFormantEngine::nearestMarkIndex (double pos) noexcept
{
    searchHint = juce::jlimit (0, markCount - 1, searchHint);

    auto distance = [this, pos] (int i)
    {
        return std::abs ((double) markAt (i).position - pos);
    };

    while (searchHint + 1 < markCount && distance (searchHint + 1) <= distance (searchHint))
        ++searchHint;

    return searchHint;
}

void PitchFormantEngine::pushMark (const PitchMark& m)
{
    if (markCount == markCapacity)
    {
        markHead = (markHead + 1) % markCapacity;
        --markCount;
        if (searchHint > 0)
            --searchHint;
    }

    marks[(size_t) ((markHead + markCount) % markCapacity)] = m;
    ++markCount;
}
