# Reverse Calculator Text Hook

A DLL hook that reverses text displayed by Calculator. For example, entering `7890` displays `0987`.


https://github.com/user-attachments/assets/cfe346f6-8243-45f7-b003-066a1526ebee


## How It Works

### 1. Finding the Target Function

Calculator uses `SetWindowTextW` from `user32.dll` to update text in a window or control. Its `lpString` parameter contains the text to display.

Using IDA, we locate this function in Calculator's Import Address Table (IAT), which stores addresses of imported functions.

The current source calculates the entry's offset as:

```text
IAT entry address - image base
0x40A2AC - 0x400000 = 0xA2AC
```

This offset is specific to the executable being analyzed.

### 2. Installing the Hook

The injector starts Calculator suspended and loads `reverseText.dll` into its process. When the DLL loads, `DllMain` calls `setHook()`.

This function:

1. Gets Calculator's loaded base address and the handle to `user32.dll`.
2. Saves the original address of `SetWindowTextW`.
3. Locates the IAT entry using the calculated offset.
4. Verifies that the entry contains the expected function address.
5. Temporarily makes the entry writable, replaces it with the address of `funcHook`, and restores the original memory protection.

Calls through the patched IAT entry now reach `funcHook`.

### 3. Reversing the Text

The hook copies `lpString` into a `std::wstring` and reverses it using `std::reverse`.

It then calls the original `SetWindowTextW` with the same window handle and the reversed string:

```text
Calculator → funcHook → reverse text → original SetWindowTextW
```

The hook changes the displayed text without directly modifying Calculator's arithmetic. It reverses the entire string, including punctuation and signs, and does not filter calls by window or control.

## Run

From the `ReverseTextCalc` folder:

```powershell
.\injector.exe .\calc.exe reverseText.dll
```

## Files

- `calc.exe`: target Calculator.
- `injector.exe`: loads the hook DLL.
- `reverseText.cpp`: hook source.
- `reverseText.dll`: compiled hook.
- `log.txt`: hook initialization log.

## Compatibility

The hook assumes a 32-bit executable and a specific IAT layout.

