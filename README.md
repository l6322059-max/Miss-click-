# Miss-Click Manager

A Geode mod for Geometry Dash that fixes miss-clicks. It ignores accidental extra taps and saves taps that come a little too early. A small counter in the game's own font shows how many clicks it has fixed, then fades away by itself.

## What it fixes

- **Accidental extra taps**: a click that comes very soon after your previous click is ignored. The default is 50 ms.
- **Taps that are too early**: if you tap up to 100 ms before you land, the tap is saved and fires the moment you land. It is meant for cube, ball, robot, and spider modes. It skips ship, UFO, wave, and swing, and it skips taps made on a ring.
- **Clicks right after you respawn** can be ignored too. This is off by default.

Taps that are too late can't be fixed, because by the time they arrive the game has already moved on.

This is an input assist. It changes what the game receives, so don't use it for runs you plan to submit as legit records. It works on Windows and Android. On macOS the mod builds but does not fix anything.

## Settings

Open Geode's mods list, pick Miss-Click Manager, and tap the settings button.

- **Enabled**: turn the click fixing on or off.
- **Early Tap Buffer (ms)**: how soon before landing a tap still counts. 0 turns this off.
- **Click Gap (ms)**: ignore a click that comes this soon after your previous click. 0 turns this off.
- **Respawn Grace (ms)**: ignore clicks for this long after you respawn. 0 turns this off.
- **Show Counter**: show the Fixed Clicks counter when a click is fixed.
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
2. Download the **Build Output** artifact and extract the `.geode` file from the zip.
3. Put the `.geode` file in your Geode mods folder (the Geode docs at docs.geode-sdk.org explain where that is on your device) and restart Geometry Dash.

## Building

The workflow in `.github/workflows/build.yml` builds the mod for Windows, macOS, and Android and packages the results into one `.geode` file. It runs on every push and can also be started by hand from the Actions tab.

## Project layout

- `mod.json`: mod name, ID, version, settings, and the Geode and Geometry Dash versions it targets
- `CMakeLists.txt`: build configuration
- `logo.png`: the mod's icon in Geode's mods list
- `src/main.cpp`: mod code
- `.github/workflows/build.yml`: automatic build
