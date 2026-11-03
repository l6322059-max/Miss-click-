# Miss-Click Manager

A Geode mod for Geometry Dash that filters accidental extra clicks. When it ignores a click, a small counter in the game's own font shows how many clicks were ignored, then fades away.

## What counts as a miss-click

The mod cannot tell a badly timed click from a good one, so it filters accidental extra taps:

- A click that comes very soon after your previous accepted click is ignored. The default gap is 50 ms.
- Optionally, clicks shortly after an attempt restart are ignored too. This is off by default.

Ignored clicks never reach the game. Filtering works on Windows and Android. On macOS the mod builds, but the click filter is not active.

## Settings

Open Geode's mods list, select Miss-Click Manager, and open its settings.

- **Enabled**: turn the click filter on or off.
- **Click Gap (ms)**: ignore a click that comes this soon after your previous accepted click. Set to 0 to disable.
- **Respawn Grace (ms)**: ignore clicks for this long after an attempt restart. Set to 0 to disable.
- **Show Counter**: show the Ignored Clicks counter when a click is ignored.
- **Fade Delay (seconds)**: how long the counter stays on screen before fading.
- **Always Show**: keep the counter visible instead of fading.
- **Reset On Restart**: clear the count and hide the counter when an attempt restarts.
- **Position**: choose the counter's screen corner.
- **Size** and **Opacity (%)**: adjust the counter's appearance.

## Requirements

- Geometry Dash 2.2081
- Geode 5.11.2

## Installing

1. Open the **Actions** tab of this repository and select the latest successful run.
2. Download the **Build Output** artifact and extract the .geode file from the ZIP.
3. Install the .geode file through Geode's mod installer, then restart Geometry Dash.

## Building

The workflow in .github/workflows/build.yml builds Windows, macOS, Android 32-bit, and Android 64-bit targets. It packages the build output as a GitHub Actions artifact and runs on each push or manually from the Actions tab.

## Project layout

- mod.json: mod metadata, settings, and supported game/Geode versions
- CMakeLists.txt: build configuration
- src/main.cpp: click-filter and counter implementation
- .github/workflows/build.yml: automated build
