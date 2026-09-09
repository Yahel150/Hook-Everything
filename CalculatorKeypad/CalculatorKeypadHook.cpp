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
typedef BOOL(WINAPI* HOOK_TYPE1)(HWND, LPCWSTR); 

typedef BOOL(WINAPI* HOOK_TYPE2)(HDC, LPCTSTR, int, LPRECT, UINT);

HOOK_TYPE1 orig_func1;
HOOK_TYPE2 orig_func2;



//Write the hook function here
BOOL WINAPI setWindowTextHook(HWND hWnd, LPCWSTR lpString) {
    wstring tmp_string(lpString);
	for (int i = 0; i < tmp_string.length(); i++) {
		if (tmp_string[i] == L'1') {
			tmp_string[i] = L'7';
		}
        else if (tmp_string[i] == L'2') {
            tmp_string[i] = L'8';
        }
        else if (tmp_string[i] == L'3') {
            tmp_string[i] = L'9';
        }
        else if (tmp_string[i] == L'7') {
            tmp_string[i] = L'1';
        }
        else if (tmp_string[i] == L'8') {
            tmp_string[i] = L'2';
        }
        else if (tmp_string[i] == L'9') {
            tmp_string[i] = L'3';
        }
	}
    
    return orig_func1(hWnd, tmp_string.c_str());

}


INT WINAPI drawTextHook(HDC     hdc,
    LPCTSTR lpchText,
    int     cchText,
    LPRECT  lprc,
      UINT    format) {

    log_file << " in drawTextHook"<<endl;
    wstring tmp_string(lpchText);
    for (int i = 0; i < tmp_string.length(); i++) {
        if (tmp_string[i] == L'1') {
            tmp_string[i] = L'7';
        }
        else if (tmp_string[i] == L'2') {
            tmp_string[i] = L'8';
        }
        else if (tmp_string[i] == L'3') {
            tmp_string[i] = L'9';
        }
        else if (tmp_string[i] == L'7') {
            tmp_string[i] = L'1';
        }
        else if (tmp_string[i] == L'8') {
            tmp_string[i] = L'2';
        }
        else if (tmp_string[i] == L'9') {
            tmp_string[i] = L'3';
        }
    }

    return orig_func2(hdc, tmp_string.c_str(), cchText, lprc, format);

}


void setHook() {
    log_file << "dll attached" << endl;
    HMODULE curr_prog = GetModuleHandle(NULL); 

    // Change this to the DLL containing the function.
    HMODULE target_dll = GetModuleHandle(L"user32.dll"); //Change!
    DWORD lpProtect = 0;
    LPVOID JumpTo;
    LPDWORD IAT_ADDRESS1;
    LPDWORD IAT_ADDRESS2;


    if ((curr_prog == NULL) || (target_dll == NULL)) {
        log_file << "couldnt get handles";
        return;
    }

    // Change SetWindowTextW to the function we want to hook
    orig_func1 = (HOOK_TYPE1)GetProcAddress(target_dll, "SetWindowTextW"); //Change!
    if (orig_func1 == NULL) {
        log_file << "couldnt find function" << endl;
        return;
    };

    
    int addr_beginning_of_our_exe1 = 0x1000000; // Change to the image base 
	int addr_func_to_hook_in_IAT1 = 0x1001110; // Change to the IAT entry address of the function we want to hook
    DWORD IAT_Func_Offset1 = addr_func_to_hook_in_IAT1-addr_beginning_of_our_exe1;
    IAT_ADDRESS1 = (LPDWORD)(curr_prog + IAT_Func_Offset1 / 4);

    if ((*IAT_ADDRESS1) != (DWORD)orig_func1) {
        log_file << "IAT contents does not match - maybe check the offset again" << endl;
        return;
    }

    log_file << "changing IAT entry" << endl << "IAT address: " << IAT_ADDRESS1 << endl;
    JumpTo = (LPVOID)((char*)&setWindowTextHook);
    VirtualProtect(IAT_ADDRESS1, 0x4, PAGE_EXECUTE_READWRITE, &lpProtect);
    memcpy(IAT_ADDRESS1, &JumpTo, 0x4);
    VirtualProtect(IAT_ADDRESS1, 0x4, lpProtect, &lpProtect);



    orig_func2 = (HOOK_TYPE2)GetProcAddress(target_dll, "DrawTextW"); //Change!
    if (orig_func2 == NULL) {
        log_file << "couldnt find function" << endl;
        return;
    };


    int addr_beginning_of_our_exe2 = 0x1000000; // Change to the image base 
    int addr_func_to_hook_in_IAT2 = 0x10010BC; // Change to the IAT entry address of the function we want to hook
    DWORD IAT_Func_Offset2 = addr_func_to_hook_in_IAT2 - addr_beginning_of_our_exe2;
    IAT_ADDRESS2 = (LPDWORD)(curr_prog + IAT_Func_Offset2 / 4);


    if ((*IAT_ADDRESS2) != (DWORD)orig_func2) {
        log_file << "IAT contents does not match - maybe check the offset again" << endl;
        return;
    }

    log_file << "changing IAT entry" << endl << "IAT address: " << IAT_ADDRESS2 << endl;
    JumpTo = (LPVOID)((char*)&drawTextHook);
    VirtualProtect(IAT_ADDRESS2, 0x4, PAGE_EXECUTE_READWRITE, &lpProtect);
    memcpy(IAT_ADDRESS2, &JumpTo, 0x4);
    VirtualProtect(IAT_ADDRESS2, 0x4, lpProtect, &lpProtect);


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

