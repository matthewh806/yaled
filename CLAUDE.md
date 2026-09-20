## Agent skills

### Issue tracker

Issues live as GitHub Issues; skills use the `gh` CLI. See `docs/agents/issue-tracker.md`.

### Triage labels

Default canonical labels (`needs-triage`, `needs-info`, `ready-for-agent`, `ready-for-human`, `wontfix`). See `docs/agents/triage-labels.md`.

### Domain docs

Single-context layout (`CONTEXT.md` + `docs/adr/` at repo root). See `docs/agents/domain.md`.

### Testing convention

TDD (red-green, seam confirmed with the user first) is for DSP-adjacent logic only: the reverse engine itself (`source/dsp/`) and standalone pure functions like tempo conversion. Parameter/APVTS wiring into `PluginProcessor` is plumbing — implement directly, then cover it with an integration test that drives the real `PluginProcessor::processBlock` (see `tests/PluginProcessorChunkLengthTests.cpp` / `PluginProcessorMixTests.cpp` for the pattern). Note: driving `PluginProcessor` directly in a test requires calling `setRateAndBufferSizeDetails()` before `prepareToPlay()`, since a real host wrapper does that step invisibly.

Before calling any DSP ticket done: rebuild all plugin targets, run the full `ctest` suite, and validate both VST3 and AU with `pluginval --strictness-level 10`. Unit tests alone don't catch parameter-wiring or host-integration mistakes.

### Branch and PR workflow

Never commit directly to `main`: a repo ruleset requires a pull request with passing `macOS` and `Windows` CI checks, and only allows squash merges.

- One branch per issue, named `<type>-issue-<N>-<slug>`, e.g. `feat-issue-4-feedback-path` or `bug-issue-12-fix-tail-length`. Docs-only changes with no issue use `docs-<slug>`.
- Open a PR with `Closes #N` in the description so the issue closes on merge.
- Run `/code-review` against `main` before merging and address what it finds. No approving reviews are required (solo project); the review is a quality gate, not a permission gate.
- Squash-merge, so `main` keeps one commit per ticket.
