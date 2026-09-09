// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <windows.h>
#include <stdio.h>
#include <tlhelp32.h>
#include <Shlwapi.h>
#include <fstream>
#include <iostream>
#include <ios>
#include <string>
#include <algorithm>

using namespace std;
using std::ofstream;
using std::endl;
using std::wstring;

//use the log_file its helpful for debugging
ofstream log_file("log.txt");


// Change this to the proper function signature, as shown in the Microsoft documentation.
typedef BOOL(WINAPI* HOOK_TYPE)(HWND, LPCWSTR); 

HOOK_TYPE orig_func;


//Write the hook function here
BOOL WINAPI funcHook(HWND hWnd, LPCWSTR lpString) {
    wstring tmp_string(lpString);
    std::reverse(tmp_string.begin(), tmp_string.end());
    
    return orig_func(hWnd, tmp_string.c_str());

}



void setHook() {
    log_file << "dll attached" << endl;
    HMODULE curr_prog = GetModuleHandle(NULL); 

    // Change this to the DLL containing the function.
    HMODULE target_dll = GetModuleHandle(L"user32.dll"); //Change!
    DWORD lpProtect = 0;
    LPVOID JumpTo;
    LPDWORD IAT_ADDRESS;

    if ((curr_prog == NULL) || (target_dll == NULL)) {
        log_file << "couldnt get handles";
        return;
    }

    // Change SetWindowTextW to the function we want to hook
    orig_func = (HOOK_TYPE)GetProcAddress(target_dll, "SetWindowTextW"); //Change!
    if (orig_func == NULL) {
        log_file << "couldnt find function" << endl;
        return;
    };

    
    int addr_beginning_of_our_exe = 0x400000; // Change to the image base 
	int addr_func_to_hook_in_IAT = 0x40A2AC; // Change to the IAT entry address of the function we want to hook
    DWORD IAT_Func_Offset = addr_func_to_hook_in_IAT-addr_beginning_of_our_exe;
    IAT_ADDRESS = (LPDWORD)(curr_prog + IAT_Func_Offset / 4);

    if ((*IAT_ADDRESS) != (DWORD)orig_func) {
        log_file << "IAT contents does not match - maybe check the offset again" << endl;
        return;
    }

    log_file << "changing IAT entry" << endl << "IAT address: " << IAT_ADDRESS << endl;
    JumpTo = (LPVOID)((char*)&funcHook);
    VirtualProtect(IAT_ADDRESS, 0x4, PAGE_EXECUTE_READWRITE, &lpProtect);
    memcpy(IAT_ADDRESS, &JumpTo, 0x4);
    VirtualProtect(IAT_ADDRESS, 0x4, lpProtect, &lpProtect);


}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        setHook();
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

