#include "TempoSync.h"

#include <cmath>

namespace
{
    double multiplierFor (NoteDivision division)
    {
        switch (division)
        {
            case NoteDivision::Whole:     return 4.0;
            case NoteDivision::Half:      return 2.0;
            case NoteDivision::Quarter:   return 1.0;
            case NoteDivision::Eighth:    return 0.5;
            case NoteDivision::Sixteenth: return 0.25;
        }

        return 1.0;
    }
}

int noteDivisionToSamples (double bpm, double sampleRate, NoteDivision division)
{
    const auto quarterNoteSeconds = 60.0 / bpm;
    const auto divisionSeconds = quarterNoteSeconds * multiplierFor (division);
    return static_cast<int> (std::round (divisionSeconds * sampleRate));
}
