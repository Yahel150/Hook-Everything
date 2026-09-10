# Solitaire Hooks

A Win32 DLL that adds visual effects and small gameplay aids to the supplied classic Solitaire executable. All hook logic is in [SolitaireHook.cpp](SolitaireHook.cpp): custom layouts, random table colors, editable score labels, Peek Mode, and move hints.

## 1. Using the hook

### Build and launch

Keep `sol.exe`, `cards.dll`, and the generated hook DLL in this folder. The standalone build requires Visual Studio's Desktop development with C++ tools and a Windows SDK.

From PowerShell in this folder:

```powershell
.\build.cmd
.\run.cmd
```

`build.cmd` selects the x86 compiler and builds both `SolitaireHook.dll` and `injector.exe`. `run.cmd` launches the game through the injector. You can also launch it directly with:

```powershell
.\injector.exe .\sol.exe "$PWD\SolitaireHook.dll"
```

Opening `sol.exe` by itself does not load the hook. Close every hooked instance before rebuilding, then launch again to load the new DLL.

To use an existing Visual Studio DLL project, select **Win32**, use `SolitaireHook.cpp` as the project's `dllmain.cpp`, and keep its normal precompiled-header setup. Compile only one copy of the hook source.

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

## 2. Investigating and implementing the hook

### Tools used

The investigation combined static analysis with focused runtime checks:

- **Python and `struct`:** read the PE headers and import table to identify the executable's architecture, preferred image base, imported functions, and IAT offsets.
- **GNU `objdump` through WSL:** inspected x86 assembly around drawing calls during the initial investigation.
- **Visual Studio `dumpbin /disasm`:** produced a local disassembly used to trace card rendering, game initialization, pile layouts, and move-rule code.
- **Visual Studio's x86 C++ compiler and linker:** built the DLL, injector, and local test programs. Linker map files identified DLL symbols during runtime checks.
- **Win32 runtime test programs:** injected the DLL into a separate test process, inspected memory using `ReadProcessMemory`, and exercised keyboard messages.
- **Windows computer-use screenshots and interaction:** checked the visible layouts, table colors, Peek preview, hint borders, and score-label dialog.

The bundled PDFs in other project folders discuss IDA, but this Solitaire implementation was investigated with the tools above; it does not depend on an IDA project.

### Discovery 1: rendering is separate from game state

The supplied `sol.exe` is a 32-bit PE executable with preferred image base `0x01000000`. Its imports showed three useful boundaries:

- `cards.dll` provides card drawing through `cdtDrawExt` and `cdtDraw`.
- `gdi32.dll` provides brushes, text measurement, and drawing operations.
- `user32.dll` provides the message loop, window controls, and painting lifecycle.

This suggested using IAT hooks for the visual effects and input controls, while reading the game's own data for Peek Mode and hints. A drawing function alone does not describe every rule or every hidden card.

### Discovery 2: the drawing call loses the hidden card's identity

The original card-drawing wrapper at preferred address `0x01001F45` reads a card record and checks bit `0x8000` in its value.

For a face-up card, it strips that bit and passes the card ID to `cdtDrawExt`. For a face-down card, it substitutes the selected card-back image number, stored at executable offset `0x7008`.

This was the key to Peek Mode: simply changing the renderer's mode from back to face would display the wrong card, because the argument may describe the back image rather than the hidden card. The real identity must be read from the original record.

Each record occupies 12 bytes:

| Offset in the card record | Meaning |
| --- | --- |
| `+0x00` | 16-bit card value |
| `+0x02` | Padding |
| `+0x04` | x coordinate |
| `+0x08` | y coordinate |

The card value encodes:

```text
card ID = value & 0x7FFF
rank    = card ID / 4       (Ace = 0, King = 12)
suit    = card ID % 4
face-up = (value & 0x8000) != 0
```

The DLL reads these values but does not write to card identities or face-up flags.

### Discovery 3: the board is a collection of 13 piles

Tracing initialization at preferred address `0x01005581` revealed the game object and its pile-pointer array. The current game pointer is stored at executable offset `0x7170`.

The pile order is:

| Index | Pile |
| --- | --- |
| `0` | Stock |
| `1` | Waste |
| `2–5` | Four foundations |
| `6–12` | Seven tableau columns |

Within the game object, the active pile count is at `+0x64` and the pointer array starts at `+0x6C`. Each pile contains geometry, a card count at `+0x1C`, and its card records starting at `+0x24`.

The disassembly also showed per-game and per-pile handler pointers used to dispatch operations. The hook does not replace those internal handlers: its small `Game`, `Pile`, and `Card` structures expose only the layout needed to read the board.

A live test confirmed 13 piles, 52 cards total, and 21 hidden tableau cards in a fresh deal. This connected the static memory-layout interpretation to actual game data.

### Discovery 4: the original rules explain legal destinations

The foundation check at preferred address `0x01004531` compares rank and suit. The column check at `0x01004279` requires an opposite-color destination one rank higher, with a special case for Kings entering empty columns.

The implementation expresses those rules in `fitsFoundation` and `fitsColumn`. In this card encoding, suits whose IDs XOR to `1` or `2` have opposite colors. The code checks the destination's face-up state before suggesting a column move.

