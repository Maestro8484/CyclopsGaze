# Archive — historical development handoffs

These are the raw, session-by-session working documents from CyclopsGaze's development
(session primers, peer-review notes, model handoffs, the original integration plan, and the
lateral-tracking investigation). They are kept for provenance and design context, **not** as
current reference.

**Read the curated docs instead** for anything you actually want to do:
- [../WIRING.md](../WIRING.md) — wiring
- [../SEN0626_PROTOCOL.md](../SEN0626_PROTOCOL.md) — protocol/registers
- [../BENCH_PROTOCOL.md](../BENCH_PROTOCOL.md) — first-flash procedure
- [../IRIS_INTEGRATION.md](../IRIS_INTEGRATION.md) — the IRIS drop-in
- [../ENGINEERING_LOG.md](../ENGINEERING_LOG.md) — the full engineering record
- [../../CHANGELOG.md](../../CHANGELOG.md) — history CG-S1 … CG-S12

## Caveats when reading these

- **They use the old file layout.** Cross-references like `NOTES.md`, `05_WIRING.md`, and
  `09_IRIS_INTEGRATION_PLAN.md` point at pre-reorganization paths (now `ENGINEERING_LOG.md`,
  `WIRING.md`, `IRIS_INTEGRATION.md`). Left as-written for historical fidelity.
- **They predate the CG-S12 sync.** Confidence is discussed on the old 0–255 scale (gate 152) and
  gaze on the old single-`GAZE_GAIN` + `Y_CENTER` scheme. The current source and the newest
  CHANGELOG entry win where they disagree.
- **They reference the parent IRIS project** (a separate, private repo) — file paths and design
  decisions there are context, not part of this repo.

## Contents

| File | What it is |
|---|---|
| `00_SETUP.ps1` | one-time environment setup script |
| `01_CYCLOPSGAZE_RULES.md` | original session rules (superseded by [../../RULES.md](../../RULES.md)) |
| `02_CLAUDE_CODE_HANDOFF.md` | initial build handoff |
| `03_README_TEMPLATE.md` | early README draft |
| `04_NOTES_TEMPLATE.md` | notes scaffold |
| `06_SESSION_PRIMER.md` | session-start primer |
| `07_PEER_REVIEW_HANDOFF.md` | peer-review pass of the tracking chain |
| `08_GESTURE_INTEGRATION_PROPOSAL.md` | (exploratory) gesture-integration proposal |
| `09_IRIS_INTEGRATION_PLAN.md` | original detailed integration plan (superseded by [../IRIS_INTEGRATION.md](../IRIS_INTEGRATION.md)) |
| `10_OPUS_HANDOFF.md` | firmware-audit + dual-eye handoff |
| `11_HANDOFF_FABLE_LATERAL_TRACKING.md` | lateral-tracking investigation handoff |
| `12_HANDOFF_ALLINONE.md` | all-in-one lateral-tracking handoff (supersedes 11) |
