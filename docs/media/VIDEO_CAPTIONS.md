# Demo video: captions and post copy

Source clip: `C:\Users\SuperMaster\Videos\CyclopsGaze\CyclopsGaze_reframed_stabilized_1080p.mp4`
Measured with `ffprobe`: **38.702 s, 1920x1080, ~30 fps, AAC audio track present.**

**Shipped caption file: `CyclopsGaze_captions_v3.srt`** (Draft D below), in that folder and mirrored
here as `CyclopsGaze_captions_v3.srt`.

Audience: YouTube plus Reels/Shorts/TikTok crossposts. The viewer is an animatronics / props /
cosplay / maker person who has seen a hundred wandering "uncanny eyes" builds and has never seen
one lock onto a face. Captions carry the whole story because most of these plays are muted.

**Plain language is the rule.** No jargon a non-embedded viewer would have to decode. "Face center
out over UART" fails that test; "the camera sends the face position over two wires" passes. Short
declarative beats, one idea per cue.

IRIS is named once, in the final cue, as a payoff rather than an explanation.

---

## What the footage actually shows

Verified by sampling frames, not assumed:

- One round GC9A01A eye running **nordicBlue** only. No `hazel`, no rotation, no second display.
- The rig: purple translucent stand, display up top, SEN0626 camera mounted directly beneath it,
  Teensy on a white breadboard to the right, loose jumper harness in shot.
- Handheld camera orbits the rig for the full 39 s, and **the iris stays turned toward the operator
  as the viewpoint moves.** That orbit is the money shot and is why the opening can state the trick.
- Autonomous blinks and visible pupil dilation throughout.
- No serial monitor, no screen text, and no face in frame at any point.

So the clip must not claim an on-screen serial readout, eye-set rotation, or the
lose-the-face-then-regain-it behaviour (the operator never leaves frame).

---

## Draft D: the shipped cut

Draft C's voice and pacing, Draft A's brevity, and plain wording throughout. Eleven cues.

| # | In | Out | Text |
|---|---|---|---|
| 1 | 0.20 | 3.20 | An animated eye is easy. / Making it look at you is not. |
| 2 | 3.40 | 6.60 | Most of these wander on a timer. / This one is watching me. |
| 3 | 6.80 | 10.00 | A tiny AI camera under the eye / finds my face on its own. |
| 4 | 10.20 | 13.20 | No Pi. No PC. No wifi. |
| 5 | 13.40 | 16.80 | The camera sends the face position / over two wires. |
| 6 | 17.00 | 20.40 | A Teensy points the eye / at whatever the camera found. |
| 7 | 20.60 | 24.00 | The blinking and the pupil / are its own idea. |
| 8 | 24.20 | 27.40 | About $64 of parts. / A second eye is under $12. |
| 9 | 27.60 | 31.00 | None of this was the point. |
| 10 | 31.20 | 34.60 | The sensor my robot face used / got discontinued. |
| 11 | 34.80 | 38.50 | That face is called IRIS. / It is running this right now. |

Why it is built this way:

- **Cue 1 is C's opener** because it frames a problem the viewer did not know existed, which beats
  stating a spec. Cue 2 immediately cashes it against the thing this audience has already seen.
- **Cue 4 gets a beat to itself.** "No Pi. No PC. No wifi." is the single most impressive claim to
  a maker and it needs no explanation, so it is not buried in a sentence.
- **Cue 9 is the pivot.** Four words, and it turns a gadget demo into a story. Cues 10 and 11 then
  land the IRIS reveal back to back so it reads as a payoff, not a credit.
- **Cue 8 carries both prices** because "a second eye is under $12" is the line that makes a viewer
  imagine building two, and generic 1.28 inch GC9A01A modules really are about $8.60
  ([BOM.md](../BOM.md)).

## Alternate cuts, E through H

Four structurally different sets, not rewordings of D. All validated against the clip: none run
past 38.50 s, none overlap, none exceed two lines, none exceed 20 characters per second.

### E, reveal first (`CyclopsGaze_captions_E_reveal.srt`), 11 cues, 13.0 char/s

Inverts D. Opens on the discontinued sensor and makes the eye the answer rather than the payoff.
Cue 3 is a three-word beat, "This is that test", which hands the clip back to the visuals.

**Trade:** the strongest thing in the video, that it is watching a person, is not stated until cue
4. On a platform where the first two seconds decide everything, that is a real cost. Best suited to
YouTube, where a viewer has already chosen to watch.

