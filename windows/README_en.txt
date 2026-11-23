Rogue 5.4.5jec.057  Windows Edition
======================================

Rogue is a computer role-playing game that originated around 1980. You
explore a dungeon drawn with characters and symbols, fight monsters,
and use various items while searching for the Amulet of Yendor.

Rogue is also known as the game that gave rise to the genre now called
"roguelikes." Rogue 5.4 is the final version of that original Rogue.
With dungeons that change each time you play, a variety of items whose
effects you discover and learn to use, and tense adventures where every
decision can change your fate, it remains compelling even today.

This package contains Rogue 5.4.5jec.057 for Windows 10 / 11 (64-bit).
It preserves the gameplay of Rogue 5.4 while adding Japanese/English
language switching, Japanese and emoji input, color display, animation
delays, and compatibility with modern operating systems. The color display
feature and the option to combine identify scrolls into one type were
ported from Rogue 5.4.5p, released by Y.Oz Vox.

[Included Files]

  rogue.exe             Standard edition
  rogue_original.exe    Original-settings edition
  rogue_ja.html         Guide to the Dungeons of Doom (Japanese)
  rogue_en.html         Guide to the Dungeons of Doom (English)
  LICENSE.txt           Copyright and license information
  README_ja.txt         Japanese README
  README_en.txt         This file (English README)

The standard and original-settings editions use the same game program,
but differ in default settings such as language, color display, and
identify scroll types.
The standard edition favors convenient play. The original-settings
edition starts in English and uses display and control defaults closer
to the original Rogue 5.4. You can change the language in either edition.

  Option                          Standard            Original settings
  ----------------------------------------------------------------------
  Startup language                System locale       English
  Color                           On                  Off
  Follow corridor bends           On                  Off
  Single identify scroll type     On                  Off
  Cursor                          Off                 On

You can change color, movement, and cursor options in the game.
The original Rogue 5.4 has five identify scroll types. The standard
edition enables idscrl, which combines them into one type. This setting
cannot be changed after a game has started.

[Starting the Game]

1. Extract the ZIP archive to a folder of your choice. Use a folder where
   you have write access so that the game can create save files.
   (Example: C:\Games\Rogue)

2. In File Explorer, double-click rogue.exe for the standard edition or
   rogue_original.exe for the original-settings edition.

   If the default save file exists, a launch menu appears:

     [R/Enter] Resume saved adventure
     [N]       Start a new adventure
     [F]       Choose another save file
     [Q]       Quit

   If there is no default save, a new game starts without the menu.
   Pressing F opens the Windows file picker; cancelling it returns to
   the menu. Starting a new game does not delete the existing save.

3. You can drag and drop a saved-game file onto rogue.exe (or
   rogue_original.exe) to resume that file directly.

4. To launch from a terminal, open the extracted folder. On Windows 11,
   right-click and select "Open in Terminal". If that option is not
   available, enter powershell in the folder's address bar and press
   Enter. Then run one of these commands:

   Standard edition:
       .\rogue.exe

   Original-settings edition:
       .\rogue_original.exe

   You can also use Command Prompt (cmd.exe). Launching without an
   argument from an existing terminal starts a new game.

Use a terminal at least 80 columns wide and 24 rows high. The game
display is fixed at 80x24. On a direct
double-click launch, the program attempts to resize Windows Terminal or
Console Host to 80x24. Some terminals may not honor this request.
It does not resize a terminal from which you launch it manually.

During a game, press ! to open Windows Command Prompt. Type exit to
close the command prompt and return to Rogue.

[Save Files and Scores]

You can create a save file using the in-game save command. To resume the
default saved game from a terminal, run:

       .\rogue.exe -r

To resume a save file with another name or location, specify its path:

       .\rogue.exe "save_filename"

To display the high-score list only:

       .\rogue.exe -s

To display a tombstone and attempt to record a score:

       .\rogue.exe -d

For the original-settings edition, substitute rogue_original.exe for
rogue.exe. After a successful restore, the original save file is deleted
by design. Copy any saved game you want to keep before resuming it.

Both editions share the following default file names:

  Save file    rogue.save
  Score file   rogue.scr

These files are stored in the working directory. When launched directly
from File Explorer, this is normally the executable's folder. Both
editions can resume from the same save file and share the high scores.
Avoid using the same save file in multiple games at the same time.

You can change the default save filename with file= in ROGUEOPTS.

[Setting Environment Variables in Windows]

The ROGUEOPTS environment variable can configure startup options.
For available settings and their meanings, see the "Options" section of
the included game guide. This section explains only how to set the
variable in Windows.

Example of a temporary setting in PowerShell:

       $env:ROGUEOPTS = "name=Alex,language=ja"
       .\rogue.exe

Example in Command Prompt (cmd.exe):

       set "ROGUEOPTS=name=Alex,language=ja"
       rogue.exe

These settings apply to the current terminal and programs launched from
it. They do not automatically apply to File Explorer double-click launches.

To use the same settings every time, add ROGUEOPTS to your Windows user
environment variables:

  1. Press Win + R, enter sysdm.cpl, and press Enter.
  2. Open "Advanced" -> "Environment Variables".
  3. Under "User variables", select "New" (or "Edit" if it already exists).
  4. Enter ROGUEOPTS as the variable name and your settings as its value.
  5. Save with "OK". If necessary, sign out of Windows and sign in again.

Delete the user environment variable when you no longer need it.

[Game Guide and License]

For game controls, display symbols, commands, items, and options, open
the included "Guide to the Dungeons of Doom" (rogue_ja.html or
rogue_en.html) in a web browser. No Internet connection is required.

See LICENSE.TXT for copyright and license information on the original
Rogue and pre-existing code, JEC additions (including localization,
features, and the Windows icon), and acknowledgment of Y.Oz Vox's
work on Rogue 5.4.5p.
