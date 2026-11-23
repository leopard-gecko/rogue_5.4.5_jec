# Rogue 5.4.5 JEC 057 — macOS / Linux (English)

This document explains how to build, run, and install Rogue 5.4.5 JEC on **macOS, Linux, and Raspberry Pi OS**, including save and score file locations. For an introduction and links to all platform guides, see the [repository README](https://github.com/leopard-gecko/rogue_5.4.5_jec). For gameplay, commands, and items, see [A Guide to the Dungeons of Doom](https://leopard-gecko.github.io/jrogue/guide.html?version=54).

日本語版：[README_mac_linux_ja.md](README_mac_linux_ja.md)

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

The language and other changeable options can also be adjusted in the in-game options screen. For details, see the dungeon guide linked above.

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

For details, see the dungeon guide.

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

You can also specify `--file-dir='~/Documents'`. A leading `~` expands to the home directory when configuring; direct `make FILEDIR='~/Documents'` builds are supported too.

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

For the dungeon guides and other platform READMEs, see the [repository README](https://github.com/leopard-gecko/rogue_5.4.5_jec).

Use `make dist.src` to create a source archive (normally `rogue-5.4.5jec.057-src.tar.gz`).

## 7. Copyright and licenses

See `LICENSE.TXT` for the original Rogue and pre-existing code licenses, JEC additions (including localization, features, and the Windows icon), and acknowledgment of Y.Oz Vox's Rogue 5.4.5p work.
