# Handoff: drafting new captions for the CyclopsGaze demo clip

For a session picking this up cold and asked for **new caption ideas**. Everything needed to draft
without re-deriving it, plus the things already tried and why they were rejected.

## Read this first

**This file is a pointer, not truth.** The local repo and the footage win. If this file and a live
file disagree, this file is stale: say so out loud and correct it. Every state line below records
how it was checked so you can re-run the check instead of trusting it.

**Staleness check, run both:**

```
git log -1 --oneline                                   # where the repo is now
git log -1 --oneline -- docs/media/CAPTIONS_HANDOFF.md # when this file was last true
```

Same hash means nothing has landed since this was written. Different means it is stale by exactly
that gap, so re-verify anything below that matters to your draft.

**The one rule that matters most here:** the repo describes the firmware, a caption describes the
video. **When they disagree, the video wins.** Sample frames before writing a word. This has
already caught two false captions that the repo's own documentation would have supported.

## The job

Turn a 39 second clip of the rig tracking a face into captions for YouTube plus short-form
crossposts. Muted autoplay is the default viewing condition, so the captions carry the whole story.
Audience is makers, animatronics, props and cosplay people who have seen many wandering "uncanny
eyes" builds and never one that locks onto a face.

Existing drafts, the shipped cue list, post copy and the publishing plan are in
**[VIDEO_CAPTIONS.md](VIDEO_CAPTIONS.md)**. Read it before drafting; do not duplicate it here.

`/demo-video` is the reusable command that performs this whole job, with its kit at
`C:\Users\SuperMaster\.claude\demo-video-kit\` (`burn.py` plus a README of ffmpeg traps). Note both
live outside this repo and are not version controlled.

## The footage

| Fact | Value | How checked |
|---|---|---|
| Master clip | `C:\Users\SuperMaster\Videos\CyclopsGaze\CyclopsGaze_reframed_stabilized_1080p.mp4` | `ls` of that folder |
| Duration | **38.702 s** | `ffprobe -show_entries format=duration` |
| Resolution / fps | 1920x1080, ~29.99 fps | `ffprobe -show_entries stream=width,height,r_frame_rate` |
| Audio | AAC track present, content unexamined | `ffprobe` stream list |
| Original camera file | `2026-07-24 12.07.03.mp4` (204 MB) | `ls` |

Stabilised by ChatGPT outside this repo. There is a DaVinci Resolve project, but it lives in
Resolve's own database, **not** in that folder (checked: the folder holds only media and subtitle
files).

**What is actually on screen**, from sampled frames, not assumption:

- **One** round GC9A01A eye running the **nordicBlue** artwork. No second display, no `hazel`, no
  eye-set rotation at any point.
- The rig: purple translucent laser-cut stand, display at the top, SEN0626 camera mounted directly
  beneath it, Teensy on a white breadboard to the right, loose dupont harness in shot.
- Handheld camera **orbits the rig for the full clip**, and the iris stays turned toward the
  operator as the viewpoint moves. That orbit is the money shot and is why an opening cue can state
  the trick outright.
- Autonomous blinks and visible pupil dilation throughout, roughly one lid close near 12.9 s.
- **Never on screen:** a human face, a serial monitor, any text, a second eye, the eye losing the
  face and resuming idle wander.

## Hard constraints on any new draft

1. **Never claim what the footage does not show.** Two specific traps, both real:
   - The firmware bundles ten eye designs and can rotate them. **The clip shows one.** A caption
     about switching looks would be contradicted on screen.
   - Idle-wander-on-face-loss is genuine firmware behaviour, but the operator never leaves frame,
     so it must be phrased as capability ("with no face in frame, it wanders"), never as something
     the viewer is watching happen.
2. **Plain language.** No term a non-embedded viewer must decode. The operator rejected "face
   center out over UART" explicitly. Part numbers belong in the description, not the captions.
3. **No em dashes and no AI writing tells**, in captions or post copy.
4. **Repo status vocabulary never appears.** REPO-ONLY / DEPLOYED / VERIFIED mean nothing to
   viewers.
5. **Bottom centre placement**, operator preference. 44 px bold, 48 px off the bottom on 1080p.
6. Around 11 cues for 39 s, 2 lines maximum per cue, roughly 3 to 3.5 s each.

## Verified facts you may use

Each was checked during the sessions that produced the current cut. Re-verify anything load-bearing.

| Claim | Status | Source |
|---|---|---|
| Build in the video costs **$63.90** | verified | [BOM.md](../BOM.md), "The VERIFIED build" row |
| Extra displays are **under $12** (generic ~$8.60) | verified | [BOM.md](../BOM.md) display rows |
| **$47 is the wrong number** for this rig; it is the unbuilt ESP32-S3 route | verified | BOM.md, and README's intro still quotes it |
| The camera does face detection **on the camera**, no Pi/PC/network | verified | driver + [SEN0626_PROTOCOL.md](../SEN0626_PROTOCOL.md) |
| Sensor talks Modbus RTU over UART at 9600, two data wires | verified | driver, and observed on the wire |
| Blink, pupil and idle drift are autonomous; only gaze comes from the camera | verified | `EyeController`, and visible in the clip |
| Tuning is live over serial with no reflash | verified on the wire | `PS_CFG:` acks read on COM6 |
| The driver **is running in IRIS right now** | verified | CHANGELOG CG-S11, README Status |
| IRIS project repo is **public**: `https://github.com/Maestro8484/IRIS` | verified | GitHub API returned 200; `IRIS-Robot-Face` returns 404 unauthenticated, so that one is private and must not be linked |
| Eye render engine is TeensyEyes by Chris Miller, MIT | verified | [ATTRIBUTION.md](../ATTRIBUTION.md). **Credit it in the description.** The nordicBlue artwork on screen is the operator's own |

