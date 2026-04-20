# asciiVideo

Render a video file as an ASCII-art video. Drag a video onto the window; the app plays it back through an ASCII rendering pass and writes a matching `.mp4` next to the source. Built with [openFrameworks](https://openframeworks.cc/).

Licensed under GPLv3. See [LICENSE](LICENSE).

## Requirements

- macOS on Apple Silicon (arm64). Intel Macs are not supported by the current build.
- macOS 11 or later.

## Running the app

1. Launch `asciiVideo.app`.
2. **First-run folder picker.** A macOS folder dialog appears asking where you want asciiVideo to keep its config file. Pick any folder you can write to (e.g. `~/Documents/asciiVideo/`). The app seeds a default `asciiVideo-settings.json` into that folder and remembers your choice across launches.
3. **Drag a video file** onto the window. The window resizes to match the video's resolution, the ASCII rendering plays in real time, and the output file is written next to the input as `<name>_ascii.mp4`.
4. Press `q` during processing to stop early.

### The GUI panel

The panel at the top-left holds live controls:

- **paletteIndex** — integer slider; pick which palette to render with.
- **palette** — read-only name of the currently-selected palette.
- **pick font…** — opens a file dialog for a `.ttf` / `.otf` font file. The chosen font is used immediately for new frames.
- **font** — read-only name of the current font file.
- **fontSize** — glyph size in points.
- **cellWidth** / **cellHeight** — size (in source pixels) of each ASCII cell. The app averages the source pixels inside each cell to choose the character, so larger cells = coarser grid, smaller cells = denser grid and slower render.
- **sample palette from video…** — opens a file dialog; clusters the colours of the chosen video into a new palette built from the *current* palette's character set and background. See [Sampling a palette from a video](#sampling-a-palette-from-a-video) below.

All changes persist across runs (they're written back to `asciiVideo-settings.json` on exit).

## Configuration file: `asciiVideo-settings.json`

The app reads and writes a single JSON file that lives in the folder you picked on first run. It has two top-level keys:

### `settings`

Live GUI state; last-known values for every control. Edit by hand if you like, but the app overwrites this block on exit.

```json
"settings": {
  "fontPath": "Courier New Bold.ttf",
  "fontSize": 9,
  "cellWidth": 7,
  "cellHeight": 9,
  "startPalette": "classic"
}
```

- `fontPath`: bare filename (looked up inside the bundle's `data/` folder) or an absolute path to any font on your system.
- `startPalette`: name of the palette to select at startup. Falls back to the first palette if the name isn't found.

### `palettes`

An array of palette definitions. The app never rewrites this block on exit, so your edits are safe. Each palette has:

```json
{
  "name": "matrix",
  "backgroundColor": [0, 0, 0],
  "characters": [
    { "char": " ", "color": [0, 50, 0] },
    { "char": ".", "color": [0, 100, 0] },
    ...
    { "char": "#", "color": [220, 255, 220] }
  ]
}
```

- `name`: appears in the GUI and is referenced by `startPalette`.
- `backgroundColor`: RGB triplet `[0-255, 0-255, 0-255]`. Fills the canvas every frame. Defaults to black if omitted.
- `characters`: ordered **dark → light**. Each entry binds a character to a foreground colour. The number of entries doesn't have to be fixed — use however many gradations you want.

### Palette authoring tips

- The app maps the luminance of each source-image cell (with a `^2.5` gamma curve) to an entry index. So put your "darkest" character first, "lightest" last.
- For a **dark-background** palette, use space (` `) as the first entry and progressively denser glyphs (`@`, `#`) at the light end. Sparse characters let the background show through, which reads as dark; dense coloured characters read as light.
- For a **light-background** palette (see `seaside`), flip the density: dense dark-ink characters at index 0, sparse/space at the end. The background shows through in light regions.
- `backgroundColor` and the foreground colours don't have to be related — wild combinations work.

## Sampling a palette from a video

Use this to build a palette whose colours come from a specific video's actual content:

1. In the GUI, pick a *base palette* (its character set and background are kept).
2. Click **sample palette from video…**.
3. Pick any video file. The app:
   - Samples up to 60 evenly-spaced frames.
   - Reads one pixel in every 8 across those frames.
   - Runs k-means clustering to find `N` dominant colours (where `N` is the character count of your base palette).
   - Sorts the colours dark → light and pairs them with the base palette's characters.
4. The new palette is appended to `asciiVideo-settings.json` and selected automatically, named `<base> + <video-stem>`.
5. Drag the same (or any) video onto the window to render with the new palette.

Sampling **seeks across the whole video** — it jumps to 60 evenly-spaced target frames via `setFrame`, waits for each seek to decode, and samples pixels from that frame. The UI stays responsive: an on-screen progress bar shows `frames attempted / total`, and you can press **Esc** to cancel at any time. Typical runtime is a handful of seconds regardless of video length. Hard 30-second cap as a safety net; if a seek doesn't decode within 1 second it's skipped. Sampled palettes behave exactly like hand-authored ones and can be edited in the JSON.

## Output file

Rendered output is written next to the source video as `<name>_ascii.mp4`, encoded via the bundled ffmpeg binary using the source video's framerate. Existing files at that path are overwritten.

Encoding settings (baked in, tuned for quality):

- **H.264 (libx264), CRF 18** — visually near-lossless. File sizes are larger than a typical streaming encode in exchange for cleaner output.
- **yuv420p pixel format** for broad player compatibility (QuickTime, Safari, iOS). Sources with odd width or height are automatically trimmed to the nearest even pixel via an ffmpeg scale filter (required by yuv420p).
- **4× MSAA** on the render FBO for anti-aliased glyph edges.
- **Box-filtered cell sampling** — each ASCII cell's character is chosen from the average of all source pixels inside it, not a single pixel, which reduces shimmer on noisy or fine-textured footage.

## File locations

- **Config folder** (you chose it on first run): `asciiVideo-settings.json`
- **Config-folder pointer**: `~/Library/Application Support/asciiVideo/config-path.txt` — delete this file to force the first-run folder picker to appear again.
- **Bundled resources** (read-only, inside the `.app`): default `asciiVideo-settings.json`, `Courier New Bold.ttf`, `ffmpeg` binary.
- **Output videos**: next to their source file.

## Building from source

From `apps/myApps/asciiVideo/`:

```sh
make                  # build Release
make RunRelease       # build and launch
make clean            # clean build artefacts
```

The build process:

1. Compiles the app and its addons (`ofxFFmpegRecorder`, `ofxGui`) and links against a statically-built openFrameworks core.
2. Assembles `bin/asciiVideo.app`.
3. `PROJECT_AFTER` (in [config.make](config.make)) copies `bin/data/` into `Contents/Resources/` and ad-hoc signs the bundled `ffmpeg`, producing a self-contained bundle.

## Distributing the app

The `.app` bundle is self-contained once built. Known limitations:

- **arm64 only.** Won't run on Intel Macs.
- **Not notarised.** Recipients will see a Gatekeeper warning on first launch; they need to right-click → Open (or allow via System Settings → Privacy & Security). Proper distribution requires signing with an Apple Developer ID and notarising the bundle.
- **GPLv3.** Any binary distribution must travel with the `LICENSE` file.

## Acknowledgements

- ASCII rendering algorithm adapted from openFrameworks' `asciiVideoExample`, originally based on Ben Fry's Processing `ASCII Video` example.
- Bundled static ffmpeg from [osxexperts.net](https://www.osxexperts.net/) (GPL build, arm64).
