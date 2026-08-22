#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

namespace vibratomxa
{

// FIG. 1 - Deviation de hauteur, tracee comme la figure d'un manuel technique.
// Ce n'est pas une illustration : la courbe est la vraie deviation Doppler du
// chemin retarde (rapport de hauteur = 1 - d(delai)/dt) evaluee a la phase LFO
// reelle du moteur, en mode "defilement" : le bord droit du cadre est
// l'instant present. Le repaint est pilote par le Timer de l'editeur (~30 Hz).
class DeviationPlot : public juce::Component
{
public:
    explicit DeviationPlot (VibratoProcessor&);

    void paint (juce::Graphics&) override;

private:
    VibratoProcessor& processor;

    std::atomic<float>* rate     = nullptr;
    std::atomic<float>* depth    = nullptr;
    std::atomic<float>* waveform = nullptr;
    std::atomic<float>* stereo   = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DeviationPlot)
};

}
