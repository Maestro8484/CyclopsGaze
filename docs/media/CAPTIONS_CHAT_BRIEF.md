# CyclopsGaze demo captions: brief for a fresh Claude Chat

**Purpose:** a self-contained brief to paste into a new Claude Chat that is doing **wording only**.
No firmware, no video tooling, no git. Every fact needed is inline, because a chat session may have
no access to this repo.

---

## Before you paste it (operator, 30 seconds)

This is a record of one moment. Confirm nothing has landed since it was written:

```
git log -1 --oneline                                     # where the repo is now
git log -1 --oneline -- docs/media/CAPTIONS_CHAT_BRIEF.md # when this brief was last true
```

Same hash means it is current. Different means re-check the hardware and cost lines below before
pasting, because those are the ones that rot. The chat itself cannot run this, which is why it sits
here rather than inside the paste block.

---

## Everything below this line is the paste block

---

I need new caption drafts for a 39 second demo video. **Wording only.** Do not write code, do not
suggest video editing, do not ask about the build process. I want words.

### What the thing is

An animatronic eye that watches your face. A round 1.28 inch LCD renders a detailed animated eye,
and a small AI camera mounted underneath it detects a face and tells the microcontroller where that
face is, so the eye turns to look at it. Everything runs on the device. No Raspberry Pi, no PC, no
network connection, no cloud service.

It exists because of a supply problem. The tiny face-detection sensor that drove the gaze on my
robot face project, **IRIS**, was discontinued. A project meant to be publicly replicable cannot
depend on a part nobody can buy, so I built and validated a replacement. That replacement is now
running in IRIS.

### What the video actually shows

39 seconds, handheld, orbiting the rig on a desk. This matters: **do not write a caption describing
something not on screen.**

On screen: one round eye display with a blue iris, mounted on a purple translucent laser-cut stand,
with the camera module directly beneath it and a microcontroller on a white breadboard beside it.
Loose coloured jumper wires. The camera circles the rig for the whole clip and **the eye keeps
turning to stay on me** as the viewpoint moves. It blinks on its own and the pupil visibly changes
size.

Never on screen: my face, any text or serial output, a second eye, a different eye design, or the
eye losing track and going idle. It is one continuous shot of one eye tracking.

### Facts I can state, all verified

- Roughly **$64** in parts for the build shown.
- Extra displays are **under $12** each, so a second eye is cheap.
- The face detection runs **on the camera itself**. No Pi, no PC, no wifi, no cloud.
- The camera sends the face position to the microcontroller over **two wires**.
- Blinking, pupil dilation and idle drift are **autonomous**. Only the direction of gaze comes from
  the camera.
- Tracking behaviour can be tuned live over a serial connection with no reflashing.
- The driver **is running in IRIS right now**, not "will be" or "could be".
- IRIS is public at `https://github.com/Maestro8484/IRIS`.
- CyclopsGaze is public at `https://github.com/Maestro8484/CyclopsGaze`.
- The eye rendering engine is **TeensyEyes by Chris Miller** (MIT licensed), in the Adafruit
  Uncanny Eyes lineage. Credit it in any long description. The iris and sclera artwork on screen is
  my own.

### Things I must not say

- Anything about the eye changing designs or cycling looks. The firmware can do it; **this clip
  does not show it.**
- Anything implying you can watch it lose the face and start wandering. It does that, but not on
  camera.
- The figure **$47**. That is a cheaper variant I have never built.
- Project status jargon. Words like REPO-ONLY, DEPLOYED and VERIFIED are internal.

### Voice

Hard rules, non-negotiable:

- **No em dashes.** Commas, periods, colons. Normal hyphens are fine.
- **No emojis.**
- **No AI filler.** Nothing like "dive into", "unlock", "game changer", "in today's world", "let's
  explore". No rhetorical question openers.
- Plain words over correct-but-opaque ones. A viewer who does not do embedded work must understand
  every line. "The camera sends the face position over two wires" is right. "Face center out over
  UART" is not.

What I like, from things I have written or approved:

- **Short declarative fragments**, especially triples of negations. "No Pi. No PC. No wifi." My
  IRIS README does the same thing: "zero cloud AI dependencies, zero external speech APIs, zero
  telemetry."
- **Contrarian openings** that set up a contrast. IRIS opens: "Most home assistants are a
  microphone with a personality bolted on. IRIS is the other way around."
- **Concrete and specific over vague and impressive.** Real numbers, real part counts.
- **Dry, understated.** No hype adjectives. Let the thing be impressive by itself.

⚠ One inconsistency to be aware of: my public IRIS README uses em dashes throughout. That predates
the no-em-dash rule. Follow the rule, not that file.

**Calibrate before drafting:** if you are unsure of my voice, ask me for two or three samples of my
own writing and match those. Do not guess and do not flatter.

### Format constraints

- About **11 cues** for 39 seconds, so roughly 3 to 3.5 seconds each.
- Maximum **2 lines per cue**, short enough to read at a glance.
- These are on-screen captions for **muted autoplay**. They carry the entire story. Assume no sound.
- One idea per cue.

### What I have already tried

Four drafts. My verdicts:

- **Technical build-log voice**, opening "The eye is following my face" then listing specs. **Too
  technical, not engaging.** It said what the thing was before giving anyone a reason to care.
- **Fast retention cut**, second person, opening "Walk around it / It keeps looking at you".
  Rejected: I am the one walking, so it asks the viewer to do something the clip does not let them
  do.
- **Documentary voice**, slower, explaining why the project exists. **The most engaging of the
  three**, but too slow for short form and the jargon was worst here.
- **A merge of the last two**, which is what currently ships:

```
1  An animated eye is easy. / Making it look at you is not.
2  Most of these wander on a timer. / This one is watching me.
3  A tiny AI camera under the eye / finds my face on its own.
4  No Pi. No PC. No wifi.
5  The camera sends the face position / over two wires.
6  A Teensy points the eye / at whatever the camera found.
7  The blinking and the pupil / are its own idea.
8  About $64 of parts. / A second eye is under $12.
9  None of this was the point.
10 The sensor my robot face used / got discontinued.
11 That face is called IRIS. / It is running this right now.
```

Structural things that worked and are worth keeping or deliberately breaking: opening on a problem
instead of a spec, contrasting early against timer-driven eyes since that is what this audience has
already seen, giving the strongest claim its own cue, and a short late pivot ("None of this was the
point") that turns a gadget demo into a story with the IRIS reveal as the payoff.

### What I want from you

Three genuinely different caption sets, not three polishes of the same one. Vary the structure, not
just the adjectives. Angles I have not tried: leading with the discontinued-part problem and letting
the eye be the answer, a numbers-forward version, a very sparse version of three or four cues that
lets the visuals carry it, and a vertical-crop version where lines must be shorter.

For each set give me the cue list with rough timings, one sentence on what it is doing differently,
and be honest about which you think is weakest and why.

Audience: makers, animatronics, props and cosplay people on YouTube and short-form. They have seen
plenty of animated eyes that wander on a timer. They have not seen one that locks onto a face.
