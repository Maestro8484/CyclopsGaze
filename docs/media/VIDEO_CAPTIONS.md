# Demo video: captions and post copy

Source clip: `C:\Users\SuperMaster\Videos\CyclopsGaze\CyclopsGaze_reframed_stabilized_1080p.mp4`
Measured with `ffprobe` 2026-07-25: **38.702 s, 1920x1080, ~30 fps, AAC audio track present.**
Shipped caption file: `CyclopsGaze_captions_v2.srt` in that same folder (Draft A below, tightened).

Audience: YouTube plus Reels/Shorts/TikTok crossposts. Viewer is an animatronics / props /
cosplay / maker person who has seen a hundred wandering "uncanny eyes" builds and has not seen
one that actually locks onto a face. The captions carry the whole story because most of these
plays are muted.

IRIS is named exactly once, in the last cue, with no explanation. That is the intended
subtlety: a name and a claim, no lore.

---

## What the footage actually shows

Verified by sampling frames out of the stabilized clip, not assumed:

- One round GC9A01A eye running **nordicBlue** only. No `hazel`, no rotation, no second display.
  (Matches the working copy of `src/config.h`, where the second set is commented out.)
- The rig: purple translucent stand, display up top, SEN0626 camera mounted directly beneath it,
  Teensy on a white breadboard to the right, loose jumper harness in shot.
- Handheld camera orbits the rig for the full 39 s. **The iris stays pushed toward the operator's
  side of the display as the viewpoint moves.** That orbit is the money shot and it is why the
  opening cue can state the trick outright.
- Autonomous blinks and visible pupil dilation throughout (roughly one lid close near 12.9 s).
- No serial monitor, no screen text, no face in frame at any point.

Two things the clip therefore must not claim: an on-screen serial readout, and the
lose-the-face-then-regain-it behavior (the operator never leaves frame).

---

## Draft A: cold open, build-log voice

The hook states the trick in the first two seconds instead of introducing the project. Spec
beats in the middle, IRIS as the closing line. This is the one that shipped.

| # | In | Out | Text |
|---|---|---|---|
| 1 | 0.20 | 3.00 | The eye is following my face. |
| 2 | 3.20 | 6.40 | The gaze is not on a timer. / It is tracking me live. |
| 3 | 6.60 | 10.00 | One round LCD, one AI camera, / one Teensy 4.1. |
| 4 | 10.20 | 13.60 | The camera detects the face itself. / No Pi, no PC in the loop. |
| 5 | 13.80 | 17.20 | It sends the face center over UART. / Modbus RTU, 9600 baud. |
| 6 | 17.40 | 20.80 | The Teensy maps that to a gaze target. |
| 7 | 21.00 | 24.20 | Blinking, pupil dilation and saccades / are all autonomous. |
| 8 | 24.40 | 27.60 | With no face in frame, / it wanders on its own. |
| 9 | 27.80 | 31.00 | Gaze range and gate tune live / over serial. No reflash. |
| 10 | 31.20 | 34.20 | Built because my robot face's sensor / was discontinued. |
| 11 | 34.40 | 38.50 | This one drops in unchanged. / It is in IRIS now, tracking live. |

## Draft B: retention cut, second person, fastest turnover

Fourteen cues, ~2.5 s each, written for a vertical crop where the viewer decides in one second.
Leans on "you" and on the one comparison that lands with this audience: most eye builds wander.
Costs the technical detail. Use this one for Shorts/Reels if A tests slow.

| # | In | Out | Text |
|---|---|---|---|
| 1 | 0.10 | 2.20 | Walk around it. |
| 2 | 2.40 | 4.60 | It keeps looking at you. |
| 3 | 4.80 | 7.20 | Most animatronic eyes just wander. |
| 4 | 7.40 | 9.80 | This one has a camera. |
| 5 | 10.00 | 12.60 | The camera finds your face on its own. |
| 6 | 12.80 | 15.20 | Nothing else is doing the thinking. |
| 7 | 15.40 | 18.00 | No Pi. No PC. No wifi. |
| 8 | 18.20 | 20.80 | Face position goes straight to the Teensy. |
| 9 | 21.00 | 23.60 | The Teensy points the eye at you. |
| 10 | 23.80 | 26.40 | The blinks are its own idea. |
| 11 | 26.60 | 29.20 | About 64 dollars of parts. |
| 12 | 29.40 | 32.00 | It was never meant to work alone. |
| 13 | 32.20 | 35.20 | It is the spare eye for a robot face / called IRIS. |
| 14 | 35.40 | 38.50 | That one is already using it. |

## Draft C: quiet documentary, craft voice

Eight cues, longer holds, more room to just watch the eye. Explains *why* the thing exists
rather than what it is made of. Best fit for the YouTube upload if the eye's motion is the
draw; too slow for TikTok.

