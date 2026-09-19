#include <PluginProcessor.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

// Exercises the real PluginProcessor::processBlock path (parameter -> ms-to-samples
// conversion -> DualBufferReverseEngine -> the plugin's fixed internal crossfade),
// as distinct from DualBufferReverseEngineTests.cpp, which tests the engine in
// isolation with crossfade disabled.
TEST_CASE ("PluginProcessor reverses audio according to the chunkLengthMs parameter", "[PluginProcessor][ChunkLength]")
{
    constexpr double sampleRate = 100.0; // contrived, to keep sample counts small and exact
    constexpr int chunkLength = 10;      // chunkLengthMs = 100.0 at 100 Hz => 10 samples

    PluginProcessor plugin;
    // A real host wrapper calls this before prepareToPlay(); driving the processor
    // directly in a test means we have to do it ourselves, or getSampleRate() reads 0.
    plugin.setRateAndBufferSizeDetails (sampleRate, 512);
    plugin.prepareToPlay (sampleRate, 512);

    auto* chunkLengthMsParam = dynamic_cast<juce::AudioParameterFloat*> (plugin.apvts.getParameter ("chunkLengthMs"));
    REQUIRE (chunkLengthMsParam != nullptr);
    *chunkLengthMsParam = 100.0f;

    juce::MidiBuffer midi;

    juce::AudioBuffer<float> firstChunk (2, chunkLength);
    for (int i = 0; i < chunkLength; ++i)
        firstChunk.setSample (0, i, static_cast<float> (i + 1)); // 1..10
    plugin.processBlock (firstChunk, midi);

    juce::AudioBuffer<float> secondChunk (2, chunkLength);
    plugin.processBlock (secondChunk, midi);

    // crossfadeMs (15ms fixed) => 2 samples at 100 Hz, so positions 0-1 fade in,
    // 8-9 fade out, and 2-7 are the unfaded steady region.
    CHECK (secondChunk.getSample (0, 0) == Catch::Approx (0.0f));  // fade-in start
    CHECK (secondChunk.getSample (0, 1) == Catch::Approx (4.5f));  // fade-in ramp
    CHECK (secondChunk.getSample (0, 2) == Catch::Approx (8.0f));  // steady region
    CHECK (secondChunk.getSample (0, 5) == Catch::Approx (5.0f));  // steady region
    CHECK (secondChunk.getSample (0, 8) == Catch::Approx (1.0f));  // fade-out ramp
    CHECK (secondChunk.getSample (0, 9) == Catch::Approx (0.0f));  // fade-out end
}

TEST_CASE ("PluginProcessor derives Chunk Length from host tempo when Tempo Sync is on", "[PluginProcessor][ChunkLength]")
{
    // At 20 Hz (contrived, for small exact sample counts), a quarter note at the
    // no-playhead fallback of 120 BPM is 0.5s = 10 samples, and the fixed 15ms
    // crossfade rounds down to 0 samples, so this is a pure reversal with no fade.
    constexpr double sampleRate = 20.0;
    constexpr int chunkLength = 10;

    PluginProcessor plugin;
    plugin.setRateAndBufferSizeDetails (sampleRate, 512);
    plugin.prepareToPlay (sampleRate, 512);

    auto* tempoSyncParam = dynamic_cast<juce::AudioParameterBool*> (plugin.apvts.getParameter ("tempoSync"));
    REQUIRE (tempoSyncParam != nullptr);
    *tempoSyncParam = true;

    auto* divisionParam = dynamic_cast<juce::AudioParameterChoice*> (plugin.apvts.getParameter ("tempoSyncDivision"));
    REQUIRE (divisionParam != nullptr);
    *divisionParam = 2; // "1/4"

    juce::MidiBuffer midi;

    juce::AudioBuffer<float> firstChunk (2, chunkLength);
    for (int i = 0; i < chunkLength; ++i)
        firstChunk.setSample (0, i, static_cast<float> (i + 1)); // 1..10
    plugin.processBlock (firstChunk, midi);

    juce::AudioBuffer<float> secondChunk (2, chunkLength);
    plugin.processBlock (secondChunk, midi);

    const float expected[chunkLength] = { 10.0f, 9.0f, 8.0f, 7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f };
    for (int i = 0; i < chunkLength; ++i)
        CHECK (secondChunk.getSample (0, i) == Catch::Approx (expected[i]));
}