### F, numbers forward (`CyclopsGaze_captions_F_numbers.srt`), 11 cues, 11.6 char/s

For the build-curious. Leads with a parts count, then costs, resolution and frame rate.

Cue 4 is newly possible: **"240 by 240 pixels of eye, redrawn about 20 times a second."** The frame
rate was unmeasurable until CG-S17 executed BENCH_PROTOCOL step 11, so no earlier draft could
honestly say it. 19-22 FPS measured, so "about 20" is fair.

**Trade:** specifications persuade people who already care and bounce people who do not. It never
makes an emotional claim.

### G, sparse (`CyclopsGaze_captions_G_sparse.srt`), 5 cues, 4.8 char/s

Five cues with long holds, closest to a title card treatment. Lets the orbit and the gaze do the
work, since the footage is genuinely good enough to carry it.

**Trade:** it leaves the whole story on the floor. No mechanism, no camera explanation, no IRIS
context beyond one closing line. Strongest as an Instagram or Bluesky post where a written caption
sits beside the video and carries the detail.

### H, vertical first (`CyclopsGaze_captions_H_vertical.srt`), 13 cues, 8.6 char/s

Written for a 9:16 crop: 13 short cues, most lines under 25 characters, so nothing wraps badly in a
narrow safe area. Faster cut, more beats, each one small. The eye stays in the upper half through
the whole orbit, so a centre crop keeps the subject.

**Trade:** on a 16:9 timeline it feels choppy and under-written. This is a crop-specific cut, not a
general improvement.

### Which is weakest

**G.** It is the most tempting because it looks confident and takes the least effort to read, but a
39 second clip with five captions asks the footage to answer questions it cannot: why the eye moves,
what is under it, why any of it exists. It only works with supporting text beside it, which means it
is not really a captioned video, it is a video with a post attached.

**D remains the recommendation for a single general-purpose cut.** E is the one worth testing against
it, since the two differ in structure rather than wording and would give a clean read on whether
problem-first or trick-first performs better with this audience.

## Earlier drafts, kept for the record

**Draft A, build-log voice**, 11 cues, cold open on "The eye is following my face", spec beats
through the middle, IRIS last. Accurate and tight but **too technical to be engaging**: it stated
what the thing was before giving anyone a reason to care, and used words like UART and Modbus that
mean nothing to most of the audience.

**Draft B, retention cut**, 14 cues at ~2.5 s, second person, "Walk around it / It keeps looking at
you". Good hook in the abstract, but the operator is the one walking, not the viewer, so the line
invites something the clip does not let you do.

**Draft C, documentary**, 8 cues with long holds, explaining why the project exists. **The most
engaging voice of the three** and the source of Draft D's spine, but too slow for short form, and
its wording was the worst offender on jargon ("Face center out over UART. Gaze target in.").

Draft D takes C's cues 1, 2 and 7 nearly intact, A's cue density, and rewrites every technical
phrase into plain language.

## Wording rules applied

- Nothing claims a behaviour the footage does not contain.
- "Two wires" instead of UART, "a tiny AI camera" instead of the part number, "points the eye"
  instead of gaze-target mapping. The part numbers live in the description, where someone who wants
  to build it will look.
- The only status claim is about IRIS, and it is true: the driver is deployed there and tracking.
  The repo's REPO-ONLY / DEPLOYED / VERIFIED vocabulary never appears; it means nothing to viewers.
- **$64 is the build in the video** (Teensy 4.1 + one display + camera, [BOM.md](../BOM.md) puts it
  at $63.90). **Not $47**, which is the unbuilt ESP32-S3 route.

---

## Post copy

### YouTube title options

1. This animatronic eye actually follows your face
2. I gave an animatronic eye a camera so it looks at you
3. Face-tracking animatronic eye: Teensy 4.1 and a $15 AI camera

