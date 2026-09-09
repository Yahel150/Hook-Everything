# Peter Pan Notepad

PeterPanHook.cpp hooks GetMessageW and replaces character messages for Notepad's edit control with successive characters from PeterPan.txt.

Uses a hardcoded IAT offset and control ID for the bundled 32-bit Notepad. Launch from this folder so the text file can be found. A Visual Studio DLL project is in PeterPanDll.

[Video demo](https://github.com/user-attachments/assets/36da5468-adab-49af-b28e-db64dd837d28)
