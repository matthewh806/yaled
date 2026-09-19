#include <PluginProcessor.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE ("PluginProcessor passes the signal through unaffected at 0% mix", "[PluginProcessor][Mix]")
{
    constexpr double sampleRate = 100.0;
    constexpr int chunkLength = 10;

    PluginProcessor plugin;
    plugin.setRateAndBufferSizeDetails (sampleRate, 512);
    plugin.prepareToPlay (sampleRate, 512);

    auto* chunkLengthMsParam = dynamic_cast<juce::AudioParameterFloat*> (plugin.apvts.getParameter ("chunkLengthMs"));
    REQUIRE (chunkLengthMsParam != nullptr);
    *chunkLengthMsParam = 100.0f;

    auto* mixParam = dynamic_cast<juce::AudioParameterFloat*> (plugin.apvts.getParameter ("mix"));
    REQUIRE (mixParam != nullptr);
    *mixParam = 0.0f;

    juce::MidiBuffer midi;

    // Even on the very first chunk, where the reverse engine's own output would be
    // silence (no history yet), 0% mix should still pass the dry input straight through.
    juce::AudioBuffer<float> chunk (2, chunkLength);
    for (int i = 0; i < chunkLength; ++i)
        chunk.setSample (0, i, static_cast<float> (i + 1)); // 1..10
    plugin.processBlock (chunk, midi);

    for (int i = 0; i < chunkLength; ++i)
        CHECK (chunk.getSample (0, i) == Catch::Approx (static_cast<float> (i + 1)));
}

TEST_CASE ("PluginProcessor blends dry and wet signals proportionally to the mix parameter", "[PluginProcessor][Mix]")
{
    constexpr double sampleRate = 100.0;
    constexpr int chunkLength = 10;

    PluginProcessor plugin;
    plugin.setRateAndBufferSizeDetails (sampleRate, 512);
    plugin.prepareToPlay (sampleRate, 512);

    auto* chunkLengthMsParam = dynamic_cast<juce::AudioParameterFloat*> (plugin.apvts.getParameter ("chunkLengthMs"));
    REQUIRE (chunkLengthMsParam != nullptr);
    *chunkLengthMsParam = 100.0f;

    auto* mixParam = dynamic_cast<juce::AudioParameterFloat*> (plugin.apvts.getParameter ("mix"));
    REQUIRE (mixParam != nullptr);
    *mixParam = 50.0f;

    juce::MidiBuffer midi;

    // First chunk: the engine's own output is silence (no history), so at 50% mix
    // the result should be exactly half the dry input.
    juce::AudioBuffer<float> chunk (2, chunkLength);
    for (int i = 0; i < chunkLength; ++i)
        chunk.setSample (0, i, static_cast<float> (i + 1)); // 1..10
    plugin.processBlock (chunk, midi);

    for (int i = 0; i < chunkLength; ++i)
        CHECK (chunk.getSample (0, i) == Catch::Approx (static_cast<float> (i + 1) * 0.5f));
}
