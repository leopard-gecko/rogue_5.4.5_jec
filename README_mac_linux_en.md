# Rogue 5.4.5 JEC 056 — macOS / Linux (English)

Rogue is a computer role-playing game created around 1980, featuring dungeons drawn with letters and symbols. Players battle monsters and make use of a variety of items as they search for the Amulet of Yendor hidden deep within the dungeon.

**Rogue 5.4 is the final version of the original Rogue line, refined through successive releases since the first Rogue.** Its gameplay mechanics are more refined than those of earlier versions. Despite its simple text-based display, its ever-changing dungeons, creative use of items, and tense adventures give it a timeless appeal that makes it enjoyable to play even today.

This modified edition leaves the gameplay of Rogue 5.4 essentially unchanged while adding Japanese/English language switching, color display, and support for modern operating systems such as macOS, Linux, and Windows.

For the game overview, controls, items and how to play, see the [「【運命の洞窟】への案内書」 (Dungeon Guide)](https://leopard-gecko.github.io/jrogue/guide.html?version=54). This README covers building, launching, file locations and distribution.

This guide covers **macOS, Linux, and Raspberry Pi OS**. Other guides: [macOS / Linux・Japanese](README_mac_linux_ja.md) / [Windows・English](README_windows_en.md) / [Windows・Japanese](README_windows_ja.md).

## 1. Requirements

Extract the source archive and open a terminal in its directory.

**macOS:** install Apple's command-line tools:

```sh
xcode-select --install
```

**Ubuntu / Debian / Raspberry Pi OS:**

```sh
sudo apt update
sudo apt install build-essential pkg-config libncurses-dev
```

**Fedora:** `sudo dnf install gcc make pkgconf-pkg-config ncurses-devel`  
**Arch Linux:** `sudo pacman -S --needed base-devel pkgconf ncurses`  
**Alpine Linux:** `sudo apk add build-base pkgconf ncurses-dev`

On Linux, configure the build for your environment first:

```sh
./configure
```

On macOS, `make` usually works directly. If curses needs special configuration, see the `--with-curses-cflags` and `--with-curses-libs` options to `./configure`.

## 2. Build

For the regular build:

```sh
make clean
make
```

This produces **`rogue`**. A regular `make` uses convenient default options, while `make original` builds the game with defaults closer to the original Rogue 5.4 and English as the initial language. Both builds still allow changing the language during play.

The names `color`, `passgo`, and others in the table below are game options. `on` means enabled and `off` means disabled. You can also override these defaults at launch using the **`ROGUEOPTS`** environment variable.

| Setting | Regular `make` | `make original` |
|---|---|---|
| Startup language | OS language | English |
| `color` (color display) | on | off |
| `passgo` (automatically turn corners in passages) | on | off |
| `idscrl` (one kind of identify scroll) | on | off |
| `cursor` (cursor display) | off | on |

An **identify scroll** reveals what an item is. Original Rogue 5.4 has five types, each for a particular category of items. Enabling `idscrl` replaces them with one type that can identify any of those categories. This option cannot be changed once a game has started; set `idscrl` (one type) or `noidscrl` (five types) in `ROGUEOPTS` before launching the game.

The language and other changeable options can also be adjusted in the in-game options screen. For details, see the 「【運命の洞窟】への案内書」 (Dungeon Guide) linked above.

| Purpose | Command |
|---|---|
| Regular build | `make` |
| Original-style defaults, English | `make original` |
| Include wizard support | `make wizard` |
| Name the executable `jrogue` | `./configure --program=jrogue` → `make clean` → `make` |

Use `make clean` when changing the build variant or executable name. Running `configure` stores the settings in `config.mk`. **`make clean` and `make distclean` do not delete save or score files.** `make clean` removes build products; `make distclean` also removes `config.mk` and `config.log`.

## 3. Launch

```sh
./rogue
```

A normal launch starts a new game. Use a terminal of at least **80 columns × 24 rows**.

You can also use these startup options:

| Command | Purpose |
|---|---|
| `./rogue -r` | Resume the game from the default save file |
| `./rogue -s` | Display the high-score list |
| `./rogue -d` | Display a tombstone and attempt to record a score |

For details, see the 「【運命の洞窟】への案内書」 (Dungeon Guide).

## 4. Installation (optional)

You can run directly from the build directory; installation is optional. A standard installation under `/usr/local` uses:

```sh
sudo make install
```

To install elsewhere, first specify `./configure --prefix=DIR`. Use `--program=NAME` to change the executable name. English and Japanese manual pages are also installed; use `--ja-man-style=auto|mac|linux|all|none` to select the Japanese manual location. `make uninstall` removes the installation.

## 5. Save and score file locations

By default, **`rogue.save`** and **`rogue.scr`** are located in the game's working directory. Changing the executable name normally changes the default data filenames to match.

Score updates lock `rogue.scr` itself; no separate lock file is created. If an older build left `rogue.lck` behind, you can remove it manually after confirming that the older build is no longer running.

**Keep both files in a dedicated directory:**

```sh
mkdir -p "$HOME/.local/share/rogue545"
./configure --file-dir="$HOME/.local/share/rogue545"
make clean
make
sudo make install
```

The game must be able to write to the data directory, so this example creates it first as the regular user. `make install` can create a missing `--file-dir` directory, but a directory created under `sudo` may not be writable by the player. `make clean` does not delete existing save or score files.

You can choose individual filenames with `./configure --save-file=FILE` and `--score-file=FILE`. Without `configure`, build-time variables include `PROGRAM=NAME` (or `PROG=NAME`) and `FILEDIR=DIR`. Individually specified filenames override `FILEDIR`. Rebuild after changing these settings.

## 6. Files and distribution

| File | Purpose |
|---|---|
| `rogue` | Built executable |
| `rogue.save` / `rogue.scr` | Saved game / scores |
| `configure` / `Makefile` | Build and installation |
| `config.mk` / `config.log` | Settings and log generated by `configure` |
| `rogue.6.en.in` / `rogue.6.ja.in` | English and Japanese manual page templates |
| `*.c` / `*.h` | Source code |
| `rogue_ja.txt` / `rogue_en.txt` | Japanese and English plain-text game guides |
| `rogue_ja.html` / `rogue_en.html` | Japanese and English HTML guides |
| `LICENSE.TXT` | Copyright, licenses, and JEC contributions |
| `README_mac_linux_ja.md` / `README_mac_linux_en.md` | macOS/Linux READMEs (Japanese/English) |
| `README_windows_ja.md` / `README_windows_en.md` | Windows READMEs (Japanese/English) |

Use `make dist.src` to create a source archive (normally `rogue-5.4.5jec.056-src.tar.gz`).

## 7. Main changes from the original Rogue 5.4.5 line

- **Modern systems:** Builds and runs on macOS, Linux and Raspberry Pi OS.
- **Two languages:** Japanese messages, items and settings, with startup and in-game switching.
- **Color and controls:** Configurable color, passage movement and cursor display.
- **Identify scrolls:** The option to combine identify scrolls into a single type was ported from Rogue 5.4.5p published by Y.Oz Vox. It is enabled by default in regular builds.
- **Build and save files:** Configurable executable names, installation paths and save/score locations.
- **Display and input:** Improved UTF-8 display, input handling, screen drawing and stability.

## 8. Copyright and licenses

See `LICENSE.TXT` for the original Rogue and pre-existing code licenses, JEC additions (including localization, features, and the Windows icon), and acknowledgment of Y.Oz Vox's Rogue 5.4.5p work.
