The game is played on a 9×9 board (or larger) and contains 10 bombs (or more), hidden in unknown locations. You are not allowed to click on the bombs. Instead, you must click on all the safe squares until only the bombs remain on the board. After each square is clicked, a number appears showing how many bombs are adjacent to it. Additionally, if a player suspects a square contains a bomb, they can mark it with a flag.

This hook is similar to the previous WinMine hook, but unlike that one, it is also a physical hook—meaning we modified the executable directly, not at runtime, and without using an injector. Specifically, we changed only 7 bytes of the executable file. Furthermore, instead of revealing the bomb locations, this hook simply places us in a state where we cannot lose the game (see the demo video for reference).


https://github.com/user-attachments/assets/4bd80e48-879e-4d73-80e8-338c65519f20

