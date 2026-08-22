#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <cmath>

namespace vibratomxa
{

// Coeur DSP du vibrato. Le principe : on fait passer le son dans une ligne a
// retard (delay) dont le temps varie au rythme d'un oscillateur (LFO). Quand le
// delai s'allonge puis se raccourcit, la forme d'onde est etiree/comprimee, ce
// qui fait monter et descendre la hauteur du son (effet Doppler) -> c'est le
// vibrato. L'interpolation Lagrange permet des temps de retard fractionnaires,
// indispensable pour une variation de hauteur lisse.
class VibratoEngine
{
public:
    VibratoEngine();

    void prepare (double sampleRate, int blockSize, int numChannels);
    void reset();

    void setRateHz (float hz);
    void setDepth (float amount01);
    void setWaveform (int type);       // 0 = sinus, 1 = triangle
    void setStereoWidth (float amount01);

    // Traite le buffer en place (remplace le son par sa version vibree).
    void process (juce::AudioBuffer<float>& buffer);

    // Le delai oscille autour de 'centerMs', d'une amplitude max de 'maxDepthMs'.
    // Le centre vaut maxDepthMs pour que le delai ne devienne jamais negatif.
    // Publics : l'UI (FIG. 2) imprime le vrai temps de retard sur le schema.
    static constexpr float centerMs   = 6.0f;
    static constexpr float maxDepthMs = 6.0f;

    // Valeur du LFO pour une forme d'onde et une phase donnees, dans [-1, 1].
    // Partagee avec l'UI : une seule source de verite pour la forme tracee.
    static float lfoValueFor (int waveformType, float phase01) noexcept
    {
        if (waveformType == 1)
            return 1.0f - 4.0f * std::abs (phase01 - 0.5f);   // triangle, dans [-1, 1]

        return std::sin (phase01 * juce::MathConstants<float>::twoPi);
    }

    // Deviation de hauteur instantanee (en cents) du chemin retarde, pour FIG. 1.
    // Meme physique que process() : delai(t) = centre + depth*maxDepth*lfo(ph),
    // rapport de hauteur = 1 - d(delai)/dt (effet Doppler de la ligne a retard).
    static float pitchDeviationCents (float depth01, float rateHzIn,
                                      int waveformType, float phase01) noexcept
    {
        float ph = phase01 - std::floor (phase01);
        const float depthSec = depth01 * maxDepthMs * 0.001f;

        // Pente du LFO par unite de phase : d(lfo)/d(ph).
        const float slope = (waveformType == 1)
            ? (ph < 0.5f ? 4.0f : -4.0f)
            : juce::MathConstants<float>::twoPi
                  * std::cos (ph * juce::MathConstants<float>::twoPi);

        const float ratio = 1.0f - depthSec * slope * rateHzIn;
        return 1200.0f * std::log2 (juce::jmax (ratio, 0.05f));
    }

    // Phase du LFO publiee pour l'UI (lecture sans verrou, jamais bloquante).
    float getLfoPhase01() const noexcept { return uiPhase.load (std::memory_order_relaxed); }

private:
    float lfoValue (float phase01) const;

    double sampleRate = 44100.0;
    int    numCh = 2;

    float rateHz      = 5.0f;
    float depth       = 0.4f;
    int   waveform    = 0;
    float stereoWidth = 0.0f;

    float lfoPhase = 0.0f;   // phase de l'oscillateur, 0..1

    std::atomic<float> uiPhase { 0.0f };   // copie de lfoPhase pour l'affichage

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLine { 1 << 16 };
};

}
