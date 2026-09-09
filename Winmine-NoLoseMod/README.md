# Minesweeper No-Lose Mod

A modified Minesweeper executable that lets you click mines without losing.

https://github.com/user-attachments/assets/85e68c22-d422-4581-8d57-72476c7baaf3


## How It Works

In the original game, you cannot lose on the first move. If your first click hits a mine, the game relocates it and continues normally. This modification applies that protection to every move.

Using IDA, I located the function that checks whether the clicked cell contains a mine. Board cells use these values:

- `0x8F`: an unflagged mine.
- `0x8E`: a flagged mine.
- `0x0F`: an unrevealed safe cell.

The function tests the mine bit (`0x80`). When a mine is detected, it checks whether this is the first move. If so, it relocates the mine instead of ending the game.

I replaced the turn-check comparison (`CMP turn, 0`) and conditional jump (`JNZ`) with `NOP` instructions. This bypasses the check, making the mine-relocation logic run on every mine click.

The patch changes only **7 bytes** in the executable and requires no DLL injection.

## Run

Launch `winmineModified.exe` directly.

- `winmine.exe`: original game.
- `winmineModified.exe`: patched game.
- `winmineNoloseDemo.mov`: demonstration video.
