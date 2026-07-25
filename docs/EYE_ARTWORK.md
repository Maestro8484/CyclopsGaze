# Where eye artwork comes from

Answer to "are there custom TeensyEyes eye sets beyond Chris Miller's repo?", researched
2026-07-25. Short version: **there is no third-party ecosystem of TeensyEyes eyes.** Nobody
has published a community pack, and the one public fork adds nothing. The supply of artwork is
three things: upstream's 23, six unported designs still sitting in Adafruit's older repos, and
anything you generate yourself. `nordicBlue` came from that third route.

Every listing below was read from the source repo (GitHub API or the local clone) on that date,
not recalled.

## What exists today

| Source | Count | Status here |
|---|---|---|
| `chrismiller/TeensyEyes`, `src/eyes/240x240` | **23** | 10 bundled in this repo, 13 a file-copy away. See [ATTRIBUTION.md](ATTRIBUTION.md). |
| `adafruit/Uncanny_Eyes`, `convert/` | 9 | 4 already ported upstream (`cat`, `doe`, `dragon`, `newt`). **5 never ported.** |
| `adafruit/Adafruit_Learning_System_Guides`, `M4_Eyes/eyes` | 15 | 13 correspond to upstream eyes, 1 is a 128x128 size variant of `hazel`. **1 never ported** (`reflection`). |
| `adafruit/Pi_Eyes`, `graphics/` | textures, not eye configs | Reusable source art, no config to port. |
| `MichaelMeissner/TeensyEyes` (only public fork found) | 23 | **Identical eye list to upstream**, confirmed by API. Adds no artwork. |
| `PaintYourDragon/Teensy-Iris` | ancestor project | Superseded by the Adafruit guide, its README says so. No artwork of its own. |

## The six designs that were never ported

These are real, MIT-licensed eye designs that no TeensyEyes build has ever carried.

From **`adafruit/Uncanny_Eyes/convert/`**, each a folder of PNGs:

| Eye | Assets | Notes |
|---|---|---|
| `terminatorEye` | `iris.png`, `sclera.png`, 4 lid PNGs | The glowing red T-800 eye. Strong prop candidate. |
| `goatEye` | same, **plus `pupilMap.png`** | Horizontal rectangular pupil. The extra `pupilMap.png` has no equivalent in TeensyEyes' schema, so this one needs thought, not just a rename. |
| `naugaEye` | `iris.png`, `sclera.png`, 2 lid PNGs | Adafruit's Nauga mascot. Only asymmetric-lid-only set of the five. |
| `noScleraEye` | same as terminator | Iris fills the whole eyeball, no white. |
| `defaultEye` | same as terminator | The original 2016 brown human eye. Mostly historical. |

From **`M4_Eyes/eyes/`**:

| Eye | Assets | Notes |
|---|---|---|
| `reflection` | `config.eye`, `iris.bmp`, `sclera.bmp` | Near-fixed pupil (`pupilMin` and `pupilMax` both 0.01), a mirrored/reflective look. |

## Porting cost, measured against the two real schemas

The blocker is not the images, it is that **`config.eye` is not one format**. Upstream's schema
and Adafruit's M4_Eyes schema share a filename and nothing else. Both read verbatim, same day:

TeensyEyes (`resources/eyes/240x240/hazel/config.eye`), nested, hex colours, lids as PNG files:

    { "name": "hazel", "radius": 125, "backColor": "0x8942",
      "pupil":  { "color": 0 },
      "iris":   { "texture": "iris.png", "radius": 60 },
      "sclera": { "texture": "sclera.png" },
      "eyelid": { "color": 0, "upperFilename": "upper.png", "lowerFilename": "lower.png" } }

M4_Eyes (`M4_Eyes/eyes/reflection/config.eye`), flat, RGB triples, lids as an index into a
stock table, and JSON with `//` comments in it:

    { "eyeRadius": 120, "eyelidIndex": "0x00", "pupilColor": [0,0,0],
      "backColor": [140,40,20], "irisTexture": "reflection/iris.bmp",
      "scleraTexture": "reflection/sclera.bmp", "pupilMin": 0.01, "pupilMax": 0.01,
      "scleraAngle": 0.0, "irisAngle": 0.0, "left": {}, "right": {} }

So the work per source:

- **From `Uncanny_Eyes`**: easiest. Assets are already PNG at the right sort of scale. Rename
  `lid-upper.png` to `upper.png` and `lid-lower.png` to `lower.png`, author a `config.eye` in
  upstream's schema, run the generator. `goatEye` additionally needs its `pupilMap.png` dealt
  with or dropped.
- **From `M4_Eyes`**: rewrite the config into upstream's schema, convert BMP to PNG, and supply
  eyelid PNGs, because `eyelidIndex` points into a table of stock lid shapes that upstream's
  generator does not consume.
- **From `Pi_Eyes`**: it ships `cyclops-eye.svg`, `dragon-eye.svg`, `dragon-iris.jpg`,
  `dragon-sclera.png`, `eye.svg`, `iris.jpg`, `sclera.png`, `lid.png`, `uv.png`. No config to
  port, so this is a texture source, not an eye port.

## The generator, which is the actual answer

Upstream ships the whole pipeline in `resources/`, and it is on this disk at
`../TeensyEyes/resources/eyes/240x240/`: one folder per eye holding `config.eye`, `iris.png`,
`sclera.png`, `upper.png`, `lower.png` and the two `*-symmetrical.png` variants, plus
`tablegen.py`, `genall.py`, `config.py` and `hextable.py`. It needs Python with `numpy` and
`Pillow`, and it emits the `<name>.h` table headers that `src/eyes/240x240/` holds.

That is how **`nordicBlue`** was made, and it is why the interesting answer to "what other eye
sets exist" is "however many you want". The generator's inputs are an iris image and a sclera
image. Any iris texture becomes an eye: a photograph of a real iris, a painted texture, a
procedural render, or one of the many iris texture packs sold for 3D and game work.

Licensing, since this repo is public and MIT: everything in Adafruit's repos and upstream
TeensyEyes is MIT and safe to bundle with attribution. Commercial texture packs are generally
**not** redistributable even when they are cheap, so art from one can drive a private build but
must not be committed here. Keep [ATTRIBUTION.md](ATTRIBUTION.md) accurate for anything added.

## Sources

- <https://github.com/chrismiller/TeensyEyes>
- <https://github.com/adafruit/Uncanny_Eyes>
- <https://github.com/adafruit/Pi_Eyes>
- <https://github.com/adafruit/Adafruit_Learning_System_Guides/tree/main/M4_Eyes>
- <https://learn.adafruit.com/customeyesation-diy-monster-m4sk-graphics>
- <https://learn.adafruit.com/adafruit-monster-m4sk-eyes/customization-basics>
