# Yaled (Reverse Delay Plugin)

A JUCE-based audio effect plugin that plays incoming audio back in reverse, in continuous, overlapping chunks, rather than reversing a fixed recording. Personal, open-source project (not commercial), started after reading a [KVR Audio forum thread](https://www.kvraudio.com/forum/viewtopic.php?t=599376) discussing reverse-delay DSP techniques.

## Language

**Dual-Buffer Engine**:
The core mechanism of the effect: two buffers alternate roles, one playing its captured audio back in reverse while the other silently fills with fresh input, then they swap. This is what makes reversed playback continuous rather than a one-shot "record then reverse" operation.
_Avoid_: reverse buffer (ambiguous between the mechanism and a single buffer instance)

**Reverse Chunk**:
The span of audio captured by one buffer between swaps, and thus the unit that gets played back reversed. Its length determines both how much audio is reversed at once and the audible lag between input and reversed output.
_Avoid_: window, segment, frame (generic DSP terms that don't capture the swap semantics)

**Chunk Boundary Crossfade**:
The crossfade applied at the moment two Reverse Chunks hand off, masking the discontinuity that would otherwise click where one buffer's reversed playback ends and the other's begins.
_Avoid_: fade, transition

**Chunk Length**:
The user-controlled duration of a Reverse Chunk, set either as a free millisecond value or, when Tempo Sync is enabled, as a host tempo note division.
_Avoid_: delay time (implies the input-to-echo gap of a conventional forward delay; here the same value also defines how much audio gets reversed)

**Tempo Sync**:
The toggle that switches Chunk Length from a free millisecond value to a host tempo note division.

**Feedback Path**:
The route by which a Reverse Chunk's output is mixed back into the buffer currently capturing the next chunk, so successive chunks are built partly from prior reversed output rather than fresh input alone. This produces the cascading, regenerating trails associated with the effect. Because the fed-back audio is already reversed, each successive echo plays in the opposite direction to the one before it (reversed, then forward, then reversed again), each scaled by the feedback gain relative to the one before. The signal is tapped before the Chunk Boundary Crossfade, so the crossfade shapes only what is heard and does not compound down the trail, and the gain is capped just below unity.
_Avoid_: feedback loop (implies unstable/unintended feedback, whereas this is a deliberate, level-capped signal path)

**Linked Stereo Processing**:
The v1 approach where left and right channels share one Dual-Buffer Engine and one Chunk Length, so both channels reverse identically and the effect stays mono-compatible.
_Avoid_: stereo linking (describes a toggle; this project has no per-channel alternative yet to link/unlink between)
