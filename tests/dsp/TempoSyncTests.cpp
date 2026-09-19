#include "dsp/TempoSync.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE ("noteDivisionToSamples converts a quarter note at 120 BPM to samples", "[TempoSync]")
{
    // At 120 BPM, one quarter note = 0.5 seconds.
    const auto samples = noteDivisionToSamples (120.0, 44100.0, NoteDivision::Quarter);
    CHECK (samples == 22050);
}

TEST_CASE ("noteDivisionToSamples scales linearly with the chosen division", "[TempoSync]")
{
    // At 120 BPM: whole = 2s, half = 1s, eighth = 0.25s, sixteenth = 0.125s.
    CHECK (noteDivisionToSamples (120.0, 44100.0, NoteDivision::Whole) == 88200);
    CHECK (noteDivisionToSamples (120.0, 44100.0, NoteDivision::Half) == 44100);
    CHECK (noteDivisionToSamples (120.0, 44100.0, NoteDivision::Eighth) == 11025);
    CHECK (noteDivisionToSamples (120.0, 44100.0, NoteDivision::Sixteenth) == 5513); // rounded from 5512.5
}

TEST_CASE ("noteDivisionToSamples scales inversely with tempo", "[TempoSync]")
{
    // At 90 BPM, one quarter note = 60/90 seconds.
    const auto samples = noteDivisionToSamples (90.0, 44100.0, NoteDivision::Quarter);
    CHECK (samples == 29400);
}
