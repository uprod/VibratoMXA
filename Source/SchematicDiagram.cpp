#include "SchematicDiagram.h"
#include "ManualStyle.h"
#include "VibratoEngine.h"

namespace vibratomxa
{

namespace
{
    // Epaisseur de trait proportionnelle a une quantite 0..1 : la geometrie
    // porte la valeur, jamais la couleur seule.
    float weightFor (float amount01)
    {
        return 0.7f + 2.4f * juce::jlimit (0.0f, 1.0f, amount01);
    }

    void drawArrowHead (juce::Graphics& g, juce::Point<float> tip, juce::Point<float> dir, float size)
    {
        dir = dir / (dir.getDistanceFromOrigin() + 1.0e-6f);
        const juce::Point<float> n (-dir.y, dir.x);
        juce::Path p;
        p.addTriangle (tip, tip - dir * size + n * (size * 0.55f),
                             tip - dir * size - n * (size * 0.55f));
        g.fillPath (p);
    }

    void drawDashedLine (juce::Graphics& g, juce::Line<float> line, float thickness)
    {
        const float dashes[] = { 3.0f, 3.0f };
        g.drawDashedLine (line, dashes, 2, thickness);
    }

    // Etiquette imprimee qui interrompt le trait qu'elle chevauche : on pose
    // un cartouche couleur film derriere le texte, comme sur un vrai plan.
    void drawLabelOverLine (juce::Graphics& g, const juce::String& text,
                            juce::Rectangle<float> area, juce::Justification just)
    {
        const float tw = juce::GlyphArrangement::getStringWidth (fonts::lettering (9.0f), text);
        auto knockout = area.withSizeKeepingCentre (tw + 10.0f, area.getHeight());
        if (just.testFlags (juce::Justification::left))
            knockout.setX (area.getX() - 5.0f);
        else if (just.testFlags (juce::Justification::right))
            knockout.setX (area.getRight() - tw - 5.0f);

        g.setColour (palette::film);
        g.fillRect (knockout);
        g.setFont (fonts::lettering (9.0f));
        g.setColour (palette::inkMid);
        g.drawText (text, area, just);
    }

    // Croix de sommateur dans un cercle (jonction "+" du schema).
    void drawSummingNode (juce::Graphics& g, juce::Point<float> c, float r)
    {
        g.setColour (palette::film);
        g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
        g.setColour (palette::ink);
        g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 1.1f);
        g.drawLine (c.x - r * 0.5f, c.y, c.x + r * 0.5f, c.y, 1.0f);
        g.drawLine (c.x, c.y - r * 0.5f, c.x, c.y + r * 0.5f, 1.0f);
    }

    // Rail pondere : epaisseur = quantite ; a zero il degenere en tirete fin.
    void drawWeightedLine (juce::Graphics& g, juce::Line<float> line, float amount01)
    {
        if (amount01 < 0.005f)
            drawDashedLine (g, line, 0.7f);
        else
            g.drawLine (line, weightFor (amount01));
    }
}

SchematicDiagram::SchematicDiagram (VibratoProcessor& proc)
    : processor (proc)
{
    auto& apvts = processor.getAPVTS();
    depth    = apvts.getRawParameterValue ("depth");
    waveform = apvts.getRawParameterValue ("waveform");
    stereo   = apvts.getRawParameterValue ("stereo");
    mix      = apvts.getRawParameterValue ("mix");

    setInterceptsMouseClicks (false, false);
}

