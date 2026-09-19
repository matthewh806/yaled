#include "DualBufferReverseEngine.h"

#include <utility>

void DualBufferReverseEngine::prepare (int numChannels, int maxChunkLengthSamples, int crossfadeLengthSamples)
{
    maxChunkLength = maxChunkLengthSamples;
    chunkLength = maxChunkLengthSamples;
    crossfadeLength = crossfadeLengthSamples;
    position = 0;
    pendingChunkLength = -1;

    bufferA.setSize (numChannels, maxChunkLength);
    bufferB.setSize (numChannels, maxChunkLength);
    bufferA.clear();
    bufferB.clear();

    captureBuffer = &bufferA;
    playBuffer = &bufferB;
}

void DualBufferReverseEngine::setChunkLength (int chunkLengthSamples)
{
    const auto clamped = juce::jlimit (1, maxChunkLength, chunkLengthSamples);

    if (position == 0)
        chunkLength = clamped;
    else
        pendingChunkLength = clamped;
}

float DualBufferReverseEngine::gainAt (int pos) const
{
    if (crossfadeLength <= 0)
        return 1.0f;

    if (pos < crossfadeLength)
        return static_cast<float> (pos) / static_cast<float> (crossfadeLength);

    if (pos >= chunkLength - crossfadeLength)
        return static_cast<float> (chunkLength - 1 - pos) / static_cast<float> (crossfadeLength);

    return 1.0f;
}

void DualBufferReverseEngine::processBlock (juce::AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    for (int n = 0; n < numSamples; ++n)
    {
        const auto gain = gainAt (position);

        for (int channel = 0; channel < numChannels; ++channel)
        {
            const auto inputSample = buffer.getSample (channel, n);
            captureBuffer->setSample (channel, position, inputSample);

            const auto outputSample = playBuffer->getSample (channel, chunkLength - 1 - position) * gain;
            buffer.setSample (channel, n, outputSample);
        }

        ++position;
        if (position >= chunkLength)
        {
            std::swap (captureBuffer, playBuffer);
            position = 0;

            if (pendingChunkLength != -1)
            {
                chunkLength = pendingChunkLength;
                pendingChunkLength = -1;
            }
        }
    }
}
