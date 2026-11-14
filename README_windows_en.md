# Rogue 5.4.5 JEC 056 — Windows (English)

Rogue is a computer role-playing game created around 1980, featuring dungeons drawn with letters and symbols. Players battle monsters and make use of a variety of items as they search for the Amulet of Yendor hidden deep within the dungeon.

**Rogue 5.4 is the final version of the original Rogue line, refined through successive releases since the first Rogue.** Its gameplay mechanics are more refined than those of earlier versions. Despite its simple text-based display, its ever-changing dungeons, creative use of items, and tense adventures give it a timeless appeal that makes it enjoyable to play even today.

This modified edition leaves the gameplay of Rogue 5.4 essentially unchanged while adding Japanese/English language switching, color display, and support for modern operating systems such as macOS, Linux, and Windows.

For the game overview, controls, items and how to play, see the [「【運命の洞窟】への案内書」 (Dungeon Guide)](https://leopard-gecko.github.io/jrogue/guide.html?version=54). This README covers building, launching, setting Windows environment variables, file locations and distribution.

This guide covers **Windows 10 / 11**. Other guides: [Windows・Japanese](README_windows_ja.md) / [macOS / Linux・English](README_mac_linux_en.md) / [macOS / Linux・Japanese](README_mac_linux_ja.md).

## 1. Requirements

To build a native Windows executable, use **MSYS2 UCRT64**. Install MSYS2 from <https://www.msys2.org/> and open “MSYS2 UCRT64” from the Start menu.

```sh
pacman -Syu
```

If requested, restart UCRT64 and complete the update, then install the compiler, tools and PDCursesMod:

```sh
pacman -S --needed make mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-binutils mingw-w64-ucrt-x86_64-pdcurses
```

Change to the extracted source directory.

## 2. Build

```sh
make clean
make
```

This produces **`rogue.exe`**.

| Purpose | Command |
|---|---|
| Regular build | `make` |
| Build as `jrogue.exe` | `make PROGRAM=jrogue` (or `make PROG=jrogue`) |
| Original-style defaults, English | `make original` |
| Build a separately named original-style EXE | `make PROG=rogue_original original` |
| Include wizard mode | `make wizard` |

A regular `make` uses convenient default options, while `make original` builds the game with defaults closer to the original Rogue 5.4 and English as the initial language. The names `color`, `passgo`, and others in the table below are game options. `on` means enabled and `off` means disabled. You can also override these defaults at launch using the **`ROGUEOPTS`** environment variable.

| Setting | Regular `make` | `make original` |
|---|---|---|
| Startup language | Windows UI language | English |
| `color` (color display) | on | off |
| `passgo` (automatically turn corners in passages) | on | off |
| `idscrl` (one kind of identify scroll) | on | off |
| `cursor` (cursor display) | off | on |

An **identify scroll** reveals what an item is. Original Rogue 5.4 has five types, each for a particular category of items. Enabling `idscrl` replaces them with one type that can identify any of those categories. This option cannot be changed once a game has started; set `idscrl` (one type) or `noidscrl` (five types) in `ROGUEOPTS` before launching the game.

The language and other changeable options can also be adjusted in the in-game options screen. For details, see the 「【運命の洞窟】への案内書」 (Dungeon Guide) linked above. Clean and rebuild when switching build modes.

Windows builds use only the **PDCursesMod VT rendering backend**. Regular `make` and `make original` differ only in initial options and language; there is no legacy WinCon backend selection. Exact colors and emoji rendering still depend on the terminal and installed fonts.

## 3. Launch and exit

**Double-click `rogue.exe`** in Explorer to start. When a default saved game exists, the startup menu offers **[R/Enter] Resume saved adventure**, **[N] Start a new adventure**, **[F] Choose another save file**, or **[Q] Quit**. Without a default save, a new game starts immediately, without the menu. You can also **drag and drop a saved-game file onto `rogue.exe`** to resume that file directly; paths containing Japanese characters or spaces are supported. PowerShell, Command Prompt, and MSYS2 UCRT64 remain available for command-line launches:

```powershell
.\rogue.exe
```

A normal launch from an existing shell begins a new game. A terminal of at least **80 columns × 24 rows** is needed. Direct double-click launches attempt to resize Windows Terminal or Console Host to 80×24. Running from an existing shell does not change its window size. Some terminal hosts may ignore the resize request.

These options are also available when launching from a shell such as PowerShell:

| Command | Purpose |
|---|---|
| `rogue.exe -r` | Resume the game from the default save file |
| `rogue.exe -s` | Display the high-score list |
| `rogue.exe -d` | Display a tombstone and attempt to record a score |

Press `!` during the game to open a Windows command shell temporarily. Type `exit` to close the shell and return to Rogue. The shell is selected by `COMSPEC`, or `cmd.exe` if that environment variable is unset.

In PowerShell, prefix the executable name with `.\` (for example, `.\rogue.exe -s`). For details, see the 「【運命の洞窟】への案内書」 (Dungeon Guide).

## 4. Setting environment variables on Windows

Rogue reads its startup settings from the Windows environment variable **`ROGUEOPTS`**. This section explains how to set that variable on Windows, rather than describing the available game options. For option names and their meanings, see the “Options” section of the [「【運命の洞窟】への案内書」 (Dungeon Guide)](https://leopard-gecko.github.io/jrogue/guide.html?version=54).

**Set it temporarily in the current shell:** the following commands use `language=ja` only as an example value.

PowerShell:

```powershell
$env:ROGUEOPTS = 'language=ja'
.\rogue.exe
```

Command Prompt:

```cmd
set "ROGUEOPTS=language=ja"
rogue.exe
```

MSYS2 UCRT64:

```sh
export ROGUEOPTS='language=ja'
./rogue.exe
```

These settings apply to the current shell and programs launched from it. They are not inherited by an unrelated terminal or an Explorer double-click launch.

**Save it as a persistent Windows user environment variable:**

1. Press `Win + R`, enter `sysdm.cpl`, and press Enter.
2. Open the **Advanced** tab and select **Environment Variables**.
3. Under **User variables**, choose **New**, enter `ROGUEOPTS` as the variable name and the desired string (for example, `language=ja`) as the value. Choose **Edit** if it already exists.
4. Confirm with **OK**. Use a newly opened terminal; if the setting is not picked up, sign out of Windows and sign back in.

To stop using the persistent setting, remove the `ROGUEOPTS` user variable.

## 5. Save and score file locations

By default, **`rogue.save`** and **`rogue.scr`** are placed in the game's working directory. The binary distribution is intended to be run from its extracted directory. Other data directories can be chosen **at build time**.

Score updates lock `rogue.scr` itself; no separate lock file is created. If an older build left `rogue.lck` behind, you can remove it manually after confirming that the older build is no longer running.

In MSYS2 UCRT64, create a data folder first and select it for the build:

```sh
mkdir -p rogue-data
make clean
make FILEDIR=rogue-data
```

You can also use an absolute Windows path with `/` separators:

```sh
make clean
make FILEDIR='C:/Games/Rogue545'
```

Normal `FILEDIR` paths are **not created automatically**. Create them first and ensure that the game can write to them, or run `make prepare-filedir FILEDIR='C:/Games/Rogue545'`.

**Use the Windows Saved Games known folder (personal builds only):**

```sh
make clean
make FILEDIR=saved-games
```

This uses the Windows Saved Games folder (including relocation) and creates its `Rogue` subfolder when needed. **The builder's absolute directory is embedded in the EXE, so do not use this setting for redistributable binaries.**

Build variables such as `SAVEFILE=FILE` and `SCOREFILE=FILE` can override individual filenames; individual overrides take precedence over `FILEDIR`. For redistribution, leave `FILEDIR` unset.

**Use the same data filenames for both binary-distribution variants:**

```sh
make clean
make PROGRAM=rogue SAVEFILE=rogue.save SCOREFILE=rogue.scr
make original PROGRAM=rogue_original SAVEFILE=rogue.save SCOREFILE=rogue.scr
```

This leaves both `rogue.exe` and `rogue_original.exe` using the same `rogue.save` and `rogue.scr`. Both are VT builds. Concurrent score updates are protected, but do not operate on the same saved game from multiple processes.

Relative paths are resolved from the game's working directory.

## 6. Distributed files and guides

You can create the complete source-distribution archive from the MSYS2 UCRT64 shell:

```bash
make dist.src
```

This produces `rogue-5.4.5jec.056-src.tar.gz` from the same source manifest used on macOS/Linux, including source files for all supported platforms.

| File | Purpose |
|---|---|
| `rogue.exe` | Regular executable |
| `rogue_original.exe` | Original-style executable, if included in the distribution |
| `rogue.save` / `rogue.scr` | Save / score files |
| `Makefile` / `Makefile.win` | Source builds |
| `windows/rogue.ico` / `windows/rogue-icon.rc` | Windows icon resources |
| `windows/windows-saved-games.ps1` | Helper for the Windows Saved Games path |
| `rogue_ja.html` / `rogue_en.html` | Japanese and English HTML guides |
| `rogue_ja.txt` / `rogue_en.txt` | Japanese and English plain-text guides |
| `README_mac_linux_ja.md` / `README_mac_linux_en.md` | macOS/Linux READMEs (Japanese/English) |
| `README_windows_ja.md` / `README_windows_en.md` | Windows READMEs (Japanese/English) |
| `LICENSE.TXT` | Copyright, licenses, and JEC contributions |

When copying a built EXE to another computer, check whether it requires runtime DLLs. For game controls and strategy, see the Dungeon Guide linked at the top.

## 7. Main changes from the original Rogue 5.4.5 line

- **Native Windows:** Build with MSYS2 UCRT64 and PDCursesMod.
- **Japanese and English:** Select the language at startup or during play.
- **Unicode input:** Improved handling of Japanese text and emoji in player and fruit names.
- **Color and icon:** Color display and a Windows executable icon.
- **Terminal behavior:** The basic display is 80×24 and direct launches attempt to resize the terminal.
- **Regular/original variants:** Choose between modern and original-style build defaults.
- **Identify scrolls:** The option to combine identify scrolls into a single type was ported from Rogue 5.4.5p published by Y.Oz Vox. It is enabled by default in regular builds.
- **Files and stability:** Configurable data locations and improvements to drawing, items and scores.

## 8. Copyright and licenses

See `LICENSE.TXT` for the original Rogue and pre-existing code licenses, JEC additions (including localization, features, and the Windows icon), and acknowledgment of Y.Oz Vox's Rogue 5.4.5p work.
