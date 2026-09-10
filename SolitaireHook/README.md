# Solitaire Hooks

A Win32 DLL that adds visual effects and small gameplay aids to the supplied classic Solitaire executable. All hook logic is in 
[SolitaireHook.cpp](SolitaireHook.cpp): custom layouts, random table colors, editable score labels, Peek Mode, and move hints.
  


https://github.com/user-attachments/assets/38aacec7-708d-42c8-b0ca-44eb41ca7c48



https://github.com/user-attachments/assets/26b428c8-96bf-4d8d-b9d1-6f850fbe953f


## 1. Using the hook

### Build and launch
Keep `sol.exe`, `cards.dll`, and the generated hook DLL in this folder. The standalone build requires Visual Studio's Desktop development with C++ tools and a Windows SDK.

```powershell
.\injector.exe .\sol.exe SolitaireHook.dll
```

### Keyboard shortcuts

Use these keys while the Solitaire window has focus:

| Key | Action |
| --- | --- |
| **L** | Cycle between the staircase, normal, and original lowered-card layouts. |
| **P** | Toggle Peek Mode on or off. |
| **H** | Highlight a legal single-card move, when one is found. |
| **C** | Generate a new random table color. |
| **S** | Switch the score label between `Nice!` and `Score`. |
| **E** | Open a text box for a custom score label. |
| **F2** | Start a new deal using Solitaire's existing shortcut. |

### L: custom card layouts

Initially, the first face-up card is drawn 100 pixels lower, matching the original exercise. From this starting mode:

1. Press L once for a staircase layout: columns farther to the right are drawn lower.
2. Press L again for the normal layout.
3. Press L again to return to the original lowered-card exercise.

**Use the normal layout when playing.** These effects change drawing coordinates, not the game's clickable regions. The coordinate conditions are designed around this executable's initial board layout.

### P: Peek Mode

Press P once. The title changes to **Peek ON (P to return)**, and the seven tableau columns are displayed face-up with wider vertical spacing. This makes the hidden cards' rank and suit corners readable.

Press P again to restore the previous display. There is no need to hold the key; keyboard auto-repeat does not repeatedly toggle it.

Peek Mode is a read-only preview. It does not change the cards' internal face-up flags, make hidden cards playable, or reveal the stock's order. Mouse actions are blocked during the preview because the expanded display does not match the normal click positions. Large late-game columns may extend below a small window; a fresh deal gives the clearest demonstration.

### H: move hints

Switch to normal layout and press H. The hook turns Peek Mode off and searches for a move:

- **Yellow border:** the source card to move.
- **Cyan border:** its destination card, foundation, or empty column.

The search considers the top face-up card of the waste pile and each tableau column. It supports:

- An Ace onto an empty foundation.
- The next rank of the same suit onto a foundation.
- A card onto a column card one rank higher and of the opposite color, such as a black 3 onto a red 4.
- A King onto an empty column.

Move the card yourself, then press H again for another hint. Clicking clears the hint state. A hint is a legal move, not necessarily the best strategic move.

If the title says **No single-card hint**, the search found no supported move. It does not mean the game is lost. Drawing from the stock, flipping an exposed face-down card, or moving a whole sequence may help. This small implementation does not search sequences, stock actions, or moves out of foundations.

For a quick demonstration, find an exposed Ace and press H, or look for an opposite-color pair such as black 3 and red 4. Hidden Aces shown by Peek Mode are not eligible until actually exposed in the game.

### C: random table colors

The table starts purple. Press C for another color. The hook updates the table brush, the window-class background brush, and the color used around cards so they remain consistent.

A small private random generator chooses the colors. It does not use or reseed Solitaire's shuffle generator.

### S and E: score labels

Press S to toggle between `Nice!` and `Score`. The numeric score is unchanged.

For a custom label:

1. Press E to open **Change score label**.
2. The dialog shows a prompt and a text box containing the current label, selected for replacement.
3. Type up to 12 characters, such as `My points`. Your input remains visible in the box.
4. Click **Save** or press Enter to apply it. Click **Cancel** or press Esc to keep the previous label.

Letters typed in the editor are text rather than hook shortcuts. If the score is not visible, enable Standard scoring in **Game > Options**. Labels and colors are session settings; they are not saved for the next launch.
## 2. analyzing the binary

### Card drawing and game data

`sol.exe` calls `cards.dll!cdtDrawExt` with the card image, drawing mode, position, size, and background color. Its drawing wrapper at `0x01001F45` passes the real card ID for face-up cards, but substitutes a card-back image for hidden cards. Peek Mode therefore reads the original card records instead of using the back-image argument.

The game pointer is stored at executable offset `0x7170`. The game contains 13 pile pointers starting at `+0x6C`: stock, waste, four foundations, and seven tableau columns. Each pile stores its card count at `+0x1C` and 12-byte card records starting at `+0x24`.

A card record contains its value and x/y coordinates. `value & 0x7FFF` gives the card ID; dividing this ID by four gives the rank, and its remainder gives the suit. Bit `0x8000` marks a face-up card. Peek Mode draws these records face-up without changing that bit.

### Move hints

The foundation check at `0x01004531` and column check at `0x01004279` show the rank and suit rules. The hook applies these rules to the top face-up cards in the waste and tableau: Aces start foundations, foundations increase by rank within one suit, columns decrease by rank with alternating colors, and only Kings enter empty columns. It returns the first supported move and highlights its source and destination.

### Hooked functions

Each hook replaces a four-byte entry in the executable's Import Address Table (IAT), saves the original function address, and restores the original memory protection after writing the new pointer.

| Function | IAT offset | Use |
| --- | --- | --- |
| `cards!cdtDrawExt` | `0x101C` | Intercept card drawing for the custom layouts. |
| `gdi32!CreateSolidBrush` | `0x1084` | Replace the initial green table brush. |
| `user32!DrawTextW` | `0x1190` | Draw the custom score label. |
| `gdi32!GetTextExtentPoint32W` | `0x1050` | Reserve the correct width for that label. |
| `user32!GetMessageW` | `0x1164` | Handle keyboard shortcuts and label-editor input. |
| `user32!EndPaint` | `0x10DC` | Draw shifted cards, the Peek preview, and hint borders after the board repaint. |

Shifted cards are drawn after the normal repaint because changing their coordinates inside the original draw call can place them outside its clipping region. Changing the table color also updates the cached brush at offset `0x737C`, the background color at `0x7348`, and the window-class brush.

`setHook1()` installs the card hook, `setHook2()` installs the brush hook, and `setHook3()` installs the text, measurement, input, and paint hooks. The score-label editor uses standard Win32 text-box and button controls. All hook logic remains in `SolitaireHook.cpp`.

The addresses above use the preferred base `0x01000000`; IAT offsets are added to the actual loaded base. These layouts apply to the supplied `sol.exe` and `cards.dll`, not arbitrary Solitaire versions.


## Files

| File | Purpose |
| --- | --- |
| `SolitaireHook.cpp` | All hook functions, board helpers, and label-editor code. |
| `injector.cpp` | Waiting-injector template copied into this folder. |
| `sol.exe`, `cards.dll` | Supplied game and card-rendering library. |
