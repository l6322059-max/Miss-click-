# Miss-Click Manager

A Geode mod for Geometry Dash that ignores accidental extra clicks. Whenever it drops a click, a small counter in the game's own font shows how many it has ignored, then fades away by itself.

## What counts as a miss-click

The mod can't tell a badly timed click from a good one, so it filters accidental extra taps:

- A click that comes very soon after your previous click is ignored. The default is 50 ms.
- Optionally, clicks right after you respawn are ignored too. This is off by default.

Ignored clicks never reach the game. Ignoring works on Windows and Android. On macOS the mod builds but does not ignore anything.

## Settings

Open Geode's mods list, pick Miss-Click Manager, and tap the settings button.

- **Enabled**: turn the click filter on or off.
- **Click Gap (ms)**: ignore a click that comes this soon after your previous click. 0 turns this off.
- **Respawn Grace (ms)**: ignore clicks for this long after you respawn. 0 turns this off.
- **Show Counter**: show the Ignored Clicks counter when a click is ignored.
- **Fade Delay (seconds)**: how long the counter stays on screen before it fades away.
- **Always Show**: keep the counter on screen instead of fading it.
- **Reset On Restart**: clear the counter and hide it every time the attempt restarts.
- **Position**: which corner of the screen the counter appears in.
- **Size** and **Opacity (%)**: how big and how see-through the counter is.

## Requirements

- Geometry Dash 2.2081
- Geode v5.11.2

## Installing

1. Open the **Actions** tab of this repo and click the latest successful run.
2. Download the **Build Output** artifact and extract the .geode file from the zip.
3. Put the .geode file in your Geode mods folder (the Geode docs at docs.geode-sdk.org explain where that is on your device) and restart Geometry Dash.

## Building

The workflow in .github/workflows/build.yml builds the mod for Windows, macOS, and Android and packages the results into one .geode file. It runs on every push and can also be started by hand from the Actions tab.

## Project layout

- mod.json: mod name, ID, version, settings, and the Geode and Geometry Dash versions it targets
- CMakeLists.txt: build configuration
- src/main.cpp: mod code
- .github/workflows/build.yml: automatic build
