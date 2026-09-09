# Minesweeper Auto-Flag

A Minesweeper hook that automatically flags every mine at the start of the game.

[Video demo](https://github.com/user-attachments/assets/41b56950-4fd2-41f1-8d6e-4606b5a6e1c0)

## How It Works

On beginner difficulty, Minesweeper has a 9×9 board containing 10 hidden mines. The goal is to reveal every safe cell without clicking a mine. Revealed numbers indicate how many mines are adjacent to that cell, and flags mark suspected mines.

This hook reveals mine locations by flagging them automatically. You can then win by clicking all remaining safe cells.

The injected DLL hooks `UpdateWindow` and modifies the board in memory, changing unflagged mine cells (`0x8F`) into flagged mine cells (`0x8E`) before calling the original function.

## Run

From this folder, run:

```powershell
.\injector.exe .\winmine.exe "$PWD\winmineHook.dll"
