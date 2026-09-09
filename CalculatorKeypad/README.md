# Calculator Keypad

Hooks SetWindowTextW and DrawTextW to swap displayed digits 1↔7, 2↔8, and 3↔9. This changes text, not calculator arithmetic or button commands.

CalculatorKeypadHook.cpp uses hardcoded IAT offsets for a specific 32-bit Calculator executable. No complete build project is included.
