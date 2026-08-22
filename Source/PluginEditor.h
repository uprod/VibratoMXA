#pragma once

/*  IMPECCABLE DIRECTION CONTRACT — seed 5bcea053 (roll: assigned)
    THESIS: The panel IS the signal path — a service-manual schematic read as
    the circuit you hear; refuses knobs-on-a-metal-plate.
    OWN-WORLD: Diazo film negative — dark drafting film #17140F, pale ink
    #E6DCC2, spot violet #9D7BFF (one spot ink per MXA sibling). Routed Gothic
    drafting lettering + Courier Prime figures, double sheet border, title
    block, FIG. captions.
    STORY: A producer reads the schematic, watches the true pitch deviation
    roll past in FIG. 1, and trusts every figure at a glance.
    FIRST VIEWPORT: Header + title block; FIG. 1 live pitch-deviation trace
    full width; FIG. 2 signal path with the real modulated delay line,
    Lagrange interpolator, weighted DRY/WET rails, LFO phase dot; five
    schematic dials beneath.
    SIGNATURE: the roll — FIG. 1's trace and the LFO dot on one 30 Hz clock,
    driven by the engine's real phase.
    FORM: Service Manual family template, adopted from PhaserMXA, seed 5bcea053.
    FINISH: unreviewed and undocumented is unfinished; this build ends with
    the finish review, the verdict, DESIGN.md, and every shipping raster
    carrying its provenance.
*/

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "ManualStyle.h"
#include "DeviationPlot.h"
#include "SchematicDiagram.h"

namespace vibratomxa
{

class VibratoEditor : public juce::AudioProcessorEditor,
                      private juce::Timer
{
public:
    explicit VibratoEditor (VibratoProcessor& proc);
    ~VibratoEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent& e) override;

private:
    using APVTS   = juce::AudioProcessorValueTreeState;
    using SAttach = APVTS::SliderAttachment;

    struct Dial
    {
        juce::Slider slider;
        juce::Label  name;
        std::unique_ptr<SAttach> attachment;
    };

    void setupDial (Dial& d, const juce::String& labelText, const juce::String& paramID);
    void timerCallback() override;

    void drawSheetFrame (juce::Graphics& g);
    void drawHeader (juce::Graphics& g);

    VibratoProcessor& processor;

    ManualLookAndFeel lookAndFeel;
    juce::Image       filmTexture;

    DeviationPlot    plot;
    SchematicDiagram schematic;

    Dial rateDial, depthDial, waveSwitch, stereoDial, mixDial;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VibratoEditor)
};

}
