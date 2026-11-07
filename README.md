# Drag and Drift Tires (FH3)

Two hidden tire compounds from the files of **Forza Horizon 3** (the Xbox One build on PC, through the Azimuth launcher), made buyable on **every car**, on the
**[FH3 Mod Platform](https://github.com/Levilente/FH3_ModPlatform)**.

> Version **1.0.0**. Needs the FH3 Mod Platform **1.0.0** or newer.

## What it does

The game ships a `19_Drag` and an `18_Drift` tire compound (grip curves, heat, wear) and a drag tire model (`tire_drag`, `tire_OW_drag`), but no car's
upgrade list offers them (only the Hot Wheels Bone Shaker and Twin Mill have the compounds, on a normal tire model). This mod adds them to the list:

**Garage > Upgrades > Tires and Rims > Tire Compound**

| Choice | Compound | Tire model | Price |
|---|---|---|---|
| **Drag Tire Compound** | `19_Drag` | the game's drag tire (`drag`, or `OW_drag` on open-world cars) | 10 000 CR |
| **Drift Tire Compound** | `18_Drift` | the car's own race tire model | 10 000 CR |

Both are on every car, are stored per owned car like any other tire compound, and show in the car's tire list with their own name and description.
Cars that already have the compound (the two Hot Wheels cars) are left alone.

## Install

Put `DragTires.dll` into the `fh3-mods` folder next to the game's platform files, and start the game **twice**: the first start builds the changed
database and the new texts, which the game reads at its next start.

## Settings

`fh3-mods\DragTires.ini` (written on first start), section `[DragTires]`:

| Key | Default | What |
|---|---|---|
| `Drag` | 1 | 0 leaves out the drag compound |
| `Drift` | 1 | 0 leaves out the drift compound |
| `DragPrice` | 10000 | Price of the drag compound |
| `DriftPrice` | 10000 | Price of the drift compound |
| `Log` | 0 | 1 writes diagnostic lines to `platform.log` |

## Build

Needs CMake and a MinGW-w64 (llvm-mingw) toolchain. Put the FH3 Mod Platform checkout at `./platform` (or pass `-DFH3_PLATFORM_DIR=...`) and build:

    cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=<your mingw toolchain>
    cmake --build build

## How it works

Pure data. The mod queues SQL for the platform (`add_sql`), which applies it to a copy of `gamedbRC.slt` that the game reads from `mediaoverride`:

- one `List_UpgradeTireCompound` row per car and compound (compound id, price, tire model; level 10 for drag, level 11 for drift),
- the names and descriptions as new strings in `Upgrades.str` (`add_string_key`), in a new `Upgrades` row for each level. Nothing the game already has is changed.

## License

MIT, see `LICENSE`.
