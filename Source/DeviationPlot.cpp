#include "DeviationPlot.h"
#include "ManualStyle.h"
#include "VibratoEngine.h"

namespace vibratomxa
{

namespace
{
    constexpr int kPoints = 220;

    // Echelle verticale adaptative : le plus petit palier "propre" qui
    // contient la deviation crete reelle (depth et rate courants).
    float centsLimitFor (float peakCents)
    {
        static const float ladder[] = { 10.0f, 25.0f, 50.0f, 100.0f,
                                        250.0f, 500.0f, 1000.0f, 1500.0f };
        for (const float l : ladder)
            if (peakCents <= l)
                return l;
        return 1500.0f;
    }

    juce::String centsLabel (float v)
    {
        const juce::String sign = v > 0.0f ? "+" : "";
        if (std::abs (v - std::round (v)) < 0.01f)
            return sign + juce::String ((int) std::round (v));
        return sign + juce::String (v, 1);
    }
}

DeviationPlot::DeviationPlot (VibratoProcessor& proc)
    : processor (proc)
{
    auto& apvts = processor.getAPVTS();
    rate     = apvts.getRawParameterValue ("rate");
    depth    = apvts.getRawParameterValue ("depth");
    waveform = apvts.getRawParameterValue ("waveform");
    stereo   = apvts.getRawParameterValue ("stereo");

    setInterceptsMouseClicks (false, false);
}

void DeviationPlot::paint (juce::Graphics& g)
{
    auto full = getLocalBounds().toFloat();
    auto caption = full.removeFromBottom (16.0f);
    auto box = full;

    const float rateV   = rate->load();
    const float depthV  = depth->load();
    const int   waveV   = juce::roundToInt (waveform->load());
    const float stereoV = stereo->load();
    const float phase   = processor.getLfoPhase01();

    // Fenetre de defilement : deux periodes du LFO, le present au bord droit.
    const float periodS = 1.0f / juce::jmax (0.01f, rateV);
    const float windowS = 2.0f * periodS;

    // Deviation crete reelle : pente max du LFO x profondeur x vitesse.
    const float depthSec  = depthV * VibratoEngine::maxDepthMs * 0.001f;
    const float slopeMax  = (waveV == 1) ? 4.0f : juce::MathConstants<float>::twoPi;
    const float kMax      = depthSec * slopeMax * rateV;
    const float peakCents = juce::jmax (
        std::abs (1200.0f * std::log2 (juce::jmax (1.0f - kMax, 0.05f))),
        1200.0f * std::log2 (1.0f + kMax));
    const float limit = centsLimitFor (peakCents * 1.02f);

    auto yForCents = [&] (float c)
    {
        return box.getCentreY() - c / limit * (box.getHeight() * 0.5f - 6.0f);
    };
    auto xForTimeBack = [&] (float tBack)   // tBack : secondes avant l'instant present
    {
        return box.getRight() - 1.0f - tBack / windowS * (box.getWidth() - 2.0f);
    };

    // --- Grille ------------------------------------------------------------
    for (int i = 1; i <= 3; ++i)   // verticales a chaque demi-periode
    {
        g.setColour (palette::inkFaint);
        g.drawVerticalLine ((int) xForTimeBack ((float) i * periodS * 0.5f),
                            box.getY() + 1.0f, box.getBottom() - 1.0f);
    }
    static const float levels[] = { 1.0f, 0.5f, 0.0f, -0.5f, -1.0f };
    for (const float lv : levels)
    {
        g.setColour (lv == 0.0f ? palette::inkMid.withAlpha (0.65f) : palette::inkFaint);
        g.drawHorizontalLine ((int) yForCents (lv * limit), box.getX() + 1.0f, box.getRight() - 1.0f);
    }

    // --- Courbes L / R -----------------------------------------------------
    // dev(t) evaluee a rebours depuis la phase reelle du moteur : la trace
    // entiere defile avec l'audio (mode oscilloscope "roll").
    auto buildCurve = [&] (int ch)
    {
        juce::Path p;
        for (int i = 0; i < kPoints; ++i)
        {
            const float tBack = windowS * (1.0f - (float) i / (float) (kPoints - 1));
            const float ph    = phase - tBack * rateV + (float) ch * stereoV * 0.5f;
            const float dev   = VibratoEngine::pitchDeviationCents (depthV, rateV, waveV, ph);
            const float px    = xForTimeBack (tBack);
            const float py    = juce::jlimit (box.getY() + 1.0f, box.getBottom() - 1.0f,
                                              yForCents (dev));
            if (i == 0) p.startNewSubPath (px, py);
            else        p.lineTo (px, py);
        }
        return p;
    };

    const bool splitChannels = stereoV > 0.001f;

    if (splitChannels)
    {
        // Canal droit : encre pale, trait tirete (l'etat survit au niveau de gris).
        juce::Path r = buildCurve (1);
        juce::Path dashed;
        const float dashes[] = { 4.0f, 3.0f };
        juce::PathStrokeType (1.1f).createDashedStroke (dashed, r, dashes, 2);
        g.setColour (palette::ink.withAlpha (0.85f));
        g.fillPath (dashed);
    }

    // Repere de l'instant present : index vertical au bord droit du cadre,
    // en retrait pour que le triangle s'imprime entier a l'interieur du filet.
    {
        const float sx = box.getRight() - 5.0f;
        g.setColour (palette::spot.withAlpha (0.30f));
        g.drawVerticalLine ((int) sx, box.getY() + 1.0f, box.getBottom() - 1.0f);

        juce::Path idx;   // petit index triangulaire en haut
        idx.addTriangle (sx - 3.5f, box.getY() + 1.0f, sx + 3.5f, box.getY() + 1.0f, sx, box.getY() + 7.0f);
        g.setColour (palette::spot);
        g.fillPath (idx);
    }

    // Canal gauche : encre spot, trait plein.
    g.setColour (palette::spot);
    g.strokePath (buildCurve (0), juce::PathStrokeType (1.6f, juce::PathStrokeType::curved));

    // Points "instant present" sur la trace : plein (L), anneau (R).
    const float devNowL = VibratoEngine::pitchDeviationCents (depthV, rateV, waveV, phase);
    {
        const float dx = box.getRight() - 5.0f;   // aligne sur l'index de l'instant present
        g.setColour (palette::spot);
        g.fillEllipse (dx - 2.2f, yForCents (devNowL) - 2.2f, 4.4f, 4.4f);
        if (splitChannels)
        {
            const float devNowR = VibratoEngine::pitchDeviationCents (
                depthV, rateV, waveV, phase + stereoV * 0.5f);
            g.drawEllipse (dx - 2.4f, yForCents (devNowR) - 2.4f, 4.8f, 4.8f, 1.1f);
        }
    }

    // --- Echelles ----------------------------------------------------------
    // Chaque chiffre est pose sur un cartouche film : ni le grain ni la grille
    // ne peuvent corrompre une valeur que le regard doit pouvoir croire.
    auto drawFigure = [&g] (const juce::String& text, juce::Point<float> anchor,
                            juce::Justification just)
    {
        const auto font = fonts::mono (9.0f);
        const float tw  = juce::GlyphArrangement::getStringWidth (font, text);
        auto area = juce::Rectangle<float> (tw + 6.0f, 11.0f).withCentre (anchor);
        if (just.testFlags (juce::Justification::left))
            area.setX (anchor.x - 3.0f);
        else if (just.testFlags (juce::Justification::right))
            area.setX (anchor.x - tw - 3.0f);

        g.setColour (palette::film);
        g.fillRect (area);
        g.setFont (font);
        g.setColour (palette::inkMid);
        g.drawText (text, area, juce::Justification::centred);
    };

    // Graduations de temps (avant l'instant present), unite choisie une fois.
    const bool inSeconds = windowS >= 2.0f;
    for (int i = 1; i <= 3; ++i)
    {
        const float tBack = (float) i * periodS * 0.5f;
        const juce::String txt = inSeconds
            ? "-" + juce::String (tBack, 1)
            : "-" + juce::String (juce::roundToInt (tBack * 1000.0f));
        drawFigure (txt, { xForTimeBack (tBack), box.getBottom() - 8.0f },
                    juce::Justification::centred);
    }

    // Graduations de deviation (cents), a gauche. L'unite est designee une
    // fois par echelle (sur la figure la plus haute), convention de plan ; les
    // graduations des bords restent a l'interieur du cadre.
    drawFigure (centsLabel (limit) + " ct",
                { box.getX() + 6.0f, yForCents (limit) + 7.0f }, juce::Justification::left);
    drawFigure (centsLabel (0.5f * limit),
                { box.getX() + 6.0f, yForCents (0.5f * limit) - 6.0f }, juce::Justification::left);
    drawFigure ("0", { box.getX() + 6.0f, yForCents (0.0f) - 6.0f }, juce::Justification::left);
    drawFigure (centsLabel (-0.5f * limit),
                { box.getX() + 6.0f, yForCents (-0.5f * limit) - 6.0f }, juce::Justification::left);
    drawFigure (centsLabel (-limit),
                { box.getX() + 6.0f, yForCents (-limit) - 6.0f }, juce::Justification::left);

    drawFigure (inSeconds ? "s" : "ms",
                { box.getRight() - 6.0f, box.getBottom() - 8.0f }, juce::Justification::right);

    // Tally de la deviation courante, en machine a ecrire : "dev  -84 ct".
    {
        const juce::String devText = (devNowL >= 0.0f ? "+" : "")
            + juce::String (juce::roundToInt (devNowL)) + " ct";

        auto tally = juce::Rectangle<float> (120.0f, 12.0f)
                         .withPosition (box.getRight() - 126.0f, box.getY() + 6.0f);
        g.setColour (palette::film);
        g.fillRect (tally.expanded (3.0f, 1.0f));
        g.setFont (fonts::mono (10.0f));
        g.setColour (palette::inkMid);
        g.drawText ("dev", tally.removeFromLeft (30.0f), juce::Justification::centredLeft);
        g.setColour (palette::ink);
        g.drawText (devText, tally, juce::Justification::centredLeft);
    }

    // Legende L / R en haut a gauche.
    {
        float lx = box.getX() + 34.0f;
        const float ly = box.getY() + 12.0f;
        g.setFont (fonts::mono (9.0f));

        g.setColour (palette::spot);
        g.drawLine (lx, ly, lx + 14.0f, ly, 1.6f);
        g.setColour (palette::inkMid);
        g.drawText ("L", juce::Rectangle<float> (12.0f, 10.0f).withPosition (lx + 18.0f, ly - 5.0f),
                    juce::Justification::centredLeft);

        if (splitChannels)
        {
            lx += 40.0f;
            g.setColour (palette::ink.withAlpha (0.85f));
            g.drawLine (lx, ly, lx + 4.0f, ly, 1.1f);
            g.drawLine (lx + 7.0f, ly, lx + 11.0f, ly, 1.1f);
            g.setColour (palette::inkMid);
            g.drawText ("R", juce::Rectangle<float> (12.0f, 10.0f).withPosition (lx + 15.0f, ly - 5.0f),
                        juce::Justification::centredLeft);
        }
    }

    // --- Cadre + legende de figure ------------------------------------------
    g.setColour (palette::ink);
    g.drawRect (box, 1.0f);

    const juce::String cap = "FIG. 1 - PITCH DEVIATION, MODULATED DELAY PATH";
    g.setFont (fonts::lettering (10.0f));
    g.setColour (palette::inkMid);
    g.drawText (cap, caption.withTrimmedTop (4.0f), juce::Justification::bottomLeft);

    const float capW = juce::GlyphArrangement::getStringWidth (fonts::lettering (10.0f), cap);
    g.setColour (palette::inkFaint);
    g.drawHorizontalLine ((int) (caption.getBottom() - 3.0f),
                          caption.getX() + capW + 10.0f, caption.getRight());
}

}
