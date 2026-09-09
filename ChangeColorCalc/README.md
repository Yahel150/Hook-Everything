# Calculator Text Colors

A DLL hook that replaces Calculator's requested text colors with randomly generated colors.

https://github.com/user-attachments/assets/f970e23e-881a-48fa-a4f3-00d3ef56a8d2


## How It Works

### 1. Loading the DLL

The injector starts Calculator suspended and loads `ColorDll.dll` into its process.

When the DLL loads, `DllMain` calls `setHook()`, which locates `SetTextColor` in `gdi32.dll`. This Windows function sets the text color for a device context used for drawing.

### 2. Redirecting SetTextColor

The hook patches instructions around the function's entry point using an x86 hotpatch layout:

1. A five-byte jump is written immediately before `SetTextColor`, pointing to `textColorHook`.
2. The first two bytes of `SetTextColor` are replaced with a short backward jump to that five-byte jump.
3. The original memory protection is restored after patching.

Calls to `SetTextColor` now follow this path:

```text
SetTextColor → short backward jump → jump to textColorHook
```

The hook saves `SetTextColor + 2` in `back_addr` so execution can later continue past the patched entry instructions.

### 3. Generating a Random Color

`getRandomint()` generates an integer between `0x000000` and `0xFFFFFF`, covering the 24-bit color range.

It uses a Mersenne Twister generator seeded through `std::random_device`. The generator is initialized once and reused for subsequent calls.

### 4. Replacing the Color Argument

`textColorHook` is a naked assembly function, so the compiler does not insert a normal function prologue or epilogue.

After calling `getRandomint()`, the generated value is returned in `EAX`. The hook writes it to `[ESP + 0x8]`, the stack location of the color argument in this x86 calling convention.

It then jumps to `back_addr`, allowing the original `SetTextColor` implementation to continue with the replacement color.

A new color is chosen on each intercepted call. Visible changes depend on when Calculator sets its text color and redraws its interface.

## Run

From the `ChangeColorCalc` folder:

```powershell
.\injector.exe .\calc.exe "$PWD\ColorDll.dll"
```

## Files

- `calc.exe`: target Calculator.
- `injector.exe`: loads the hook DLL.
- `Color.cpp`: hook source.
- `ColorDll.dll`: compiled hook.
- `ColorDll/`: Visual Studio DLL project.
- `log.txt`: hook initialization log.

## Compatibility

The hook requires 32-bit x86 code and assumes that `SetTextColor` has a compatible hotpatch layout: five available bytes before its entry and a two-byte entry instruction that can be skipped.

The implementation does not verify this layout before patching, so compatibility depends on the Windows version and target environment. Build the DLL using the Win32 configuration and rebuild after source changes.