### YouTube description, long version (the "about this video" text)

    Most animatronic eyes wander on a timer. This one has a camera under it, so it
    looks at whoever is actually in the room.

    Everything happens on the device. A small AI camera does the face detection
    itself and sends the position over two wires to a Teensy 4.1, which points a
    1.28 inch round LCD at whatever it found. No Raspberry Pi. No PC. No wifi.
    Nothing phones home, and nothing breaks when your internet does.

    The blinking, the pupil dilation and the small idle drifts are the eye's own
    business. Only the direction it looks comes from the camera. That split is why
    it reads as alive without much cleverness behind it: the gaze is honest, and
    everything else is just good animation running at about 20 frames a second.

    Around $64 of parts for what you see here. Extra round displays run under $12,
    so the second eye is the cheap one. The firmware carries ten different eye
    designs and you pick one by uncommenting a line. Tracking range, sensitivity
    and how far the eye travels all tune over a serial cable while it is running,
    with no reflashing.

    Now the part that is not in the video.

    I did not build this because I wanted an eye watching me while I work. I built
    it because a part went out of production.

    My robot face, IRIS, uses a small face sensor to drive its gaze. That sensor
    was discontinued. A project meant to be rebuilt by other people cannot depend
    on something nobody can buy, so I needed a replacement that spoke the old
    part's language exactly, or every piece of code that talked to it would have
    to be rewritten.

    That is what this is. Same data structure, same function calls, same behaviour.
    Drop it in and the robot does not notice it was replaced. It has been running
    in IRIS ever since.

    IRIS, the robot face this was built for:
    https://github.com/Maestro8484/IRIS

    CyclopsGaze, this project. Firmware, wiring diagrams, a sourced parts list and
    the full build log, including everything that went wrong:
    https://github.com/Maestro8484/CyclopsGaze

    Hardware: Teensy 4.1, a 1.28 inch 240x240 GC9A01A round LCD, and a DFRobot
    SEN0626 vision camera doing the face detection on its own processor.

    The eye rendering engine is TeensyEyes by Chris Miller, MIT licensed, which
    descends from Adafruit's Uncanny Eyes. The iris and sclera artwork in this
    particular eye is mine.

### YouTube description, short version

    An animatronic eye that tracks a real face instead of wandering on a timer.

    A DFRobot SEN0626 AI vision camera does the face detection on the camera itself and
    sends the face position to a Teensy 4.1 over two wires. The Teensy points a 1.28 inch
    240x240 round LCD eye at whatever it found. No Raspberry Pi, no PC, no network anywhere
    in the loop. The blinking, pupil dilation and idle drift are autonomous; only the gaze
    direction comes from the camera. Tracking range and the confidence gate tune live over
    serial with no reflash.

    About $64 in parts for the build in this video. Extra displays are under $12 each, so a
    second eye is cheap.

    It exists for a reason that is not visible in the clip. The tiny Useful Sensors Person
    Sensor that drove the gaze on my robot face, IRIS, was discontinued, and a project meant
    to be publicly replicable cannot depend on a part nobody can buy. This driver presents
    the exact same struct and method surface the old sensor did, so IRIS consumed it with
    almost no changes. It is running in IRIS right now.

    CyclopsGaze, firmware and wiring and a sourced parts list:
    https://github.com/Maestro8484/CyclopsGaze

    IRIS, the robot face it was built for:
    https://github.com/Maestro8484/IRIS

    The eye rendering engine is TeensyEyes by Chris Miller (MIT), itself adapted from
    Adafruit's Uncanny Eyes. The iris and sclera artwork in this eye is mine.

### Short social caption (Instagram, TikTok, Mastodon, Bluesky)

    Most animatronic eyes wander on a timer. This one has an AI camera under it, so it looks
    at whoever is actually there. Teensy 4.1, one round LCD, no Pi and no PC in the loop,
    about $64 of parts and under $12 for a second eye. Built as the replacement gaze sensor
    for my robot face IRIS after the original part was discontinued. Build notes on GitHub.

### Tags

    animatronics, animatronic eye, face tracking, teensy, teensy 4.1, platformio,
    uncanny eyes, teensyeyes, gc9a01a, dfrobot, sen0626, halloween prop, cosplay,
    puppet, robot face, diy electronics, embedded, arduino

---

## Publishing

### Files ready in `C:\Users\SuperMaster\Videos\CyclopsGaze\`

| File | Size | Purpose |
|---|---|---|
| `CyclopsGaze_captions_v3.srt` | 1 KB | Import into Resolve, or upload to YouTube as a caption track |
| `CyclopsGaze_reframed_stabilized_1080p_captioned_1080p.mp4` | 24.1 MB | Captions burned in bottom centre, audio kept. For social, where muted autoplay needs baked captions |
| `CyclopsGaze_reframed_stabilized_1080p_captioned_web.mp4` | 2.3 MB | 1280 wide, no audio. Small enough to drop straight into GitHub |
| `CyclopsGaze_captions_v3.burn.ass` | 2 KB | Generated style file, kept for inspection |