Current firmware state, if a caption ever needs it: `FIRMWARE_VERSION` is `CG-S17d` and
`PS_CONF_GATE_DEFAULT` is 55 (read from `src/config.h`), running at roughly 19-22 FPS.
**None of that belongs in captions**; it is here so you do not state something contradictory.

## What has been tried, and the verdicts

Full text of each is in [VIDEO_CAPTIONS.md](VIDEO_CAPTIONS.md).

- **Draft A, build-log voice.** Cold open on "The eye is following my face", spec beats, IRIS last.
  Operator verdict: **too technical, not engaging.** Stated what the thing was before giving anyone
  a reason to care.
- **Draft B, retention cut.** 14 fast cues, second person, "Walk around it / It keeps looking at
  you". Not chosen. The operator is the one walking, so the line invites something the clip does not
  let the viewer do.
- **Draft C, documentary.** 8 cues, long holds, explains why the project exists. Operator verdict:
  **most engaging voice**, but too slow for short form and the worst offender on jargon.
- **Draft D, shipped.** C's spine and hook, A's cue density, every technical phrase rewritten
  plainly. This is the current `CyclopsGaze_captions_v3.srt`.

**Operator preferences, stated explicitly:**

- Liked: brief punchy lines that make sense to anyone, specifically "No Pi. No PC. No wifi."
- Wanted: the price, **and** that a second eye is under $12.
- Wanted: IRIS named and linked, "IRIS is using it right now".
- Wanted: the rewrite pattern "face center out over UART" into something like "eye gaze is centred
  on the tracked face over just two wires".

**Structural things that worked in D**, worth keeping or deliberately breaking:

- Opening on a problem rather than a spec.
- Contrasting against timer-driven eyes early, since that is what this audience has already seen.
- Giving the single most impressive claim its own cue.
- A late four-word pivot ("None of this was the point") that turns a gadget demo into a story, with
  the IRIS reveal as the payoff in the final two cues.

## Angles not yet tried

Offered as starting points, not recommendations:

- **Cold open on the reveal.** Lead with the discontinued-part problem and let the eye be the
  answer, inverting D's structure.
- **Second person throughout**, done properly this time: "it is looking at you" rather than asking
  the viewer to walk.
- **Numbers-forward**, for a build-curious audience: cost, part count, frame rate, distance.
- **No-narration cut**, letting three or four very sparse cues carry a mostly-visual piece.
- **Vertical-first**, written for a 9:16 crop where cue length and safe area both change. The eye
  stays in the upper half throughout the orbit, so a centre crop keeps it.

## Tooling

Burn and check in one step, from the kit:

```
python C:\Users\SuperMaster\.claude\demo-video-kit\burn.py \
    C:\Users\SuperMaster\Videos\CyclopsGaze\CyclopsGaze_reframed_stabilized_1080p.mp4 \
    <your.srt> --check 5
```

Produces a 1080p burn with audio, a ~2 MB web copy, and one PNG frame. **Look at the frame.** The
kit README documents four ffmpeg traps, the nastiest being that pixel values passed to
`force_style` are wrong by about 3.75x because ffmpeg writes `PlayResY: 288` into the converted ASS.

For Resolve, import the SRT (`File > Import > Subtitle`); its default subtitle position is already
bottom centre.

## Open, not blocking a new draft

- The video has not been published anywhere yet. Plan is YouTube as canonical host, a thumbnail
  link in the README, and possibly a Hackaday.io project page. README video block and Hackaday copy
  are drafted nowhere yet.
- README's intro still quotes `$47`, which is the unbuilt ESP32-S3 figure rather than the rig on
  screen. Not corrected, since it arguably describes the cheapest route.
