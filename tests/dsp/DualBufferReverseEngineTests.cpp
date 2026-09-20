#include "dsp/DualBufferReverseEngine.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <limits>

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

TEST_CASE ("DualBufferReverseEngine feeds a Reverse Chunk back so its echo plays forward at the feedback gain", "[DualBufferReverseEngine][Feedback]")
{
    constexpr int chunkLength = 4;

    DualBufferReverseEngine engine;
    engine.prepare (1, chunkLength, 0);
    engine.setFeedback (0.5f);

    // Period 1: an impulse at the start of the first chunk. Nothing to play back yet.
    juce::AudioBuffer<float> impulseChunk (1, chunkLength);
    impulseChunk.clear();
    impulseChunk.setSample (0, 0, 1.0f);
    engine.processBlock (impulseChunk);

    // Period 2: the impulse plays back reversed, so it lands on the chunk's last sample.
    juce::AudioBuffer<float> reversedEcho (1, chunkLength);
    reversedEcho.clear();
    engine.processBlock (reversedEcho);

    const float expectedReversed[chunkLength] = { 0.0f, 0.0f, 0.0f, 1.0f };
    for (int i = 0; i < chunkLength; ++i)
        CHECK (reversedEcho.getSample (0, i) == Catch::Approx (expectedReversed[i]));

    // Period 3: that reversed echo was fed back into the capturing buffer, so reversing
    // it again puts the impulse back at the start of the chunk, at half the level.
    juce::AudioBuffer<float> forwardEcho (1, chunkLength);
    forwardEcho.clear();
    engine.processBlock (forwardEcho);

    const float expectedForward[chunkLength] = { 0.5f, 0.0f, 0.0f, 0.0f };
    for (int i = 0; i < chunkLength; ++i)
        CHECK (forwardEcho.getSample (0, i) == Catch::Approx (expectedForward[i]));
}

TEST_CASE ("DualBufferReverseEngine clamps feedback to a maximum below unity gain", "[DualBufferReverseEngine][Feedback]")
{
    constexpr int chunkLength = 4;

    STATIC_REQUIRE (DualBufferReverseEngine::maxFeedbackGain < 1.0f);

    DualBufferReverseEngine engine;
    engine.prepare (1, chunkLength, 0);
    engine.setFeedback (5.0f); // far more than the engine allows

    juce::AudioBuffer<float> impulseChunk (1, chunkLength);
    impulseChunk.clear();
    impulseChunk.setSample (0, 0, 1.0f);
    engine.processBlock (impulseChunk);

    juce::AudioBuffer<float> reversedEcho (1, chunkLength);
    reversedEcho.clear();
    engine.processBlock (reversedEcho);

    juce::AudioBuffer<float> forwardEcho (1, chunkLength);
    forwardEcho.clear();
    engine.processBlock (forwardEcho);

    // The forward echo is the impulse scaled once by the (clamped) feedback gain.
    CHECK (forwardEcho.getSample (0, 0) == Catch::Approx (DualBufferReverseEngine::maxFeedbackGain));
}

TEST_CASE ("DualBufferReverseEngine keeps alternating echo direction while each echo decays by the feedback gain", "[DualBufferReverseEngine][Feedback]")
{
    constexpr int chunkLength = 4;

    DualBufferReverseEngine engine;
    engine.prepare (1, chunkLength, 0);
    engine.setFeedback (0.5f);

    juce::AudioBuffer<float> impulseChunk (1, chunkLength);
    impulseChunk.clear();
    impulseChunk.setSample (0, 0, 1.0f);
    engine.processBlock (impulseChunk);

    // Reversed, forward, reversed, forward, each half the level of the one before.
    const float expected[][chunkLength] = {
        { 0.0f, 0.0f, 0.0f, 1.0f },
        { 0.5f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.25f },
        { 0.125f, 0.0f, 0.0f, 0.0f },
    };

    for (const auto& expectedChunk : expected)
    {
        juce::AudioBuffer<float> chunk (1, chunkLength);
        chunk.clear();
        engine.processBlock (chunk);

        for (int i = 0; i < chunkLength; ++i)
            CHECK (chunk.getSample (0, i) == Catch::Approx (expectedChunk[i]));
    }
}