| # | In | Out | Text |
|---|---|---|---|
| 1 | 0.30 | 4.80 | An animated eye is easy. / Making it look at you is the hard part. |
| 2 | 5.00 | 9.80 | Wandering eyes read as a toy. / A tracked gaze reads as attention. |
| 3 | 10.00 | 15.00 | So the eye gets a camera that finds / faces by itself, and nothing else. |
| 4 | 15.20 | 20.20 | Face center out over UART. / Gaze target in. One Teensy between them. |
| 5 | 20.40 | 25.40 | Everything else it does, the blinks, / the pupil, the drift, it does alone. |
| 6 | 25.60 | 30.60 | The camera sits below the eye, / so its idea of level had to be measured. |
| 7 | 30.80 | 35.00 | None of this was the point. / The point was replacing a dead sensor. |
| 8 | 35.20 | 38.50 | The robot it was built for is / using it right now. |

---

## Why Draft A shipped

- The orbit in the footage proves the tracking claim on its own, so the strongest opening is
  the flat statement of what you are watching. B's "walk around it" is a better hook in the
  abstract but the operator is the one walking, not the viewer, and the line invites a viewer to
  do something the clip does not let them do.
- A's cue 10 and 11 put the discontinued-sensor reason and the IRIS name back to back, which is
  what makes the IRIS mention land as a payoff instead of a credit.
- B quotes the price, A does not. Both are fine; the number is real ($63.90, BOM.md) but it dates
  the clip, and a price in cue 11 competes with the IRIS reveal for the last thing a viewer reads.
- C is the honest edit and the weakest social edit. Its cue 6 (the camera mounting below the eye
  and the measured Y offset) is the single most interesting engineering detail in the project and
  it is completely invisible to a scrolling viewer.

## Wording rules applied to all three

- Nothing claims a behavior the footage does not contain. A's cue 8 is phrased as a capability
  ("with no face in frame, it wanders") rather than "watch it lose me", because the operator
  never leaves frame.
- "Tracking live" is only used of IRIS, which is where it is true (README § Status, CHANGELOG
  CG-S11). The standalone rig's own status word is not used in the captions at all; REPO-ONLY vs
  VERIFIED is repo vocabulary and means nothing to this audience.
- No price in A. If you want one, `$63.90` is the T4.1 build in the video. **Not $47**, which is
  the unbuilt ESP32-S3 path.
- One line of credit belongs in the description, not the captions: the eye render engine is Chris
  Miller's TeensyEyes (MIT). The nordicBlue iris artwork on screen is the project author's own.

---

## Post copy

### YouTube title options

1. This animatronic eye actually follows your face
2. I gave an animatronic eye a camera so it looks at you
3. Face-tracking animatronic eye, Teensy 4.1 and a $15 AI camera

### YouTube description

    An animatronic eye that tracks a real face instead of wandering on a timer.

    A DFRobot SEN0626 AI vision camera does the face detection on the camera itself and
    reports the face center over UART (Modbus RTU, 9600 baud). A Teensy 4.1 maps that
    position to a gaze target and drives a 1.28 inch 240x240 round GC9A01A display. No Pi,
    no PC, no network anywhere in the loop. The blinking, pupil dilation and idle saccades
    are autonomous; only the gaze direction comes from the camera. Tracking range, the
    confidence gate and the per-axis gaze gain all tune live over serial with no reflash.

    Parts are about $64 domestic for the build in this video.

    It was built for a reason that is not visible in the clip. The tiny Useful Sensors
    Person Sensor that drove the gaze on my robot face, IRIS, was discontinued, and a
    project meant to be publicly replicable cannot depend on a part nobody can buy. So this
    driver presents the exact same struct and method surface the old sensor did, which means
    IRIS consumed it with almost no changes. It is running in IRIS now.

    Firmware, wiring tables, sourced parts list and the full engineering log:
    https://github.com/Maestro8484/CyclopsGaze

    The eye rendering engine is TeensyEyes by Chris Miller (MIT), itself adapted from
    Adafruit's Uncanny Eyes. The iris and sclera artwork in this eye is mine.

### Short social caption (Instagram, TikTok, Mastodon, Bluesky)

    Most animatronic eyes wander on a timer. This one has an AI camera under it, so it
    looks at whoever is actually there. Teensy 4.1, one round LCD, about $64 of parts, no Pi
    and no PC in the loop. Built as the replacement eye sensor for my robot face after the
    original part got discontinued. Build notes are on GitHub, link in bio.

### Tags

    animatronics, animatronic eye, face tracking, teensy, teensy 4.1, platformio,
    uncanny eyes, teensyeyes, gc9a01a, dfrobot, sen0626, modbus, halloween prop,
    cosplay, puppet, robot face, diy electronics, embedded, arduino

### Notes for the Resolve timeline

- Import `CyclopsGaze_captions_v2.srt` (File > Import > Subtitle) onto its own subtitle track.
  Cue text uses a hard line break where two lines are intended.
- The rig is bottom-centre to bottom-right in most of the orbit, so captions read best pinned to
  the **upper third**, not Resolve's default lower third.
- The display is a bright blue emissive object against a white desk. Use a solid or heavily
  darkened caption background rather than a drop shadow; white-on-white is the failure mode here.
- Vertical crop for Shorts/Reels: the eye stays in the upper half of the 16:9 frame for the whole
  orbit, so a centre 9:16 crop keeps it. Re-check around 15 s, the lowest camera angle in the clip.
