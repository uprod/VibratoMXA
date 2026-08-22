#include "VibratoEngine.h"

namespace vibratomxa
{

VibratoEngine::VibratoEngine() = default;

void VibratoEngine::prepare (double newSampleRate, int blockSize, int numChannels)
{
    sampleRate = newSampleRate;
    numCh = juce::jmax (1, numChannels);

    juce::dsp::ProcessSpec spec { sampleRate,
                                  (juce::uint32) juce::jmax (1, blockSize),
                                  (juce::uint32) numCh };
    delayLine.prepare (spec);

    // Retard maximal = centre + amplitude max, plus une petite marge de securite.
    const float maxDelaySamples = (centerMs + maxDepthMs) * 0.001f * (float) sampleRate + 4.0f;
    delayLine.setMaximumDelayInSamples ((int) maxDelaySamples);

    reset();
}

void VibratoEngine::reset()
{
    delayLine.reset();
    lfoPhase = 0.0f;
}

void VibratoEngine::setRateHz (float hz)        { rateHz      = juce::jlimit (0.05f, 20.0f, hz); }
void VibratoEngine::setDepth (float a)          { depth       = juce::jlimit (0.0f, 1.0f, a); }
void VibratoEngine::setWaveform (int type)      { waveform    = type; }
void VibratoEngine::setStereoWidth (float a)    { stereoWidth = juce::jlimit (0.0f, 1.0f, a); }

float VibratoEngine::lfoValue (float ph) const
{
    return lfoValueFor (waveform, ph);   // meme math que l'UI (FIG. 1 / FIG. 2)
}

void VibratoEngine::process (juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int chs        = juce::jmin (numCh, buffer.getNumChannels());

    const float inc           = rateHz / (float) sampleRate;          // avance de phase par echantillon
    const float centerSamples = centerMs * 0.001f * (float) sampleRate;
    const float depthSamples  = depth * maxDepthMs * 0.001f * (float) sampleRate;

    // Decalage de phase entre canaux : jusqu'a un demi-cycle (180 degres) pour un
    // mouvement stereo large quand on monte le reglage Stereo.
    const float chPhaseOffset = stereoWidth * 0.5f;

    for (int n = 0; n < numSamples; ++n)
    {
        for (int ch = 0; ch < chs; ++ch)
        {
            float ph = lfoPhase + (float) ch * chPhaseOffset;
            ph -= std::floor (ph);                                    // ramene dans [0, 1)

            const float lfo = lfoValue (ph);
            float delaySamples = centerSamples + depthSamples * lfo;
            if (delaySamples < 1.0f) delaySamples = 1.0f;

            const float in = buffer.getSample (ch, n);
            delayLine.pushSample (ch, in);
            buffer.setSample (ch, n, delayLine.popSample (ch, delaySamples));
        }

        lfoPhase += inc;
        if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
    }

    uiPhase.store (lfoPhase, std::memory_order_relaxed);   // pour FIG. 1 / FIG. 2
}

}
