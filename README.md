# Miss-Click Manager

A Geode mod for Geometry Dash that shows a small counter when you die right after a click, so you can see how many miss-clicks you have. The counter uses the game's own font and fades away by itself.

## What counts as a miss-click

A click counts as a miss-click if you die within a short time after it. The default is 300 ms and you can change it in the settings. In ship and wave sections you click almost constantly, so most deaths there will count.

## Settings

Open Geode's mods list, pick Miss-Click Manager, and tap the settings button.

- **Enabled**: turn the counter on or off.
- **Miss Window (ms)**: how soon after a click a death counts as a miss-click.
- **Fade Delay (seconds)**: how long the counter stays on screen before it fades away.
- **Always Show**: keep the counter on screen instead of fading it.
- **Reset On Restart**: clear the count and hide the counter every time the attempt restarts.
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
- `src/main.cpp`: mod code
- `.github/workflows/build.yml`: automatic build
