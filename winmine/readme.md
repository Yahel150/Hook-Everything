# Minesweeper Auto-Flag

winmineHook.cpp hooks UpdateWindow and changes mine cells in board memory from 0x8F to 0x8E, flagging mines automatically. Safe cells still need to be revealed.

Includes Minesweeper, an injector, and a prebuilt DLL. The hook uses hardcoded IAT and board offsets for the bundled 32-bit executable.

[Video demo](https://github.com/user-attachments/assets/41b56950-4fd2-41f1-8d6e-4606b5a6e1c0)