`findHint` scans the waste and columns, examining only their top face-up cards. For each candidate it tries foundations, then other columns, and stops at the first match. This keeps the feature short; it is neither a complete move enumerator nor a solver.

### Installing the IAT hooks

The Import Address Table stores pointers to imported functions. Each `setHook` follows the same template used elsewhere in this project:

1. Find the executable and target DLL modules.
2. Save the original API address with `GetProcAddress`.
3. Locate the target IAT entry using an offset from the loaded executable base.
4. Temporarily make the four-byte entry writable with `VirtualProtect`.
5. Replace it with the hook function's address.
6. Restore the saved memory protection.

The hook calls the saved original function when normal rendering or message handling is needed. It changes the target process's IAT in memory, not the system DLL on disk.

| Installer | Imported function | IAT offset | Purpose |
| --- | --- | --- | --- |
| `setHook1` | `cards!cdtDrawExt` | `0x101C` | Intercept card drawing and suppress cards that will be drawn at shifted positions. |
| `setHook2` | `gdi32!CreateSolidBrush` | `0x1084` | Replace the initial green table brush. |
| `setHook3` | `user32!DrawTextW` | `0x1190` | Substitute the score label. |
| `setHook3` | `gdi32!GetTextExtentPoint32W` | `0x1050` | Measure the replacement label correctly. |
| `setHook3` | `user32!GetMessageW` | `0x1164` | Handle the shared keyboard shortcuts and editor input. |
| `setHook3` | `user32!EndPaint` | `0x10DC` | Draw the preview, shifted cards, and hint borders after the game's repaint. |

`DllMain` initializes the loaded executable base and color seed, then calls `setHook1()`, `setHook2()`, and `setHook3()` during `DLL_PROCESS_ATTACH`.

The injector creates Solitaire suspended, writes the DLL path into the target process, and starts a remote thread calling `LoadLibraryA`. It waits for that thread before resuming Solitaire's main thread. All feature implementations remain in one DLL source file.

### Discovery 5: clipping explains disappearing shifted cards

The first layout attempt changed y directly inside `cdtDrawExt`. Visual testing showed that the shifted card could disappear: its new position fell outside the game's existing drawing or clipping region.

The revised approach skips that original draw and draws the shifted card after `EndPaint`, using the main window's device context. The same stage draws the hint outlines. Their coordinates come from the source card and destination pile or card.

Peek Mode also uses this final drawing stage. It clears the tableau preview area and redraws the seven columns from the real records, using face-up mode and 20-pixel vertical spacing. Returning from Peek Mode requests a normal repaint; no game-state rollback is needed because no card state was changed.

### Discovery 6: brushes and text widths are cached separately

**Table color:** intercepting brush creation changes the initial color, but a later key press must update objects the game already created. `changeColor` replaces the cached brush at offset `0x737C`, updates the background color at `0x7348`, updates the window-class background brush, and requests a repaint. This keeps the table and card backgrounds aligned.

**Score label:** replacing the text alone was insufficient for long labels. The original score layout still measured `Score: ` and used that width when arranging and clearing the status area. The additional `GetTextExtentPoint32W` hook measures the same replacement text that `DrawTextW` draws, so the layout reserves its actual width.

**Extra repainting:** the message hook originally called `refresh()` on every ordinary click. That explicitly invalidated the full board. Installing a `GetMessageW` hook does not itself require such repainting. The revised code refreshes when a visual setting changes or a displayed hint needs clearing, rather than on every normal click.

### The score-label editor

E creates a small owned window using ordinary Win32 controls: a static prompt, an edit box, and Save/Cancel buttons. Its window procedure saves or discards the text and re-enables the game when the editor closes.

The shared message hook routes editor messages before Solitaire's own shortcut processing. This is why typing `score` into the box does not trigger S, C, or E as game controls. No separate UI framework or extra hook DLL is used.

## Validation and limits

Builds used the x86 Visual Studio toolchain. Focused tests checked foundation rules, opposite-color 3-on-4 moves across all suit pairs, hidden destinations, empty-column Kings, text measurement, key toggling, editor save/cancel behavior, and unchanged card values during Peek Mode.

Runtime injection tests checked the original five IAT hooks and P/H/L behavior. Visual checks covered the lowered-card correction, Peek preview, random table color, foundation hint/no-hint behavior, and editor appearance. The later column-hint, long-label measurement, and reduced-repaint changes were built and tested with focused checks; they did not receive another full visual regression session.

The offsets and structures are specific to the supplied 32-bit executable. They are not portable to modern Solitaire or arbitrary `sol.exe` versions. The shifted layouts are cosmetic, hints cover only the moves listed above, and there is no automatic solver.

## Files

| File | Purpose |
| --- | --- |
| `SolitaireHook.cpp` | All hook functions, board helpers, and label-editor code. |
| `pch.h` | Minimal header for the standalone build. |
| `injector.cpp` | Waiting-injector template copied into this folder. |
| `build.cmd` | Build the Win32 DLL and injector. |
| `run.cmd` | Launch the game through the injector. |
| `sol.exe`, `cards.dll` | Supplied game and card-rendering library. |
| `SolitaireHook.dll`, `injector.exe` | Generated binaries, ignored by Git. |
