# Miss-Click Manager

A Geode mod for Geometry Dash that counts button presses on the player during levels.

**Status:** Early version. It counts every press and logs the running total. The rules for what counts as a miss-click are not finished yet.

## Requirements

- Geometry Dash 2.2081
- Geode v5.11.2

## Installing

1. Open the **Actions** tab of this repo and click the latest successful run.
2. Download the **Build Output** artifact and extract the `.geode` file from the zip.
3. Copy the `.geode` file into your Geode mods folder. On Windows this is `%LOCALAPPDATA%\GeometryDash\geode\mods\`.
4. Restart Geometry Dash.

## Building

The workflow in `.github/workflows/build.yml` builds the mod for Windows, macOS, and Android and packages the results into one `.geode` file. It runs on every push and can also be started by hand from the Actions tab.

## Project layout

- `mod.json`: mod name, ID, version, and the Geode and Geometry Dash versions it targets
- `CMakeLists.txt`: build configuration
- `src/main.cpp`: mod code
- `.github/workflows/build.yml`: automatic build

## TODO

- Decide what counts as a miss-click
