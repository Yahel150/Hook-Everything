# Peter Pan Notepad

A DLL hook that changes Notepad so that typing produces successive characters from *Peter Pan*, regardless of the characters you enter.

[Video demo](https://github.com/user-attachments/assets/36da5468-adab-49af-b28e-db64dd837d28)


## How It Works

### 1. Loading the DLL

The injector starts the bundled Notepad executable suspended and loads `PeterPanDll.dll` into its process.

When the DLL loads, Windows calls `DllMain` with `DLL_PROCESS_ATTACH`. The DLL then calls `setHook()` to install the hook.

### 2. Redirecting GetMessageW

Notepad uses `GetMessageW` from `user32.dll` to retrieve messages from its thread's message queue, including character input.

The hook redirects this call by modifying Notepad's Import Address Table (IAT), which stores the addresses of imported functions.

`setHook()`:

1. Gets the base address of Notepad and the handle to `user32.dll`.
2. Saves the original address of `GetMessageW`.
3. Locates its IAT entry at offset `0x1280` from Notepad's base address.
4. Checks that the entry contains the expected function address.
5. Temporarily makes the memory writable, replaces the entry with the address of `GetMessageWHook`, and restores the original protection.

Subsequent calls through this IAT entry reach the hook.

### 3. Intercepting Character Messages

`GetMessageWHook` first calls the original `GetMessageW`, allowing Windows to retrieve a message normally.

The hook modifies the message only when:

- The original call returns a positive result.
- The message is `WM_CHAR`, which represents character input.
- The destination window is the edit control identified by ID `0x0F` under its parent.

This targets Notepad's text area while leaving other messages unchanged.

### 4. Substituting Text from the Book

The DLL opens `PeterPan.txt` as an input stream. For each matching character message, it reads the next character and replaces `lpMsg->wParam`, the field containing the character value.

Notepad then processes the modified message normally and inserts the replacement character.

For example, repeatedly typing `a` produces successive characters from the book rather than a sequence of `a` characters. The file position advances with each intercepted character message.

## Run

Run this command from the `PeterPan` folder:

```powershell
.\injector.exe .\notepad.exe "$PWD\PeterPanDll.dll"
```

Keep `PeterPan.txt` in the working directory so the DLL can open it.

## Files

- `notepad.exe`: target application.
- `injector.exe`: loads the DLL into Notepad.
- `PeterPanHook.cpp`: hook source.
- `PeterPanDll.dll`: compiled hook.
- `PeterPan.txt`: replacement text.
- `PeterPanDll/`: Visual Studio DLL project.
- `log.txt`: hook initialization log.

## Compatibility

The hook assumes a specific 32-bit Notepad executable, IAT offset, and edit-control ID. Other Notepad versions may require different values.

