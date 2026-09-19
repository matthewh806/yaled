#include "dsp/DualBufferReverseEngine.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE ("DualBufferReverseEngine outputs silence for the first chunk before any history exists", "[DualBufferReverseEngine]")
{
    constexpr int chunkLength = 4;

    DualBufferReverseEngine engine;
    engine.prepare (1, chunkLength, 0);

    juce::AudioBuffer<float> buffer (1, chunkLength);
    for (int i = 0; i < chunkLength; ++i)
        buffer.setSample (0, i, 1.0f);

    engine.processBlock (buffer);

    for (int i = 0; i < chunkLength; ++i)
        CHECK (buffer.getSample (0, i) == 0.0f);
}

TEST_CASE ("DualBufferReverseEngine plays the previous chunk back in reverse sample order", "[DualBufferReverseEngine]")
{
    constexpr int chunkLength = 4;

    DualBufferReverseEngine engine;
    engine.prepare (1, chunkLength, 0);

    juce::AudioBuffer<float> firstChunk (1, chunkLength);
    for (int i = 0; i < chunkLength; ++i)
        firstChunk.setSample (0, i, static_cast<float> (i + 1)); // 1, 2, 3, 4
    engine.processBlock (firstChunk);

    juce::AudioBuffer<float> secondChunk (1, chunkLength);
    for (int i = 0; i < chunkLength; ++i)
        secondChunk.setSample (0, i, static_cast<float> (i + 5)); // 5, 6, 7, 8
    engine.processBlock (secondChunk);

    const float expected[chunkLength] = { 4.0f, 3.0f, 2.0f, 1.0f };
    for (int i = 0; i < chunkLength; ++i)
        CHECK (secondChunk.getSample (0, i) == Catch::Approx (expected[i]));
}

TEST_CASE ("DualBufferReverseEngine fades in at the start of a chunk's playback", "[DualBufferReverseEngine]")
{
    constexpr int chunkLength = 8;
    constexpr int crossfadeLength = 2;

    DualBufferReverseEngine engine;
    engine.prepare (1, chunkLength, crossfadeLength);

    juce::AudioBuffer<float> firstChunk (1, chunkLength);
    firstChunk.setSample (0, 0, 5.0f);
    for (int i = 1; i < chunkLength; ++i)
        firstChunk.setSample (0, i, 5.0f);
    engine.processBlock (firstChunk);

    juce::AudioBuffer<float> secondChunk (1, chunkLength);
    for (int i = 0; i < chunkLength; ++i)
        secondChunk.setSample (0, i, 5.0f);
    engine.processBlock (secondChunk);

    CHECK (secondChunk.getSample (0, 0) == 0.0f);
    CHECK (secondChunk.getSample (0, 1) == 2.5f);
    CHECK (secondChunk.getSample (0, 4) == 5.0f); // steady region, unaffected
}

TEST_CASE ("DualBufferReverseEngine fades out at the end of a chunk's playback", "[DualBufferReverseEngine]")
{
    constexpr int chunkLength = 8;
    constexpr int crossfadeLength = 2;

    DualBufferReverseEngine engine;
    engine.prepare (1, chunkLength, crossfadeLength);

    juce::AudioBuffer<float> firstChunk (1, chunkLength);
    for (int i = 0; i < chunkLength; ++i)
        firstChunk.setSample (0, i, 5.0f);
    engine.processBlock (firstChunk);

    juce::AudioBuffer<float> secondChunk (1, chunkLength);
    for (int i = 0; i < chunkLength; ++i)
        secondChunk.setSample (0, i, 5.0f);
    engine.processBlock (secondChunk);

    CHECK (secondChunk.getSample (0, 6) == 2.5f);
    CHECK (secondChunk.getSample (0, 7) == 0.0f);
}

TEST_CASE ("DualBufferReverseEngine reverses each channel independently on shared chunk timing", "[DualBufferReverseEngine]")
{
    constexpr int chunkLength = 4;

    DualBufferReverseEngine engine;
    engine.prepare (2, chunkLength, 0);

    juce::AudioBuffer<float> firstChunk (2, chunkLength);
    for (int i = 0; i < chunkLength; ++i)
    {
        firstChunk.setSample (0, i, static_cast<float> (i + 1));       // 1, 2, 3, 4
        firstChunk.setSample (1, i, static_cast<float> ((i + 1) * 10)); // 10, 20, 30, 40
    }
    engine.processBlock (firstChunk);

    juce::AudioBuffer<float> secondChunk (2, chunkLength);
    engine.processBlock (secondChunk);

    const float expectedChannel0[chunkLength] = { 4.0f, 3.0f, 2.0f, 1.0f };
    const float expectedChannel1[chunkLength] = { 40.0f, 30.0f, 20.0f, 10.0f };
    for (int i = 0; i < chunkLength; ++i)
    {
        CHECK (secondChunk.getSample (0, i) == Catch::Approx (expectedChannel0[i]));
        CHECK (secondChunk.getSample (1, i) == Catch::Approx (expectedChannel1[i]));
    }
}