void SchematicDiagram::paint (juce::Graphics& g)
{
    const float w = (float) getWidth();
    auto caption  = getLocalBounds().toFloat().removeFromBottom (16.0f);

    const float depthV  = depth->load();
    const int   waveV   = juce::roundToInt (waveform->load());
    const float stereoV = stereo->load();
    const float mixV    = mix->load();
    const float phase   = processor.getLfoPhase01();

    // Rangs horizontaux du schema.
    const float wetY  = 26.0f;   // rail principal (chemin retarde)
    const float lfoY  = 58.0f;   // centre du cercle LFO
    const float dryY  = 74.0f;   // rail dry

    // Colonnes.
    const float inX      = 12.0f;
    const float branchX  = 36.0f;
    const float delayX0  = 120.0f, delayX1 = 250.0f;   // bloc ligne a retard
    const float interpX0 = 292.0f, interpX1 = 394.0f;  // bloc interpolateur
    const float mixX     = w * 0.86f;
    const float outX     = w - 16.0f;
    const float lfoCx    = (delayX0 + delayX1) * 0.5f;
    const float blockH   = 28.0f;

    const float wetW = weightFor (mixV);
    const float dryW = weightFor (1.0f - mixV);

    // --- Rail d'entree et derivation dry ------------------------------------
    g.setColour (palette::ink);
    g.drawEllipse (inX - 3.0f, wetY - 3.0f, 6.0f, 6.0f, 1.1f);               // borne IN
    g.drawLine (inX + 3.0f, wetY, delayX0, wetY, 1.2f);
    drawArrowHead (g, { delayX0, wetY }, { 1.0f, 0.0f }, 6.0f);
    g.fillEllipse (branchX - 2.2f, wetY - 2.2f, 4.4f, 4.4f);                 // noeud de derivation

    g.setColour (palette::ink.withAlpha (0.9f));
    drawWeightedLine (g, { { branchX, wetY }, { branchX, dryY } },
                      (1.0f - mixV) * 0.75f);                                // descente dry
    drawWeightedLine (g, { { branchX, dryY }, { mixX, dryY } }, 1.0f - mixV); // rail dry
    drawWeightedLine (g, { { mixX, dryY }, { mixX, lfoY + 1.0f } }, 1.0f - mixV);

    g.setFont (fonts::lettering (9.0f));
    g.setColour (palette::inkMid);
    g.drawText ("IN", juce::Rectangle<float> (24.0f, 10.0f).withPosition (inX - 8.0f, wetY - 17.0f),
                juce::Justification::centredLeft);

    // L'etiquette interrompt le rail dry, comme sur un vrai plan.
    drawLabelOverLine (g, "DRY PATH",
                       juce::Rectangle<float> (80.0f, 10.0f)
                           .withCentre ({ (interpX0 + interpX1) * 0.5f, dryY }),
                       juce::Justification::centred);

    // --- Bloc ligne a retard : le temps de lecture imprime est le vrai -------
    {
        const juce::Rectangle<float> block (delayX0, wetY - blockH * 0.5f,
                                            delayX1 - delayX0, blockH);
        g.setColour (palette::film);
        g.fillRect (block);
        g.setColour (palette::ink);
        g.drawRect (block, 1.2f);

        g.setFont (fonts::lettering (10.0f));
        g.drawText ("DELAY LINE", block.withTrimmedBottom (10.0f), juce::Justification::centred);

        // Tap module reel : centre 6.0 ms, excursion depth x 6.0 ms.
        const juce::String tap = juce::String (VibratoEngine::centerMs, 1) + " ±"
            + juce::String (depthV * VibratoEngine::maxDepthMs, 1) + " ms";
        g.setFont (fonts::mono (8.0f));
        g.setColour (palette::inkMid);
        g.drawText (tap, block.withTrimmedTop (13.0f), juce::Justification::centred);
    }

    // --- Bloc interpolateur ---------------------------------------------------
    g.setColour (palette::ink);
    g.drawLine (delayX1, wetY, interpX0, wetY, 1.2f);
    {
        const juce::Rectangle<float> block (interpX0, wetY - blockH * 0.5f,
                                            interpX1 - interpX0, blockH);
        g.setColour (palette::film);
        g.fillRect (block);
        g.setColour (palette::ink);
        g.drawRect (block, 1.2f);

        g.setFont (fonts::lettering (10.0f));
        g.drawText ("LAGRANGE", block.withTrimmedBottom (10.0f), juce::Justification::centred);
        g.setFont (fonts::mono (8.0f));
        g.setColour (palette::inkMid);
        g.drawText ("3RD ORDER", block.withTrimmedTop (13.0f), juce::Justification::centred);
    }

    // --- Rail wet vers le sommateur de mix ------------------------------------
    g.setColour (palette::ink.withAlpha (0.9f));
    drawWeightedLine (g, { { interpX1, wetY }, { mixX, wetY } }, mixV);
    drawWeightedLine (g, { { mixX, wetY }, { mixX, lfoY - 1.0f } }, mixV);
    g.setFont (fonts::lettering (9.0f));
    g.setColour (palette::inkMid);
    g.drawText ("WET", juce::Rectangle<float> (30.0f, 10.0f).withPosition (mixX - 44.0f, wetY - 13.0f),
                juce::Justification::centredRight);

    drawSummingNode (g, { mixX, lfoY }, 8.0f);
    g.drawText ("MIX", juce::Rectangle<float> (30.0f, 10.0f).withPosition (mixX + 12.0f, lfoY - 20.0f),
                juce::Justification::centredLeft);

    // --- Sortie ----------------------------------------------------------------
    g.setColour (palette::ink);
    g.drawLine (mixX + 8.0f, lfoY, outX - 3.0f, lfoY, 1.4f);
    g.fillEllipse (outX - 3.0f, lfoY - 3.0f, 6.0f, 6.0f);                    // borne OUT
    g.setColour (palette::inkMid);
    g.drawText ("OUT", juce::Rectangle<float> (28.0f, 10.0f).withPosition (outX - 24.0f, lfoY - 17.0f),
                juce::Justification::centredRight);

    // --- LFO : glyphe de la forme selectionnee + point de phase reel -----------
    {
        const float r = 10.0f;

        // Ligne de modulation vers le tap : tiretee, epaisseur = depth.
        g.setColour (palette::spot.withAlpha (0.6f));
        drawDashedLine (g, { { lfoCx, lfoY - r }, { lfoCx, wetY + blockH * 0.5f } },
                        depthV < 0.005f ? 0.7f : weightFor (depthV));
        g.setColour (palette::spot);
        drawArrowHead (g, { lfoCx, wetY + blockH * 0.5f }, { 0.0f, -1.0f }, 5.0f);

        // Rappel de plan : la ligne porte son nom, reliee par une amorce pointee.
        {
            const juce::Point<float> tip (lfoCx, (lfoY - r + wetY + blockH * 0.5f) * 0.5f);
            g.setColour (palette::inkMid);
            g.drawLine (tip.x, tip.y, tip.x + 16.0f, tip.y, 0.7f);
            g.fillEllipse (tip.x - 1.6f, tip.y - 1.6f, 3.2f, 3.2f);
            g.setFont (fonts::lettering (9.0f));
            g.drawText ("MODULATION",
                        juce::Rectangle<float> (90.0f, 10.0f).withPosition (tip.x + 20.0f, tip.y - 5.0f),
                        juce::Justification::centredLeft);
        }

        g.setColour (palette::film);
        g.fillEllipse (lfoCx - r, lfoY - r, r * 2.0f, r * 2.0f);
        g.setColour (palette::ink);
        g.drawEllipse (lfoCx - r, lfoY - r, r * 2.0f, r * 2.0f, 1.1f);

        // Un cycle de la forme d'onde SELECTIONNEE dans le cercle.
        juce::Path glyph;
        for (int i = 0; i <= 24; ++i)
        {
            const float t = (float) i / 24.0f;
            const float sx = lfoCx - 6.5f + 13.0f * t;
            const float sy = lfoY - 4.5f * VibratoEngine::lfoValueFor (waveV, t);
            if (i == 0) glyph.startNewSubPath (sx, sy);
            else        glyph.lineTo (sx, sy);
        }
        g.setColour (palette::inkMid);
        g.strokePath (glyph, juce::PathStrokeType (1.0f));

        // Point de phase (canal gauche, plein) ; canal droit en anneau si stereo.
        auto dotAt = [&] (float ph, bool filled)
        {
            ph -= std::floor (ph);
            const float dx = lfoCx - 6.5f + 13.0f * ph;
            const float dy = lfoY - 4.5f * VibratoEngine::lfoValueFor (waveV, ph);
            g.setColour (palette::spot);
            if (filled) g.fillEllipse (dx - 2.2f, dy - 2.2f, 4.4f, 4.4f);
            else        g.drawEllipse (dx - 2.4f, dy - 2.4f, 4.8f, 4.8f, 1.1f);
        };
        dotAt (phase, true);
        if (stereoV > 0.001f)
            dotAt (phase + stereoV * 0.5f, false);

        g.setFont (fonts::lettering (9.0f));
        g.setColour (palette::inkMid);
        g.drawText ("LFO",
                    juce::Rectangle<float> (30.0f, 10.0f).withPosition (lfoCx - r - 36.0f, lfoY - 5.0f),
                    juce::Justification::centredRight);
    }

    // --- Legende de figure ------------------------------------------------------
    const juce::String cap = juce::String ("FIG. 2 - SIGNAL PATH, DELAY-LINE VIBRATO, ")
        + (waveV == 1 ? "TRIANGLE" : "SINE") + " LFO";
    g.setFont (fonts::lettering (10.0f));
    g.setColour (palette::inkMid);
    g.drawText (cap, caption.withTrimmedTop (4.0f), juce::Justification::bottomLeft);

    const float capW = juce::GlyphArrangement::getStringWidth (fonts::lettering (10.0f), cap);
    g.setColour (palette::inkFaint);
    g.drawHorizontalLine ((int) (caption.getBottom() - 3.0f),
                          caption.getX() + capW + 10.0f, caption.getRight());
}

}
