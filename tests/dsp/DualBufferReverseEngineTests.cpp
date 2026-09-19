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

TEST_CASE ("DualBufferReverseEngine applies setChunkLength immediately when called before any processing", "[DualBufferReverseEngine]")
{
    constexpr int maxChunkLength = 8;
    constexpr int newChunkLength = 4;

    DualBufferReverseEngine engine;
    engine.prepare (1, maxChunkLength, 0);
    engine.setChunkLength (newChunkLength);

    juce::AudioBuffer<float> firstChunk (1, newChunkLength);
    for (int i = 0; i < newChunkLength; ++i)
        firstChunk.setSample (0, i, static_cast<float> (i + 1)); // 1, 2, 3, 4
    engine.processBlock (firstChunk);

    juce::AudioBuffer<float> secondChunk (1, newChunkLength);
    for (int i = 0; i < newChunkLength; ++i)
        secondChunk.setSample (0, i, static_cast<float> (i + 5)); // 5, 6, 7, 8
    engine.processBlock (secondChunk);

    const float expected[newChunkLength] = { 4.0f, 3.0f, 2.0f, 1.0f };
    for (int i = 0; i < newChunkLength; ++i)
        CHECK (secondChunk.getSample (0, i) == Catch::Approx (expected[i]));
}

TEST_CASE ("DualBufferReverseEngine defers setChunkLength to the next chunk boundary when called mid-chunk", "[DualBufferReverseEngine]")
{
    constexpr int initialChunkLength = 8;
    constexpr int newChunkLength = 4;

    DualBufferReverseEngine engine;
    engine.prepare (1, initialChunkLength, 0);

    // Fill the first (initialChunkLength) chunk completely, so it becomes the play buffer.
    juce::AudioBuffer<float> firstChunk (1, initialChunkLength);
    for (int i = 0; i < initialChunkLength; ++i)
        firstChunk.setSample (0, i, static_cast<float> (i + 1)); // 1..8
    engine.processBlock (firstChunk);

    // Start the second (still initialChunkLength) cycle, but only partway through it...
    juce::AudioBuffer<float> partialSecondChunk (1, 3);
    partialSecondChunk.setSample (0, 0, 100.0f);
    partialSecondChunk.setSample (0, 1, 101.0f);
    partialSecondChunk.setSample (0, 2, 102.0f);
    engine.processBlock (partialSecondChunk);

    // ...then request a shorter chunk length mid-chunk. This must NOT take effect
    // until the current (initialChunkLength) cycle finishes.
    engine.setChunkLength (newChunkLength);

    // Finish the rest of the original-length cycle.
    juce::AudioBuffer<float> restOfSecondChunk (1, initialChunkLength - 3);
    restOfSecondChunk.setSample (0, 0, 200.0f);
    restOfSecondChunk.setSample (0, 1, 201.0f);
    restOfSecondChunk.setSample (0, 2, 202.0f);
    restOfSecondChunk.setSample (0, 3, 203.0f);
    restOfSecondChunk.setSample (0, 4, 204.0f);
    engine.processBlock (restOfSecondChunk);

    // The now-fully-captured buffer holds [100, 101, 102, 200, 201, 202, 203, 204].
    // With the new (4-sample) chunk length now active, only the first 4 of those
    // samples should be read back, in reverse.
    juce::AudioBuffer<float> thirdChunk (1, newChunkLength);
    engine.processBlock (thirdChunk);

    const float expected[newChunkLength] = { 200.0f, 102.0f, 101.0f, 100.0f };
    for (int i = 0; i < newChunkLength; ++i)
        CHECK (thirdChunk.getSample (0, i) == Catch::Approx (expected[i]));
}

TEST_CASE ("DualBufferReverseEngine clamps setChunkLength to the maximum established at prepare", "[DualBufferReverseEngine]")
{
    constexpr int maxChunkLength = 8;

    DualBufferReverseEngine engine;
    engine.prepare (1, maxChunkLength, 0);
    engine.setChunkLength (4);    // shrink first, so growing back is observable
    engine.setChunkLength (1000); // should clamp to maxChunkLength, not 1000

    juce::AudioBuffer<float> firstChunk (1, maxChunkLength);
    for (int i = 0; i < maxChunkLength; ++i)
        firstChunk.setSample (0, i, static_cast<float> (i + 1)); // 1..8
    engine.processBlock (firstChunk);

    juce::AudioBuffer<float> secondChunk (1, maxChunkLength);
    engine.processBlock (secondChunk);

    const float expected[maxChunkLength] = { 8.0f, 7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f };
    for (int i = 0; i < maxChunkLength; ++i)
        CHECK (secondChunk.getSample (0, i) == Catch::Approx (expected[i]));
}
