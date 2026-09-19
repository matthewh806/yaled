#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

// Dual-Buffer Engine (see docs/adr/0001-dual-buffer-reverse-engine.md and
// CONTEXT.md): two buffers alternate roles every chunkLength samples, one
// playing its captured audio back in reverse while the other silently fills
// with fresh input. A fade-in/fade-out envelope at each buffer's playback
// boundaries (Chunk Boundary Crossfade) avoids audible clicks at the handoff.
class DualBufferReverseEngine
{
public:
    void prepare (int numChannels, int maxChunkLengthSamples, int crossfadeLengthSamples);
    void setChunkLength (int chunkLengthSamples);
    void processBlock (juce::AudioBuffer<float>& buffer);

private:
    juce::AudioBuffer<float> bufferA, bufferB;
    juce::AudioBuffer<float>* captureBuffer = &bufferA;
    juce::AudioBuffer<float>* playBuffer = &bufferB;

    int maxChunkLength = 0;
    int chunkLength = 0;
    int crossfadeLength = 0;
    int position = 0;
    int pendingChunkLength = -1; // -1 means no pending change

    float gainAt (int pos) const;
};
