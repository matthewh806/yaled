#pragma once

// The note divisions the Tempo Sync toggle can lock Chunk Length to (CONTEXT.md).
enum class NoteDivision
{
    Whole,
    Half,
    Quarter,
    Eighth,
    Sixteenth
};

// Converts a host tempo + note division into a sample count at the given sample rate.
int noteDivisionToSamples (double bpm, double sampleRate, NoteDivision division);
