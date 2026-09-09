# Hook Everything

Windows C++ demos that modify Calculator, Notepad, and Minesweeper using DLL injection, API hooking, and executable modification.

| Folder | Demo |
| --- | --- |
| [CalculatorKeypad](CalculatorKeypad/README.md) | Swap displayed digits: 1↔7, 2↔8, 3↔9. |
| [ChangeColorCalc](ChangeColorCalc/README.md) | Randomize text colors through SetTextColor. |
| [ChangeColorsInCalc](ChangeColorsInCalc/README.md) | Calculator executable only. |
| [PeterPan](PeterPan/readme.md) | Replace typed Notepad characters with text from Peter Pan. |
| [ReverseTextCalc](ReverseTextCalc/README.md) | Reverse text passed to SetWindowTextW. |
| [winmine](winmine/readme.md) | Automatically flag mines by modifying board memory. |
| [Winmine-NoLoseMod](Winmine-NoLoseMod/README.md) | Original and modified Minesweeper executables. |

## How it works

The injectors start a target process suspended and load a hook DLL through a remote thread calling LoadLibraryA. The DLL installs its hook, then the injector resumes the target. Most demos replace an Import Address Table (IAT) entry; the color demo patches instructions around SetTextColor directly.

waitInjector.cpp waits for the DLL-loading thread before resuming the target. Its command-line format is:

```text
waitInjector.exe <path-to-target.exe> <absolute-path-to-hook.dll>
```

For PeterPan, launch from its folder so the DLL can find PeterPan.txt.


