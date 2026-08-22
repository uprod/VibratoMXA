#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

namespace vibratomxa
{

// FIG. 2 - Le chemin du signal dessine comme dans le manuel : IN, ligne a
// retard dont le temps de lecture imprime est la vraie valeur (6.0 ms
// +/- depth x 6.0 ms), interpolateur Lagrange du 3e ordre, LFO (glyphe de la
// forme d'onde SELECTIONNEE, point de phase reel), rails dry/wet ponderes par
// le mix. La quantite est dessinee en geometrie : le schema est la valeur.
class SchematicDiagram : public juce::Component
{
public:
    explicit SchematicDiagram (VibratoProcessor&);

    void paint (juce::Graphics&) override;

private:
    VibratoProcessor& processor;

    std::atomic<float>* depth    = nullptr;
    std::atomic<float>* waveform = nullptr;
    std::atomic<float>* stereo   = nullptr;
    std::atomic<float>* mix      = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SchematicDiagram)
};

}
