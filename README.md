# Hook Everything

An educational collection of Windows C++ experiments that modify classic Calculator, Notepad, Minesweeper, and Solitaire using DLL injection, API hooking, and executable patching.


## Project overview

| Folder | Behavior | Technique |
| --- | --- | --- |
| [CalculatorKeypad](CalculatorKeypad/README.md) | Swap displayed digits: 1 ↔ 7, 2 ↔ 8, 3 ↔ 9. | IAT hooks on `SetWindowTextW` and `DrawTextW`. |
| [ChangeColorCalc](ChangeColorCalc/README.md) | Randomize Calculator text colors. | Inline hotpatch around `SetTextColor`. |
| [ReverseTextCalc](ReverseTextCalc/README.md) | Reverse Calculator's displayed text. | IAT hook on `SetWindowTextW`. |
| [PeterPan](PeterPan/readme.md) | Replace typed Notepad characters with text from Peter Pan. | IAT hook on `GetMessageW`. |
| [winmine](winmine/readme.md) | Automatically flag mines. | Hook `UpdateWindow` and modify board memory. |
| [Winmine-NoLoseMod](Winmine-NoLoseMod/README.md) | Allow mine clicks without losing. | Seven-byte executable patch; no DLL injection. |
| [SolitaireHook](SolitaireHook/README.md) | Add layouts, colors, score labels, hidden-card previews, and hints. | Multiple IAT hooks and reads of game data. |

Each folder's README explains its implementation and usage. Some folders also include exercise PDFs and demonstration videos.

## General hooking method

### Analyze the target

Find where the application handles the behavior to change: drawing text, retrieving keyboard messages, rendering cards, or checking a game rule. Inspecting PE headers and imports identifies the executable's architecture and imported functions. Disassembly and debugging connect these functions to visible behavior and help locate internal game data. The folder guides describe analysis with IDA. 

For an IAT hook, calculate the imported function entry's offset relative to the executable's image base. Add that offset to the actual loaded base at runtime. These offsets remain specific to the executable being analyzed.

### Load the DLL

The injector starts the target with its main thread suspended, writes the hook DLL's path into that process, and starts a remote thread calling `LoadLibraryA`. Loading the DLL invokes `DllMain` with `DLL_PROCESS_ATTACH`, where the example installs its hooks.

The waiting-injector variant waits for the DLL-loading thread before resuming the application's main thread, allowing initialization hooks to run before normal startup.

### Redirect calls through the IAT

The Import Address Table (IAT) holds pointers to imported functions. Most examples:

1. Save the original API address.
2. Locate its entry in the target's IAT.
3. Temporarily make that memory writable using `VirtualProtect`.
4. Replace the pointer with the hook's address.
5. Restore the saved original memory protection.

Calls through the modified entry then follow this path:

```text
Application → patched IAT entry → hook → original API
```

The hook matches the API's signature and calling convention. It can change arguments before forwarding a call, modify a returned message, or update game data before normal drawing continues. Saving the original pointer lets the hook call the API without recursively calling itself.

This redirects calls through the patched entry; it does not automatically intercept calls from every other module.

### Other techniques

The color example patches instructions near an API's entry instead of replacing an IAT pointer. The No-Lose Minesweeper example changes the executable on disk. Their implementation and compatibility requirements differ from the IAT examples.

## Individual examples

### Calculator keypad

`SetWindowTextW` handles control text and `DrawTextW` handles drawn labels. Both hooks copy the supplied text and exchange the selected digits before calling the original API. This changes labels and display text; button commands and arithmetic are not remapped.

### Calculator text colors

A five-byte jump immediately before `gdi32!SetTextColor` and a short backward jump at its entry redirect execution to an assembly handler. The handler replaces the color argument with a random 24-bit value, then continues past the patched entry instruction. A new color is selected on each intercepted call.

### Reversed Calculator text

The `SetWindowTextW` hook copies the requested string into a `std::wstring`, reverses it, and calls the original function. For example, `7890` displays as `0987`. It reverses the entire string, including signs and punctuation, without directly changing the calculation.

### Peter Pan Notepad

The hook first retrieves a message through the original `GetMessageW`. For character messages directed at Notepad's edit control, it substitutes the next character read from `PeterPan.txt`. Notepad then processes the modified message normally, so typing produces successive characters from the book.

### Minesweeper auto-flag

Before calling the original `UpdateWindow`, the hook scans a fixed memory region used for the board. It changes unflagged mine values (`0x8F`) into flagged mine values (`0x8E`). This changes board state so mines appear flagged; the player still reveals the safe cells. The fixed scan is specific to this example.

### Minesweeper No-Lose modification

The original game protects the first move by relocating a mine when necessary. The patch removes the turn check so mine relocation can also run on later mine clicks. Replacing the comparison and conditional jump with `NOP` instructions changes seven bytes. Launch `winmineModified.exe` directly; no injector is required.

### Solitaire

The single hook source combines six API hooks:

| Function | Purpose |
| --- | --- |
| `cards.dll!cdtDrawExt` | Intercept card drawing for custom layouts. |
| `gdi32!CreateSolidBrush` | Replace the initial green table brush. |
| `user32!DrawTextW` | Replace the score label while preserving the number. |
| `gdi32!GetTextExtentPoint32W` | Measure the replacement label correctly. |
| `user32!GetMessageW` | Handle shortcuts and label-editor input. |
| `user32!EndPaint` | Draw layout effects, Peek Mode, and hint borders after repainting. |

Solitaire substitutes a card-back image before calling its drawing API for hidden cards. Peek Mode therefore reads the actual card records from the internal piles and draws their faces without changing their face-up flags. Hints inspect top face-up cards and apply foundation and column rules to identify a supported move.

Drawing overlays after the normal repaint avoids clipping that would otherwise hide shifted cards. Table-color changes also update the game's cached brush and background color.

| Key | Action |
| --- | --- |
| **L** | Cycle through lowered-card, staircase, and normal layouts. |
| **P** | Toggle a face-up preview of tableau cards. |
| **H** | Highlight a legal single-card move: yellow source, cyan destination. |
| **C** | Choose a random table color. |
| **S** | Toggle the score label between `Nice!` and `Score`. |
| **E** | Open the custom label editor; Enter saves and Esc cancels. |

Use the normal layout when playing: cosmetic layouts do not move clickable regions. Peek Mode blocks mouse actions during the preview. Hints do not search whole sequences or stock actions, and no hint does not mean the game is lost. This is a gameplay aid, not an automatic solver.

## Build and run

Run from the example's folder, passing both the executable path and the DLL path. For the color demo:

```powershell
.\injector.exe .\calc.exe ColorDll.dll
```

For the tracked Solitaire example, from the repository root:

```powershell
cd .\SolitaireHook
.\injector.exe .\sol.exe SolitaireHookDll.dll
```

Keep `cards.dll` beside `sol.exe`. Launch Peter Pan from its folder so `PeterPan.txt` can be found. Close the target before replacing its loaded DLL, rebuild, and launch again to test source changes.

