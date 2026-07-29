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
#include <sstream>


using std::ofstream;
using std::endl;
using std::wstring;

ofstream log_file("log.txt");


std::fstream in("PeterPan.txt", std::ios::in);

// change this to the proper signature of the hook
typedef BOOL(WINAPI* HOOK_TYPE)(LPMSG, HWND, UINT, UINT);

HOOK_TYPE orig_GetMessageW;


BOOL WINAPI GetMessageWHook(LPMSG lpMsg, HWND hWnd, UINT  wMsgFilterMin, UINT  wMsgFilterMax) {
    const BOOL result = orig_GetMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
    if (result > 0 && lpMsg != nullptr && lpMsg->message == WM_CHAR) {
        //creating edit window in 10043A8 with id = 0x0f
        HWND parent = GetParent(lpMsg->hwnd);
        if (lpMsg->hwnd == GetDlgItem(parent, 0x0F)) {
            //nextchar
            char nextCharacter = in.get();
            if (nextCharacter != EOF)
                lpMsg->wParam = (WPARAM)(nextCharacter);
        }
    }
    return result;

}



void setHook() {

    log_file << "dll attached" << endl;


    HMODULE curr_prog = GetModuleHandle(NULL); // get the handle of the main process
    HMODULE target_dll = GetModuleHandle(L"user32.dll"); // get the handle of the dll conatining the function we want to override
    DWORD lpProtect = 0;
    DWORD IAT_Func_Offset = 0x1280;
    LPVOID JumpTo;
    LPDWORD IAT_ADDRESS;

    if ((curr_prog == NULL) || (target_dll == NULL)) {
        log_file << "couldnt get handles";
        return;
    }

    orig_GetMessageW = (HOOK_TYPE)GetProcAddress(target_dll, "GetMessageW");
    if (orig_GetMessageW == NULL) {
        log_file << "couldnt find function" << endl;
        return;
    };
    IAT_ADDRESS = (LPDWORD)(curr_prog + IAT_Func_Offset / 4);
    if ((*IAT_ADDRESS) != (DWORD)orig_GetMessageW) {
        log_file << "IAT contents does not match - maybe check the offset again" << endl;
        return;
    }

    log_file << "changing IAT entry" << endl << "IAT address: " << IAT_ADDRESS << endl;
    JumpTo = (LPVOID)((char*)&GetMessageWHook);
    VirtualProtect(IAT_ADDRESS, 0x4, PAGE_EXECUTE_READWRITE, &lpProtect);
    memcpy(IAT_ADDRESS, &JumpTo, 0x4);
    VirtualProtect(IAT_ADDRESS, 0x4, PAGE_EXECUTE_READWRITE, &lpProtect);


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
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

