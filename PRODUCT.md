# Product

<!-- impeccable:product-schema 1 -->

## Platform

desktop

Native JUCE audio plugin (AU/VST3/Standalone, macOS 11+). Sibling of the MXA suite; family design authority: `../PhaserMXA/DESIGN.md`; family product context: `../PhaserMXA/PRODUCT.md`.

## Product Purpose

A delay-line vibrato: an LFO-modulated interpolated delay tap producing true pitch modulation (Doppler), sine or triangle.

## Capabilities and Constraints

- Exactly five parameters: `rate` (0.1–14 Hz), `depth`, `waveform` (Sine/Triangle choice), `stereo` (L/R LFO phase offset), `mix`.
- Delay center 6 ms, max depth ±6 ms (`VibratoEngine::centerMs`/`maxDepthMs`), Lagrange 3rd-order interpolation.
- UI truth taps: atomic LFO phase (`uiPhase`), static `lfoValueFor()` / `pitchDeviationCents()` — the single source of truth for FIG. 1's deviation trace; the trace shape must follow the selected waveform (sine → cosine trace, triangle → square trace).
- Editor: Service Manual family sheet, 820×470, spot ink violet #9D7BFF, DWG NO. MXA-VB-01.

## Brand Commitments

Inherits the family's: MXAudio, "BY MESCALINA" credit, one spot ink per sibling (Vibrato = violet).

## Evidence on Hand

Working DSP (`Source/VibratoEngine.*`); review captures in `.impeccable/review/`. No users/testimonials — nothing may be fabricated.