Produced by `C:\Users\SuperMaster\.claude\demo-video-kit\burn.py`, which is the reusable version of
this whole job (driven by `/demo-video`). **Captions sit bottom centre**, 48 px off the bottom edge,
44 px Arial bold with a 4 px black outline. That reads over both the white desk and the dark
background, and stays clear of the eye, which sits high in frame throughout the orbit.

⚠ **Three `ffmpeg` traps, each of which cost a render, all now encoded in the kit:**

1. **Pixel values handed to `force_style` are wrong by ~3.75x.** ffmpeg's SRT to ASS conversion
   writes `PlayResX: 384 / PlayResY: 288` (confirmed by converting and reading the file), and libass
   scales every script unit by `video_height / PlayResY`. So `MarginV=55` rendered ~206 px off the
   bottom, which is the "still a quarter up from the bottom" the operator spotted, and `FontSize=24`
   rendered ~90 px tall. The kit converts to ASS, rewrites `PlayRes` to 1920x1080, and writes the
   style line itself, so its numbers are literal pixels.
2. **`BorderStyle=4` (opaque caption box) breaks positioning outright.** With it set, `Alignment`
   and `MarginV` are both ignored and the block lands mid-frame-left. Verified by rendering variants
   and comparing frames. Use `BorderStyle=1` with an outline.
3. **An absolute Windows path cannot go inside a filtergraph value.** ffmpeg splits filter options
   on `:` and reads the drive-letter colon as a separator, failing with a misleading
   `Unable to parse option value ... as image size`. Escaping does not survive a non-shell argv, so
   run ffmpeg with `cwd` set to the subtitle's folder and pass a bare filename.

Also: when spot-checking a frame, put `-ss` **after** `-i`. Input seeking rebases timestamps to
zero, so the subtitle filter finds no active cue and renders a caption-free frame, which looks
exactly like a styling failure and sends you chasing the wrong thing.

### Recommended route

**Host the video on YouTube and reference it everywhere else.** Neither GitHub nor Hackaday is a
video host, and a single canonical URL means one place to fix if the edit changes.

1. **YouTube** is the canonical copy. Upload the Resolve export, attach
   `CyclopsGaze_captions_v3.srt` as the caption track rather than relying on auto-captions, and use
   the description above.

### Resolve: import the SRT, do not rebuild the project

**Import the subtitle track into the existing project.** A Resolve project is not a file that can
be authored externally: it lives in Resolve's own project database (nothing in
`C:\Users\SuperMaster\Videos\CyclopsGaze\` but media and SRTs, checked), and the only portable form
is a `.drp` export, which is a proprietary format. So there is no "hand you a finished project
file" option.

Route: `File > Import > Subtitle`, pick `CyclopsGaze_captions_v3.srt`. It lands as its own subtitle
track, fully editable, and Resolve's default subtitle position is already bottom centre. Style once
on the track in the Inspector and every cue follows.

The alternative, if this ever needs automating: **Resolve's scripting API is installed on this
machine** (`DaVinciResolveScript.py` under
`C:\ProgramData\Blackmagic Design\DaVinci Resolve\Support\Developer\Scripting\Modules\`, Resolve
21.0.2.4). A Python script can create a timeline, import the clip and attach subtitles, but it
needs Resolve running with Preferences > System > General > External scripting set to Local. That is
strictly more moving parts than three clicks, so it is only worth it for a repeatable pipeline
across many clips.
2. **GitHub README** gets a thumbnail image linking to the YouTube URL, placed just under the hero
   photo. GitHub does render an uploaded MP4 if you drag it into an issue or release and use the
   resulting attachment URL, and the 2.4 MB web file exists for that, but a YouTube link survives
   repo moves and gives view counts.
3. **Hackaday.io project page is worth doing** and is the best audience match: props, cosplay and
   animatronics people are already there, it embeds YouTube directly, and its build-log format maps
   onto [CHANGELOG.md](../../CHANGELOG.md) almost one to one. Lead with the video, then the BOM,
   then the wiring gotchas (DIP switch on UART, check the sensor's own VCC), then the story of why
   a discontinued part started the whole thing.

### One correction to make first

[README.md](../../README.md) § Relation to IRIS calls IRIS "a private tabletop robot-face project".
That is now half wrong: `Maestro8484/IRIS-Robot-Face` (the firmware working repo) is private, but
**`Maestro8484/IRIS` is public** and is the project's public home. Any copy that names IRIS should
link the public one.