TEST_CASE ("DualBufferReverseEngine takes feedback before the Chunk Boundary Crossfade, so the crossfade does not compound in the echoes", "[DualBufferReverseEngine][Feedback]")
{
    constexpr int chunkLength = 8;
    constexpr int crossfadeLength = 2;

    DualBufferReverseEngine engine;
    engine.prepare (1, chunkLength, crossfadeLength);
    engine.setFeedback (0.5f);

    juce::AudioBuffer<float> steadyChunk (1, chunkLength);
    for (int i = 0; i < chunkLength; ++i)
        steadyChunk.setSample (0, i, 4.0f);
    engine.processBlock (steadyChunk);

    juce::AudioBuffer<float> firstEcho (1, chunkLength);
    firstEcho.clear();
    engine.processBlock (firstEcho);

    juce::AudioBuffer<float> secondEcho (1, chunkLength);
    secondEcho.clear();
    engine.processBlock (secondEcho);

    // What went back in was 0.5 * 4.0 = 2.0 across the whole chunk, before any crossfade. The
    // second echo is then shaped by the crossfade once, on its way out: half level on the
    // second sample of the ramp up.
    CHECK (secondEcho.getSample (0, 0) == 0.0f);
    CHECK (secondEcho.getSample (0, 1) == Catch::Approx (1.0f));
    CHECK (secondEcho.getSample (0, 4) == Catch::Approx (2.0f));
}

TEST_CASE ("DualBufferReverseEngine feeds each channel back into itself with no cross-feed", "[DualBufferReverseEngine][Feedback]")
{
    constexpr int chunkLength = 4;

    DualBufferReverseEngine engine;
    engine.prepare (2, chunkLength, 0);
    engine.setFeedback (0.5f);

    juce::AudioBuffer<float> impulseChunk (2, chunkLength);
    impulseChunk.clear();
    impulseChunk.setSample (0, 0, 1.0f); // left impulse at the start
    impulseChunk.setSample (1, 1, 1.0f); // right impulse one sample later
    engine.processBlock (impulseChunk);

    juce::AudioBuffer<float> reversedEcho (2, chunkLength);
    reversedEcho.clear();
    engine.processBlock (reversedEcho);

    juce::AudioBuffer<float> forwardEcho (2, chunkLength);
    forwardEcho.clear();
    engine.processBlock (forwardEcho);

    const float expectedLeft[chunkLength] = { 0.5f, 0.0f, 0.0f, 0.0f };
    const float expectedRight[chunkLength] = { 0.0f, 0.5f, 0.0f, 0.0f };
    for (int i = 0; i < chunkLength; ++i)
    {
        CHECK (forwardEcho.getSample (0, i) == Catch::Approx (expectedLeft[i]));
        CHECK (forwardEcho.getSample (1, i) == Catch::Approx (expectedRight[i]));
    }
}

TEST_CASE ("DualBufferReverseEngine lets a non-finite input sample play once without circulating in the Feedback Path", "[DualBufferReverseEngine][Feedback]")
{
    constexpr int chunkLength = 4;

    for (const float badSample : { std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN() })
    {
        for (const float feedbackAmount : { 0.0f, 0.5f })
        {
            DualBufferReverseEngine engine;
            engine.prepare (1, chunkLength, 0);
            engine.setFeedback (feedbackAmount);

            juce::AudioBuffer<float> badChunk (1, chunkLength);
            badChunk.clear();
            badChunk.setSample (0, 0, badSample);
            engine.processBlock (badChunk);

            // It plays back once, in the second chunk. That is unavoidable and unchanged.
            juce::AudioBuffer<float> playedOnce (1, chunkLength);
            playedOnce.clear();
            engine.processBlock (playedOnce);

            // But it must not have been fed back, so every later chunk is clean.
            for (int later = 0; later < 3; ++later)
            {
                juce::AudioBuffer<float> chunk (1, chunkLength);
                chunk.clear();
                engine.processBlock (chunk);

                for (int i = 0; i < chunkLength; ++i)
                    CHECK (std::isfinite (chunk.getSample (0, i)));
            }
        }
    }
}

TEST_CASE ("DualBufferReverseEngine treats a negative feedback request as no feedback", "[DualBufferReverseEngine][Feedback]")
{
    constexpr int chunkLength = 4;

    DualBufferReverseEngine engine;
    engine.prepare (1, chunkLength, 0);
    engine.setFeedback (-1.0f);

    juce::AudioBuffer<float> impulseChunk (1, chunkLength);
    impulseChunk.clear();
    impulseChunk.setSample (0, 0, 1.0f);
    engine.processBlock (impulseChunk);

    juce::AudioBuffer<float> reversedEcho (1, chunkLength);
    reversedEcho.clear();
    engine.processBlock (reversedEcho);

    juce::AudioBuffer<float> noEcho (1, chunkLength);
    noEcho.clear();
    engine.processBlock (noEcho);

    for (int i = 0; i < chunkLength; ++i)
        CHECK (noEcho.getSample (0, i) == 0.0f);
}
