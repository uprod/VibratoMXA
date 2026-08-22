#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace vibratomxa
{

namespace IDs
{
    constexpr auto rate     = "rate";
    constexpr auto depth    = "depth";
    constexpr auto waveform = "waveform";
    constexpr auto stereo   = "stereo";
    constexpr auto mix      = "mix";
}

juce::AudioProcessorValueTreeState::ParameterLayout VibratoProcessor::createLayout()
{
    using P = juce::AudioParameterFloat;

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Affichages "tally" a la machine a ecrire, aussi bien dans l'editeur que
    // dans les lignes d'automation de l'hote.
    const auto hzAttr = juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction ([] (float v, int) { return juce::String (v, 2) + " Hz"; })
        .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue(); });

    const auto pctAttr = juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; })
        .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue() / 100.0f; });

    // Vitesse du vibrato. Skew < 1 = plus de finesse dans les vitesses lentes.
    params.push_back (std::make_unique<P> (juce::ParameterID { IDs::rate, 1 },
        "Rate", juce::NormalisableRange<float> (0.1f, 14.0f, 0.01f, 0.5f), 5.0f, hzAttr));

    params.push_back (std::make_unique<P> (juce::ParameterID { IDs::depth, 1 },
        "Depth", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.4f, pctAttr));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { IDs::waveform, 1 }, "Waveform",
        juce::StringArray { "Sine", "Triangle" }, 0));

    params.push_back (std::make_unique<P> (juce::ParameterID { IDs::stereo, 1 },
        "Stereo", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f, pctAttr));

    params.push_back (std::make_unique<P> (juce::ParameterID { IDs::mix, 1 },
        "Mix", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f, pctAttr));

    return { params.begin(), params.end() };
}

VibratoProcessor::VibratoProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
}

VibratoProcessor::~VibratoProcessor() = default;

void VibratoProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    engine.reset();

    dryBuffer.setSize (getTotalNumOutputChannels(), samplesPerBlock);
}

void VibratoProcessor::releaseResources()
{
    engine.reset();
}

bool VibratoProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& main = layouts.getMainOutputChannelSet();
    if (main != juce::AudioChannelSet::mono() && main != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == main;
}

void VibratoProcessor::pushParameterUpdatesToEngine()
{
    engine.setRateHz      (apvts.getRawParameterValue (IDs::rate)->load());
    engine.setDepth       (apvts.getRawParameterValue (IDs::depth)->load());
    engine.setWaveform    ((int) apvts.getRawParameterValue (IDs::waveform)->load());
    engine.setStereoWidth (apvts.getRawParameterValue (IDs::stereo)->load());
}

void VibratoProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    pushParameterUpdatesToEngine();

    // On garde une copie du son d'origine pour le mix dry/wet.
    dryBuffer.makeCopyOf (buffer, true);

    engine.process (buffer);   // 'buffer' contient maintenant le signal vibre (wet)

    const float wetAmt = apvts.getRawParameterValue (IDs::mix)->load();
    const float dryAmt = 1.0f - wetAmt;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* wet = buffer.getWritePointer (ch);
        const auto* dryIn = dryBuffer.getReadPointer (juce::jmin (ch, dryBuffer.getNumChannels() - 1));

        for (int n = 0; n < buffer.getNumSamples(); ++n)
            wet[n] = dryAmt * dryIn[n] + wetAmt * wet[n];
    }
}

juce::AudioProcessorEditor* VibratoProcessor::createEditor()
{
    return new VibratoEditor (*this);
}

void VibratoProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void VibratoProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

}

// Point d'entree du plugin JUCE — doit etre au niveau global.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new vibratomxa::VibratoProcessor();
}
