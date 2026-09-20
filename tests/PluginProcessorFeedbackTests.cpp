#include <PluginProcessor.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

namespace
{
    constexpr double sampleRate = 100.0;
    constexpr int chunkLength = 10; // 100 ms at 100 Hz
    constexpr int impulsePosition = 4; // mid-chunk, clear of the boundary fades

    void setParameter (PluginProcessor& plugin, const char* id, float value)
    {
        auto* param = dynamic_cast<juce::AudioParameterFloat*> (plugin.apvts.getParameter (id));
        REQUIRE (param != nullptr);
        *param = value;
    }

    // Feeds an impulse in the first chunk, then two silent chunks, through the real
    // processBlock and returns the third chunk (the first forward echo).
    juce::AudioBuffer<float> thirdChunkAfterImpulse (PluginProcessor& plugin)
    {
        juce::MidiBuffer midi;

        juce::AudioBuffer<float> impulseChunk (2, chunkLength);
        impulseChunk.clear();
        impulseChunk.setSample (0, impulsePosition, 1.0f);
        plugin.processBlock (impulseChunk, midi);

        juce::AudioBuffer<float> reversedEcho (2, chunkLength);
        reversedEcho.clear();
        plugin.processBlock (reversedEcho, midi);

        juce::AudioBuffer<float> forwardEcho (2, chunkLength);
        forwardEcho.clear();
        plugin.processBlock (forwardEcho, midi);

        return forwardEcho;
    }

    void prepare (PluginProcessor& plugin)
    {
        plugin.setRateAndBufferSizeDetails (sampleRate, 512);
        plugin.prepareToPlay (sampleRate, 512);

        setParameter (plugin, "chunkLengthMs", 100.0f);
        setParameter (plugin, "mix", 100.0f); // hear the wet signal only
    }
}

TEST_CASE ("PluginProcessor feeds the reversed output back according to the feedback parameter", "[PluginProcessor][Feedback]")
{
    PluginProcessor plugin;
    prepare (plugin);
    setParameter (plugin, "feedback", 50.0f);

    const auto forwardEcho = thirdChunkAfterImpulse (plugin);

    // The impulse comes back where it started, at half level.
    CHECK (forwardEcho.getSample (0, impulsePosition) == Catch::Approx (0.5f));
}

TEST_CASE ("PluginProcessor produces no echoes while feedback is at its default", "[PluginProcessor][Feedback]")
{
    PluginProcessor plugin;
    prepare (plugin);

    const auto forwardEcho = thirdChunkAfterImpulse (plugin);

    for (int i = 0; i < chunkLength; ++i)
        CHECK (forwardEcho.getSample (0, i) == 0.0f);
}

TEST_CASE ("PluginProcessor keeps feedback below unity gain even at 100%", "[PluginProcessor][Feedback]")
{
    PluginProcessor plugin;
    prepare (plugin);
    setParameter (plugin, "feedback", 100.0f);

    const auto forwardEcho = thirdChunkAfterImpulse (plugin);
    const auto echoLevel = forwardEcho.getSample (0, impulsePosition);

    CHECK (echoLevel == Catch::Approx (DualBufferReverseEngine::maxFeedbackGain));
    CHECK (echoLevel < 1.0f);
}
