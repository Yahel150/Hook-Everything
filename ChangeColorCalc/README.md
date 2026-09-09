# Calculator Text Colors

Color.cpp hooks gdi32!SetTextColor to substitute a random text color. Includes Calculator, an injector, a prebuilt DLL, and a Visual Studio project in ColorDll.

The hook uses x86 assembly and assumes a compatible hotpatch layout in the target function. Build the DLL using Win32.
