# Calculator Keypad Hook

A DLL hook that swaps Calculator's displayed digits, changing both keypad labels and display text.

## How It Works

### 1. Choosing the Functions

The hook intercepts two functions from `user32.dll`:

- `SetWindowTextW`: updates window or control text, including Calculator's display.
- `DrawTextW`: draws text, including keypad labels in the target Calculator.

During debugging, a breakpoint on `DrawTextW` showed the keypad labels being drawn individually. Its `lpchText` parameter contains the text to modify.

Hooking both functions applies the same digit replacement to the display and keypad labels.

### 2. Installing the Hooks

The injector loads `CalculatorKeypadHook.dll` into Calculator. When the DLL loads, `DllMain` calls `setHook()`.

The function saves the original API addresses and replaces their entries in Calculator's Import Address Table (IAT):

| Function | IAT offset from the executable's base |
| --- | --- |
| `SetWindowTextW` | `0x1110` |
| `DrawTextW` | `0x10BC` |

Before replacing each entry, the code checks that it contains the expected function address. It then temporarily makes the memory writable, installs the hook address, and restores the original protection.

### 3. Swapping the Digits

Both hook functions copy the supplied text into a `std::wstring` and apply these replacements:

| Original digit | Replacement |
| --- | --- |
| `1` | `7` |
| `2` | `8` |
| `3` | `9` |
| `7` | `1` |
| `8` | `2` |
| `9` | `3` |

Digits `0`, `4`, `5`, and `6`, along with other characters, remain unchanged.

Each character passes through an `if`/`else if` chain, so a replacement is performed only once. For example, `1` becomes `7` without immediately being changed back.

### 4. Drawing the Modified Text

`setWindowTextHook` passes the modified string and original window handle to `SetWindowTextW`.

`drawTextHook` passes the modified string to `DrawTextW`, preserving the original drawing context, text length, rectangle, and formatting flags.

This visually exchanges the keypad's `1 2 3` and `7 8 9` rows. It changes text only: button commands, keyboard input handling, and Calculator's arithmetic are not remapped.

## Run

With a compatible `calc.exe`, an injector, and the compiled DLL in the same folder:

```powershell
.\injector.exe .\calc.exe "$PWD\CalculatorKeypadHook.dll"
```

## Source and Compatibility

`CalculatorKeypadHook.cpp` contains both hooks and their installation logic. It writes initialization messages and `DrawTextW` hook activity to `log.txt`.

The implementation assumes a specific 32-bit Calculator executable and hardcoded IAT offsets. Other versions may require different offsets.

Build the DLL with Unicode enabled. The source references `pch.h`, but this folder does not include a complete build project.
