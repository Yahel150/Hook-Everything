// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <Windows.h>
#include <stdio.h>
#include <TlHelp32.h>
#include <Shlwapi.h>
#include <fstream>
#include <iostream>
#include <ios>
#include <string>
#include <cstdlib>
#include <ctime>
#include <random>

using std::ofstream;
using std::endl;
using std::wstring;

ofstream log_file("log.txt");
LPVOID back_addr;


extern "C" __declspec(dllexport) int getRandomint() {
    static std::random_device rd;
    static std::mt19937 gen(rd());  // Hardware-seeded Mersenne Twister
    static std::uniform_int_distribution<int> dist(0x000000, 0xFFFFFF);
    return dist(gen);
}
__declspec(naked) void textColorHook() {
    __asm {
        call getRandomint ; our function for getting a random int in our range
        mov[esp + 0x8], eax; COLORREF
        jmp back_addr; Jump to the real SetTextColor
    }
}

void setHook() {

    LPVOID f;
    HMODULE h = GetModuleHandle(L"gdi32.dll");
    CHAR JmpOpcode[6] = "\xE9\x90\x90\x90\x90";
    CHAR JmpOpcode2[8] = "\xB8\x90\x90\x90\x90\xFF\xE0";
    DWORD lpProtect = 0;
    LPVOID JumpTo;

    if (h == NULL) {
        log_file << "ColorDll: couldnt get handle" << endl;
        return;
    }
    f = GetProcAddress(h, "SetTextColor");

    if (f == NULL) {
        log_file << "ColorDll: couldnt get function" << endl;
        return;
    }
    log_file << "ColorDll: setting hook" << endl;
    // calculate relative jump to setTextHook from f	
    JumpTo = (LPVOID)((char*)&textColorHook - (char*)f);
    memcpy(JmpOpcode + 1, &JumpTo, 0x4); // write the jump 	
    VirtualProtect((char*)f - 5, 0x7, PAGE_EXECUTE_READWRITE, &lpProtect);
    memcpy((char*)f - 5, &JmpOpcode, 0x5); // override the first two bytes with jmp short 
    *(char*)f = 0xEB;
    *((char*)(f)+1) = 0xf9;

    VirtualProtect((char*)f - 5, 0x7, PAGE_EXECUTE_READ, &lpProtect);
    // jump to f from setTextHook 
    back_addr = (LPVOID)((char*)f + 2);
    log_file << "hook set up succesfull" << endl;
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
