# Reverse delay prior art

Research note for Yaled, checking the design (ADR-0001, issues #4, #5, #11, #13, #14) against sources beyond the one KVR thread.
All sources were accessed 2026-09-20. Source IDs (S1, S2, ...) are resolved in the Sources table at the end, which has the full URLs and commit permalinks.
Vocabulary follows `CONTEXT.md`. "Source states" is what a source says. "Reading" is my interpretation. Nothing below was run, only read.

## 1. Summary

- The KVR thread supports "two buffers take turns" only lightly. One post (Urs) describes it. The crossfade at the boundary comes from a different post (earlevel). A third (Richard_Synapse) warns that reversed audio is hard to keep in sync (S1).
- **ADR-0001 overstates "standard technique".** Of the readable implementations, only Yaled itself uses two swapping buffers. Others use one ring or delay buffer with moving read pointers (Zynaddsubfx, Roy Fox, the Stanford report), an in-place single buffer (Calf, Gx), or two read heads on one line (dm-Reverse). The Dual-Buffer Engine is legitimate but is not the common structure.
- **Dip and overlap are both established.** Fade-to-silence windows: Calf, Gx, and the Stanford report. Complementary overlapping crossfade: Zynaddsubfx and dm-Reverse. #13's "it is a preference" is consistent with the prior art.
- **Feedback taps in prior art sit before the window.** Calf and Gx take feedback from the reverse read before the window gain and write it into the buffer being captured (S3, S4). That avoids the compounding dips #13 worries about. The Stanford report shows that putting the reverse stage inside the loop makes echoes alternate reversed and forward (S6).
- **Feedback is usually not capped below unity.** Calf, Gx and dm-Reverse expose up to 1.0 / 100%. None has filtering in the loop. #4's "capped below unity" is more conservative than the prior art.
- **Chunk Length changes:** no readable implementation plays each Reverse Chunk at its captured length (#11's fix). Calf and Gx restart their counters. Zynaddsubfx reallocates and clears. dm-Reverse smooths the time control, which gives a pitch glide. #11's fix is compatible with the theory but has no readable precedent.
- **Short chunks are handled by scaling the fade.** Zynaddsubfx caps the fade at 0.8 of the delay. dm-Reverse's fade shrinks with time. Calf's window is a fraction of the chunk. This supports #14's `min(crossfade, chunk/2)` direction. Hardware usually avoids the problem with a floor (Boss DD-8 reverse: 300 ms). Yaled's floor is 10 ms.
- **Tail and latency:** a reverse delay has no fixed latency, so `0` latency is right. Tail is a separate matter, and `0.0` is probably wrong (section 3).

## 2. Findings

### Q1. How real reverse effects work

**Buffer strategy**

- KVR thread, "How does reverse delay effect work?" Urs (2023-07-19): reverse delays are "commonly achieved by two or more parallel delay lines which take turns in filling their buffers in quiet, then play them back backwards aloud" (S1). Richard_Synapse (2023-07-20) says it "might be tricky though to get musically meaningful results, since typcially the reversed audio would be out-of-sync" (S1). earlevel (2023-08-14): "the delay line is read out in reverse", takes "twice as much memory per second of delay time", and the boundary must be crossfaded (S1).
- Zynaddsubfx `Reverter`: one ring buffer, sized `3 * delay + fading_samples + ...` (S2, `Reverter.cpp` L230-256). Recording, reverse playback and a phase offset each get one delay's worth of memory (comment at L231-236).
- Calf `reverse_delay_line_impl`: one fixed buffer per channel. It reads `buf[length-1-counter]` and then writes `buf[counter]` in place (S3, `modules_delay.cpp` L706-720). GxReverseDelay uses the same routine (S4).
  - Reading: the first half of each period reads the previous cycle's data. The second half reads data written earlier in the same cycle. The window's period is `deltime/2` (L669), so the fade dips at the midpoint too. **Reading:** the effective Reverse Chunk in Calf is half the nominal time. Not run.
- dm-Reverse: one delay line, two read heads half a cycle apart. Each is `phasor * time` behind the write pointer, so the delay grows and playback runs backwards. The two heads are crossfaded (S5, `lib.rs` L51-66).
- Stanford EE264 report (Zhang and Chen, Winter 2018): a circular delay line reversed "block by block". The output pointer runs backwards through the previous block while the input fills the current one, then jumps. It states the "maximum block size is half the total length of the circular delay line" (S6, Sec. II-B).
- Roy Fox's JUCE plugin reverses each host block via `AudioBuffer::reverse`. The blog says the crossfade "needs to be smoother" and its code has no fade (S7, `DelayBuffer.cpp` L98-112). Weak evidence on quality.

**Boundary discontinuity**

- Dip (fade to silence). Calf's `overlap_window` is documented in a comment as "warping clicks avoiding between chunks". It is a trapezoid with linear ramps, and the ramp share is the `Window` control, default 0.5 (S3, `audio_fx.h` L739-790, `metadata.cpp` L342). Gx uses the same class (S4). The Stanford report multiplies by `G = 4x(1-x)`, a parabola over the whole block (S6, Eq. 3).
- Overlap. Zynaddsubfx, in AUTO and HOST modes, sums `fadein * new + fadeout * old` with linear complementary weights (S2, `applyFade`, L207-213). dm-Reverse uses `xfade_b = 1 - xfade_a`, also linear and complementary (S5, L60-61).
- **Fade length relative to chunk length:**

| Implementation | Fade length | Short-chunk rule |
|---|---|---|
| Zynaddsubfx | User time, default `(32+1)/100` s (S2 `Reverse.cpp`) | If delay < 1.25 x fade, fade = 0.8 x delay (S2 L258-264) |
| dm-Reverse | Fixed 20 ms (`xfade_factor = time/20`) | Minimum time 20 ms (S5 `.ttl`) |
| Calf, Gx | Fraction of half-chunk, user `Window` | Scales with chunk |
| Stanford | Whole block | Whole block |
| Yaled | Fixed 15 ms at each end | None, see #14 |

**Very short chunks**

- Prior-art floors:
  - Zynaddsubfx: computed as `(0+1)/128 * 4 s`, about 31 ms (S2 `Reverse.cpp`, `globals.h` L189).
  - dm-Reverse: 20 ms (S5).
  - Timeline: 60 ms (S8, PDF p.12).
  - DD-8: 300 ms in REVERSE, against 20 ms for other modes (S9, PDF p.5).
- DL4 MkII manual tip for Reverse: set a "very short delay time" for a "weird 'resonant filter' effect" (S10, PDF p.35). So very short chunks become a tonal effect, not an echo.

**Runtime Chunk Length changes**

- Calf: `params_changed()` resets `counters` to 0 (S3 L666). The read indexes the new length, so the buffer is not the one captured at the old length. Gx sets `counter = 0` when time changes (S4 `compute`). **Reading:** these have the mismatched-audio class of bug in #11. Not run.
- Zynaddsubfx: `setdelay` calls `update_memsize`, which reallocates and `reset()`s (zeroes) the buffer (S2 L215-256, L276-282). This is a hard restart.
- dm-Reverse smooths `time` with `LogarithmicSmooth` (S5, `params.rs`). The delay glides, so changes are continuous but bend pitch.
- Stanford: says smoothing filters on delay-length controls keep the output pointer moving between adjacent samples (S6, Sec. II-A). That is written about ordinary delays.
- **Could not find** an implementation that applies a new length to the next capture only.

### Q2. Feedback

- **Tap point and injection:** Calf and Gx take `feedback_buf = out` before `out *= ow.get()`. They add `feedback_buf * feedback` to the input that is written into the same buffer (S3 L762-767; S4 `compute`). So the tap is pre-window and the injection is into the capturing buffer.
  - Note on Calf: the buffer is also being read, so this is in-place regeneration.
  - dm-Reverse differs. It feeds back a forward delay read at `time` (`write(input + delay * feedback)`), not the reversed output (S5 L36-38).
- **Loop topology (Stanford):** if the reverse module is inside the feedback loop, "the output echoes of this system is by turns reversed and forward" (S6, Fig. 6). For "pure reverse" echoes, the reverse output feeds a separate forward echo (S6, Fig. 7). **Reading:** #4's definition, feeding the Reverse Chunk's output into the next capture, is the Fig. 6 topology. The second generation will play forward, not reversed. The maintainer should decide whether that is wanted.
- **Gain limits:**
  - Calf: feedback range 0 to 1, default 0.5 (S3, `metadata.cpp` L335).
  - Gx: 0 to 1 (S4, port comment).
  - dm-Reverse: 0 to 100 %, scaled by 0.01 (S5 `lv2/src/lib.rs` L44).
  - Stanford: "smaller than 1" (S6).
  - Boss DD-8: FEEDBACK adjusts "the number of times the delay sound is repeated" (S9, PDF p.4).
- **Filtering or damping in the loop:** none in the Calf, Gx or dm-Reverse code. Strymon's Reverse has a post-delay High Pass, and Filter/Grit "shape the fidelity of the repeats" (S8, PDF p.11-12). The manual does not say whether these sit in the feedback loop. Unverified.
- **Reporting the tail to a host:** the two open-source JUCE-style examples I read do not report one. Roy Fox returns `0.0` (S7 `PluginProcessor.cpp` L65-68). I found no tail override in Calf's `modules_delay.h`. Hardware treats trails as a designed feature: Timeline "Persist" and "Spillover" (S8, PDF p.14 and p.22) and DL4 MkII "Bypass Trails" (S10, PDF p.11).

### Q3. Tempo sync and latency

- **Length from tempo:**
  - Calf: `unit = 60 * srate / (bpm * divide)`, `deltime = unit * time` (S3 L659-661), with a BPM range of 30 to 300 (`metadata.cpp` L330).
  - Roy Fox: `(60/bpm) * delayFraction * 4` with no guard against bpm = 0 (S7 L149).
  - DL4 MkII: subdivisions "from 1/8th note triplet to dotted 1/2 note" (S10, PDF p.6).
  - Zynaddsubfx HOST mode does not derive a length. It locks chunk switches to beat positions and force-switches at 4 s (S2 `Reverse.cpp` L104-171, `Reverter.cpp` L117-121).
- **Extreme tempi:** hardware and plugins clamp to a range (DD-8 reverse 300-5000 ms, Timeline 60-2500 ms; S9, S8). This is the usual answer.
- **Latency:** Stanford writes that the "forward" delay length "keeps changing as the output pointer moves away from the input pointer" (S6). **Reading:** in a Dual-Buffer Engine, output at position `p` of a chunk lags the corresponding input by `2p+1` samples. That is 1 to `2L-1`, so there is no fixed group delay to report. I found no reverse-delay code that calls `setLatencySamples`. The VST3 docs say latency should be "zero or small" for live recording (S12).
- **Tail (standards):** JUCE `getTailLengthSeconds()` maps to VST3 `getTailSamples()` (`<= 0` gives `kNoTail`, infinity gives `kInfiniteTail`) and to AU `GetTailTime()` (S13, `juce_audio_plugin_client_VST3.cpp` L3482-3490; `_AU_1.mm` L1202). The VST3 docs give a reverb example of returning the maximum length, and say tail is for avoiding "signal cut (clicks)" (S12, secondary read via fetch summarizer).

### Q4. Stereo

- Zynaddsubfx: with `Pstereo` off, L and R are summed and one Reverter runs; with it on, two Reverters run with identical parameters and sync (S2 `Reverse.cpp` L92-102, 176-178). Independent buffers with shared timing behave like Linked Stereo Processing.
- Calf: independent per-channel times (`time_l`, `time_r`) plus a `width` control that cross-mixes both the input and the feedback between channels (S3 L752-760). This is a ping-pong-like variant.
- Boss DD-8: in stereo mode REVERSE offers "Completely independent delays for A and B", the factory setting (S9, PDF p.18).
- dm-Reverse: "A mono reverse delay effect" (S5 README).
- DL4 MkII: ping-pong is a separate model, not a reverse option (S10, PDF p.26 and p.35).

### Q5. Crossfade curves (theory)

- Fink, Holters and Zölzer (DAFx-16): "Uncorrelated signals are cross-faded using the so-called Equal Power Crossfade featuring a -3 dB weighting in the transition center", correlated ones linearly with -6 dB. Linear and square-root fades "is solely power-complementary for fully correlated (r = 1) and uncorrelated signals (r = 0), respectively", with deviations over 1 dB in between. The same paper gives 10-30 ms as a typical crossfade time for splicing takes (S11).
- Julius O. Smith, *Spectral Audio Signal Processing*: the Hann window "can be used with 50% overlap", and "any positive COLA window can be split into an analysis and synthesis window pair by taking its square root" (S14, secondary read via fetch summarizer). So Hann overlaps are amplitude-complementary, which is the linear-like case.
- **Reading:** in an overlap crossfade the two heads read different parts of the recent past, which for music is closer to uncorrelated (equal-power). Tonal or periodic material is partly correlated, so neither curve is perfect. The prior art uses linear for overlap (Zynaddsubfx, dm-Reverse), and its listening results are not documented. Not verified by listening.

## 3. Implications for Yaled

| Item | Prior art says | Status |
|---|---|---|
| **ADR-0001 (dual-buffer + crossfade)** | Dual buffers are described in KVR by Urs only. Ring-buffer, in-place and two-head designs are more common in readable code. | **Partly contradicts the wording** "standard technique". The design choice itself is supported as one valid option. Suggest amending the ADR wording. |
| **ADR-0001 / CONTEXT.md "Chunk Boundary Crossfade" as dip** | Both dips (Calf, Gx, Stanford) and overlaps (Zynaddsubfx, dm-Reverse) ship. Yaled's dip is not a crossfade in the overlap sense. | **Flag:** the code is a dip, and the term "crossfade" applies to the overlap option. Wording gap already noted in #13. |
| **#13 dip vs overlap** | Split. Linear complementary is the norm for overlap. Equal-power is theoretically better for uncorrelated overlap (S11). | Supports "keep the dip; overlap as an option". Curve choice: silent from listening data. |
| **#14 short chunks** | Fade scaled to chunk (Zynaddsubfx 0.8 cap, dm-Reverse, Calf). Floors on hardware. | Supports `min(crossfade, chunk/2)`. Consider a higher minimum than 10 ms (see S2, S5, S8, S9). Tempo sync can still reach 1 sample via `jlimit(1, ...)`. |
| **#11 length change** | No readable implementation plays each chunk at its captured length. Calf and Gx have the same class of defect. | Silent. Proposed fix follows from the theory. No precedent to copy. |
| **#4 Feedback Path** | Pre-window tap into capture (Calf, Gx). Reverse-in-loop makes alternating reversed/forward echoes (S6). Gain up to 1.0 with no loop filter. | Supports pre-window tap, which also avoids the compounding dips in #13. **New:** second-generation echoes play forward. Cap below unity is stricter than prior art but sensible. |
| **#5 Linked Stereo** | Zynaddsubfx's stereo mode is equivalent. Independent times and width exist in Calf. | Supports v1 scope. Independent channels are a later feature. |
| **`getTailLengthSeconds()` returns 0** | Standards allow the maximum. Trails are a designed feature in hardware. **Reading:** after input stops, audible output can last up to `2L` (finish capturing, then play back), plus roughly `L` per feedback regeneration. With max 2000 ms that is at least 4 s. | **Likely wrong.** Prior art in code is silent (Roy Fox also returns 0). Standards support a conservative constant. |
| **Latency** | No fixed latency exists (see Q3). | Supports reporting 0. |
| **Tempo Sync** | Clamping to a range is normal. Zero BPM is not guarded in Roy Fox. | Yaled's `noteDivisionToSamples` divides by `bpm` unguarded (`TempoSync.cpp` L24). At 120 BPM a whole note equals the 2000 ms max, so slower tempi clamp. Consider a `bpm <= 0` guard. |

## 4. Sources

All accessed 2026-09-20.

| ID | Source | Type | Primary? | Used for |
|---|---|---|---|---|
| S1 | KVR Audio thread "How does reverse delay effect work?" https://www.kvraudio.com/forum/viewtopic.php?t=599376 (Rainwaves, mystran, Urs, kerfuffle, Richard_Synapse, earlevel; 14 posts, 2023-07-19 to 2023-08-14) | Forum | Primary for what posters said | Design origin; dual delay lines; crossfade; sync warning. Quotes taken from a second fetch asking for verbatim text. |
| S2 | zynaddsubfx `src/DSP/Reverter.cpp`, `Reverter.h`, `src/Effects/Reverse.cpp`, `src/globals.h` at commit `c379439e41b34dee5eededbf5b13e7514c4ded64`. https://github.com/zynaddsubfx/zynaddsubfx/blob/c379439e41b34dee5eededbf5b13e7514c4ded64/src/DSP/Reverter.cpp | Source code | Primary | Ring buffer, overlap crossfade, short-chunk cap, stereo, host sync, length change |
| S3 | Calf Studio Gear `src/modules_delay.cpp`, `src/calf/audio_fx.h`, `src/metadata.cpp` at `ecbbb2d68a5066a9ee30e56c6a20635a20f03ee4`. https://github.com/calf-studio-gear/calf/blob/ecbbb2d68a5066a9ee30e56c6a20635a20f03ee4/src/modules_delay.cpp | Source code | Primary | In-place buffer, dip window, feedback tap, BPM formula, stereo |
| S4 | brummer10/GxReverseDelay.lv2 `dsp/reversedelay.cc` at `3207aec0c7d9164cf4717c9dde82caf61e2ba274`. https://github.com/brummer10/GxReverseDelay.lv2/blob/3207aec0c7d9164cf4717c9dde82caf61e2ba274/dsp/reversedelay.cc | Source code | Primary | Same window class as Calf; feedback; counter reset on time change. Derived from Calf: unverified. |
| S5 | davemollen/dm-Reverse `reverse/src/lib.rs`, `params.rs`, `lv2/src/lib.rs`, `lv2/dm-Reverse.lv2/dm-Reverse.ttl` at `67b68d0c084da0f116bff6c230896af39c408faf`. https://github.com/davemollen/dm-Reverse/blob/67b68d0c084da0f116bff6c230896af39c408faf/reverse/src/lib.rs | Source code | Primary | Two-head design, 20 ms fade, smoothing, feedback range |
| S6 | Zhang and Chen, "A Pitch Shifting Reverse Echo Audio Effect", EE264 project report, CCRMA, Winter 2018. https://ccrma.stanford.edu/~jingjiez/portfolio/echoing-harmonics/pdfs/A%20Pitch%20Shifting%20Reverse%20Echo%20Audio%20Effect.pdf (linked by kerfuffle in S1) | Student report | Primary for its claims, not peer-reviewed | Block reversal, parabolic window, feedback topologies |
| S7 | royfox/juce-projects `FoxDelay/Source/PluginProcessor.cpp`, `DelayBuffer.cpp` at `426487caac086258c9909e23209d159f96997a8a`; blog https://www.royfox.co.uk/2022-10-04/reverse-delay (blog read via fetch summarizer) | Source code and blog | Primary for his own plugin | Simple JUCE example, tail 0.0, BPM formula |
| S8 | Strymon TimeLine User Manual REV F. https://www.strymon.net/manuals/TimeLine_UserManual_REVF.pdf (read locally as text) | Manual | Primary | Reverse type, time ranges, persist/spillover |
| S9 | Boss DD-8 Reference Manual. https://static.roland.com/assets/media/pdf/DD-8_Reference_eng01_W.pdf | Manual | Primary | Reverse range, feedback, stereo modes |
| S10 | Line 6 DL4 MkII Owner's Manual. https://line6.com/data/6/0a00050b29426643c77742426/application/pdf/DL4%20MkII%20Owner's%20Manual%20-%20English%20.pdf | Manual | Primary | Reverse model, short-time tip, trails, subdivisions |
| S11 | Fink, Holters, Zölzer, "Signal-matched power-complementary cross-fading and dry-wet mixing", DAFx-16, Brno, 2016. https://www.hsu-hh.de/ant/wp-content/uploads/sites/699/2017/10/Fink-Holters-Z%C3%B6lzer-2016-Signal-matched-power-complementary-cross-fading-and-dry-wet-mixing.pdf | Conference paper | Primary | Curve theory |
| S12 | VST3 `IAudioProcessor` docs. https://steinbergmedia.github.io/vst3_doc/vstinterfaces/classSteinberg_1_1Vst_1_1IAudioProcessor.html (read via fetch summarizer) | Standard docs | Primary | `getTailSamples`, `getLatencySamples` |
| S13 | JUCE at `72782788ce18c2d4d760b28e0921d6ffc6431102`: `modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp`, `juce_audio_plugin_client_AU_1.mm`, `modules/juce_audio_processors_headless/processors/juce_AudioProcessor.h` | Source code | Primary | How tail and latency reach the host |
| S14 | J. O. Smith, *Spectral Audio Signal Processing*, "COLA Examples". https://ccrma.stanford.edu/~jos/sasp/COLA_Examples.html (read via fetch summarizer) | Book (online) | Primary | Hann COLA statements |

## 5. Open questions and things I could not verify

- **Commercial plugins and hardware internals.** I found no source or manual that says whether Strymon, DL4, DD-8, or Eventide use dips or overlaps, or how long their crossfades are. The manuals only describe behaviour. I did not obtain a TC Electronic Flashback manual (the host was unreachable) or any Eventide or Ableton documentation.
- **Airwindows, SuperCollider, Pure Data, Csound, Faust.** I listed the Airwindows plugin folder names and none is a reverse delay. I found no readable reverse delay in the others.
- **Whether GxReverseDelay is derived from Calf.** The window class is structurally identical, but I did not check history.
- **Calf and Gx behaviour** (half-length effective chunk, mismatch on time change) is my reading of the code. I did not build or run them.
- **Listening quality of linear vs equal-power** for reversed overlaps. The theory (S11) is stated for random signals. No listening evidence was found. The `prototypes/crossfade-listening-test.html` prototype is the place to test.
- **Where Strymon's High Pass and Filter sit relative to feedback.**
- **Pages for S8-S10** are PDF page indices from my text extraction, not necessarily printed page numbers.
- **The Stanford report is a student project.** Its window formula and topologies are a valid derivation but not evidence of what commercial products do.
- **KVR quotes** came from a fetch that first returned a summary. I re-fetched for verbatim text, but I cannot check them against the live page beyond that.
