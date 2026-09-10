// Build as a Win32 DLL in the same Visual Studio template as the other hooks.
#include "pch.h"
#include <windows.h>
#include <string>
#include <stddef.h>

using namespace std;

COLORREF tableColor = RGB(80, 40, 120); // Purple. Change these three numbers.
BYTE* program;
bool peek = false;
bool showHint = false;
wstring scoreLabel = L"Nice!";
HWND labelEditor = nullptr;
HWND labelEdit = nullptr;
unsigned colorSeed = 1;
int layout = 1; // 0 = normal, 1 = original exercise, 2 = staircase. Press L.

// These structures describe THIS sol.exe's memory, not a general Solitaire API.
struct Card {
    WORD value; // Low 15 bits: rank * 4 + suit. Bit 0x8000: face-up.
    WORD padding;
    int x, y;
};

struct Pile {
    DWORD kind, handler;
    RECT area;
    DWORD selection;
    int count, capacity;
    Card cards[1]; // The game allocates more records after this first one.
};

struct Game {
    BYTE unused[0x64];
    int pileCount, capacity;
    Pile* piles[13]; // Stock, waste, four foundations, seven columns.
};

static_assert(sizeof(void*) == 4, "Build this DLL as Win32, not x64.");
static_assert(sizeof(Card) == 12 && offsetof(Pile, cards) == 0x24,
    "The structures must match sol.exe.");
static_assert(offsetof(Game, piles) == 0x6C, "Incorrect game layout.");

Game* getGame() { return *(Game**)(program + 0x7170); }
HWND gameWindow() { return *(HWND*)(program + 0x733C); }

Card* cardAt(Pile* pile, int index) {
    return (Card*)((BYTE*)pile + 0x24 + index * sizeof(Card));
}

Card* topCard(Pile* pile) {
    return pile->count ? cardAt(pile, pile->count - 1) : nullptr;
}

void moveDrawing(int x, int& y, int type) {
    if (layout == 1 && type == 0 && x == 11 && y == 107) y += 100;
    if (layout == 2 && type <= 1 && y >= 107) y += (x - 11) / 8;
}

bool fitsFoundation(WORD value, Pile* foundation) {
    int card = value & 0x7FFF;
    Card* top = topCard(foundation);
    if (!top) return card / 4 == 0; // An Ace starts an empty foundation.
    int target = top->value & 0x7FFF;
    return card % 4 == target % 4 && card / 4 == target / 4 + 1;
}

bool fitsColumn(WORD value, Pile* pile) {
    int card = value & 0x7FFF;
    Card* top = topCard(pile);
    if (!top) return card / 4 == 12; // Only a King can enter an empty column.
    int target = top->value & 0x7FFF;
    int suits = (card % 4) ^ (target % 4);
    return (top->value & 0x8000) && (suits == 1 || suits == 2)
        && card / 4 + 1 == target / 4;
}

bool findHint(Game* game, Card*& source, Pile*& destination) {
    if (!game) return false;
    for (int p = 1; p < game->pileCount; ++p) {
        if (p >= 2 && p <= 5) continue; // Do not move between foundations.
        Card* card = topCard(game->piles[p]);
        if (!card || !(card->value & 0x8000)) continue;
        for (int f = 2; f <= 5; ++f) {
            if (fitsFoundation(card->value, game->piles[f])) {
                source = card;
                destination = game->piles[f];
                return true;
            }
        }
        // Keep this small: suggest single top-card moves, not whole sequences.
        for (int c = 6; c < game->pileCount; ++c) {
            if (c != p && fitsColumn(card->value, game->piles[c])) {
                source = card;
                destination = game->piles[c];
                return true;
            }
        }
    }
    return false;
}

typedef BOOL(WINAPI* HOOK_TYPE1)(HDC, INT, INT, INT, INT, INT, INT, COLORREF);
typedef HBRUSH(WINAPI* HOOK_TYPE2)(COLORREF);
typedef INT(WINAPI* HOOK_TYPE3)(HDC, LPCWSTR, INT, LPRECT, UINT);

HOOK_TYPE1 orig_func1;
HOOK_TYPE2 orig_func2;
HOOK_TYPE3 orig_func3;
typedef BOOL(WINAPI* MESSAGE_HOOK)(LPMSG, HWND, UINT, UINT);
typedef BOOL(WINAPI* PAINT_HOOK)(HWND, const PAINTSTRUCT*);
MESSAGE_HOOK orig_GetMessageW;
PAINT_HOOK orig_EndPaint;
typedef BOOL(WINAPI* SIZE_HOOK)(HDC, LPCWSTR, INT, LPSIZE);
SIZE_HOOK orig_GetTextExtentPoint32W;

BOOL WINAPI cardHook(HDC hdc, INT x, INT y, INT dx, INT dy,
    INT card, INT type, COLORREF color) {
    int movedY = y;
    moveDrawing(x, movedY, type);
    // Draw shifted cards later, outside the game's per-card clipping region.
    if (movedY != y) return TRUE;

    // Match the background used by the card renderer to the table brush.
    if (color == RGB(0, 128, 0))
        color = tableColor;

    return orig_func1(hdc, x, y, dx, dy, card, type, color);
}

void drawColumns(HDC hdc, Game* game) {
    int width = *(int*)(program + 0x7308);
    int height = *(int*)(program + 0x730C);
    for (int p = 6; p < game->pileCount; ++p) {
        Pile* pile = game->piles[p];
        for (int i = 0; i < pile->count; ++i) {
            Card* card = cardAt(pile, i);
            int type = (card->value & 0x8000) ? 0 : 1;
            int y = card->y;
            if (peek) y = pile->area.top + i * 20; // Expose rank/suit corners.
            else moveDrawing(card->x, y, type);
            if (peek || y != card->y)
                orig_func1(hdc, card->x, y, width, height,
                    (peek || type == 0) ? card->value & 0x7FFF : *(int*)(program + 0x7008),
                    peek ? 0 : type, tableColor);
        }
    }
}

HBRUSH WINAPI colorHook(COLORREF color) {
    if (color == RGB(0, 128, 0))
        color = tableColor;
    return orig_func2(color);
}

INT WINAPI scoreHook(HDC hdc, LPCWSTR text, INT count, LPRECT rect, UINT format) {
    // This executable passes explicit character counts to DrawTextW.
    wstring message(text, count);
    // Replace only the score label; keep the numeric score and time intact.
    if (message == L"Score: ")
        message = scoreLabel + L": ";
    return orig_func3(hdc, message.c_str(), (INT)message.length(), rect, format);
}

BOOL WINAPI textSizeHook(HDC hdc, LPCWSTR text, INT count, LPSIZE size) {
    wstring message(text, count);
    if (message == L"Score: ") message = scoreLabel + L": ";
    return orig_GetTextExtentPoint32W(hdc, message.c_str(), (INT)message.length(), size);
}

void outline(HDC hdc, int x, int y, COLORREF color) {
    int width = *(int*)(program + 0x7308);
    int height = *(int*)(program + 0x730C);
    RECT rect = { x, y, x + width, y + height };
    HBRUSH brush = CreateSolidBrush(color);
    for (int i = 0; i < 3; ++i) {
        FrameRect(hdc, &rect, brush);
        InflateRect(&rect, -1, -1);
    }
    DeleteObject(brush);
}

BOOL WINAPI paintHook(HWND hwnd, const PAINTSTRUCT* paint) {
    BOOL result = orig_EndPaint(hwnd, paint);
    Game* game = getGame();
    if (hwnd != gameWindow() || !game) return result;
    HDC hdc = GetDC(hwnd);
    if (peek) {
        RECT area;
        GetClientRect(hwnd, &area);
        area.top = game->piles[6]->area.top;
        area.bottom -= 20; // Keep the score/time strip visible.
        HBRUSH brush = CreateSolidBrush(tableColor);
        FillRect(hdc, &area, brush);
        DeleteObject(brush);
    }
    drawColumns(hdc, game);
    if (showHint && !peek) {
        Card* source = nullptr;
        Pile* destination = nullptr;
        if (findHint(game, source, destination)) {
            int y = source->y;
            moveDrawing(source->x, y, 0);
            outline(hdc, source->x, y, RGB(255, 220, 0));
            Card* target = topCard(destination);
            int x = target ? target->x : destination->area.left;
            int targetY = target ? target->y : destination->area.top;
            moveDrawing(x, targetY, 0);
            outline(hdc, x, targetY, RGB(0, 220, 255));
        }
    }
    ReleaseDC(hwnd, hdc);
    return result;
}

void refresh() {
    InvalidateRect(gameWindow(), nullptr, TRUE);
}

void changeColor() {
    // A tiny private random generator: it does not change Solitaire's shuffle.
    colorSeed = colorSeed * 1664525u + 1013904223u;
    tableColor = RGB(32 + (colorSeed & 127), 32 + ((colorSeed >> 8) & 127),
        32 + ((colorSeed >> 16) & 127));
    HBRUSH oldBrush = *(HBRUSH*)(program + 0x737C);
    HBRUSH newBrush = CreateSolidBrush(tableColor);
    *(COLORREF*)(program + 0x7348) = tableColor;
    *(HBRUSH*)(program + 0x737C) = newBrush;
    SetClassLongPtrW(gameWindow(), GCLP_HBRBACKGROUND, (LONG_PTR)newBrush);
    DeleteObject(oldBrush);
    refresh();
}

LRESULT CALLBACK labelWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_COMMAND && (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)) {
        if (LOWORD(wParam) == IDOK) {
            wchar_t text[13];
            GetWindowTextW(labelEdit, text, 13);
            scoreLabel = text;
            refresh();
        }
        DestroyWindow(hwnd);
        return 0;
    }
    if (message == WM_DESTROY) {
        labelEditor = nullptr;
        labelEdit = nullptr;
        EnableWindow(gameWindow(), TRUE);
        SetForegroundWindow(gameWindow());
        return 0;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

void editScoreLabel() {
    HINSTANCE instance = GetModuleHandle(nullptr);
    WNDCLASSW wc = {};
    wc.lpfnWndProc = labelWindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = L"SolitaireScoreLabel";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    RegisterClassW(&wc);
    RECT parent;
    GetWindowRect(gameWindow(), &parent);
    labelEditor = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT,
        wc.lpszClassName, L"Change score label", WS_CAPTION | WS_SYSMENU,
        parent.left + 50, parent.top + 100, 390, 180, gameWindow(), nullptr, instance, nullptr);
    CreateWindowW(L"STATIC", L"Type your score label (up to 12 characters):",
        WS_CHILD | WS_VISIBLE, 16, 15, 355, 22, labelEditor, nullptr, instance, nullptr);
    labelEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", scoreLabel.c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        16, 43, 345, 26, labelEditor, (HMENU)100, instance, nullptr);
    SendMessageW(labelEdit, EM_SETLIMITTEXT, 12, 0);
    SendMessageW(labelEdit, EM_SETSEL, 0, -1);
    CreateWindowW(L"BUTTON", L"Save", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
        175, 88, 88, 28, labelEditor, (HMENU)IDOK, instance, nullptr);
    CreateWindowW(L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        273, 88, 88, 28, labelEditor, (HMENU)IDCANCEL, instance, nullptr);
    EnableWindow(gameWindow(), FALSE);
    ShowWindow(labelEditor, SW_SHOW);
    SetFocus(labelEdit);
}

BOOL WINAPI messageHook(LPMSG msg, HWND hwnd, UINT first, UINT last) {
    BOOL result = orig_GetMessageW(msg, hwnd, first, last);
    if (result <= 0) return result; // Preserve WM_QUIT and GetMessage errors.
    HWND main = gameWindow();
    // Route the editor's input before Solitaire can treat letters as shortcuts.
    if (labelEditor && (msg->hwnd == labelEditor || IsChild(labelEditor, msg->hwnd))) {
        if (msg->message == WM_CHAR) DispatchMessageW(msg);
        else if (!IsDialogMessageW(labelEditor, msg)) {
            TranslateMessage(msg);
            DispatchMessageW(msg);
        }
        msg->message = WM_NULL;
        return result;
    }
    if (msg->hwnd != main) return result; // Leave the game's other dialogs alone.

    if (msg->message == WM_KEYDOWN || msg->message == WM_KEYUP) {
        bool down = msg->message == WM_KEYDOWN;
        bool firstPress = down && !(msg->lParam & (1L << 30));
        if (msg->wParam == 'P') {
            if (firstPress) {
                peek = !peek;
                showHint = false;
                SetWindowTextW(main, peek ? L"Solitaire - Peek ON (P to return)" : L"Solitaire");
                refresh();
            }
        } else if (msg->wParam == 'H') {
            if (firstPress) {
                peek = false;
                showHint = true;
                Card* source = nullptr;
                Pile* destination = nullptr;
                SetWindowTextW(main, findHint(getGame(), source, destination)
                    ? L"Solitaire - Hint: yellow card -> cyan destination"
                    : L"Solitaire - No single-card hint: try stock, flipping a card, or a sequence");
                refresh();
            }
        } else if (msg->wParam == 'L') {
            if (firstPress) { layout = (layout + 1) % 3; refresh(); }
        } else if (msg->wParam == 'C') {
            if (firstPress) changeColor();
        } else if (msg->wParam == 'S') {
            if (firstPress) {
                scoreLabel = scoreLabel == L"Nice!" ? L"Score" : L"Nice!";
                refresh();
            }
        } else if (msg->wParam == 'E') {
            if (firstPress) editScoreLabel();
        } else {
            if (msg->wParam == VK_F2) { showHint = false; peek = false; }
            return result;
        }
        msg->message = WM_NULL; // Do not also send our keys to Solitaire.
    } else if (peek && (msg->message == WM_LBUTTONDOWN || msg->message == WM_LBUTTONUP
        || msg->message == WM_LBUTTONDBLCLK || msg->message == WM_RBUTTONDOWN
        || msg->message == WM_RBUTTONUP || msg->message == WM_MOUSEMOVE)) {
        msg->message = WM_NULL; // Press P again before playing.
    } else if (msg->message == WM_LBUTTONDOWN || msg->message == WM_COMMAND) {
        bool clearBorders = showHint;
        showHint = false;
        if (clearBorders) {
            SetWindowTextW(main, L"Solitaire");
            refresh(); // Ordinary clicks no longer force a full-board repaint.
        }
    }
    return result;
}

void setHook1() {
    HMODULE curr_prog = GetModuleHandle(NULL);
    HMODULE target_dll = GetModuleHandleW(L"cards.dll");
    orig_func1 = (HOOK_TYPE1)GetProcAddress(target_dll, "cdtDrawExt");
    LPDWORD IAT_ADDRESS = (LPDWORD)((BYTE*)curr_prog + 0x101C);
    DWORD lpProtect;
    LPVOID JumpTo = (LPVOID)&cardHook;
    VirtualProtect(IAT_ADDRESS, 4, PAGE_READWRITE, &lpProtect);
    memcpy(IAT_ADDRESS, &JumpTo, 4);
    VirtualProtect(IAT_ADDRESS, 4, lpProtect, &lpProtect);
}

void setHook2() {
    HMODULE curr_prog = GetModuleHandle(NULL);
    HMODULE target_dll = GetModuleHandleW(L"gdi32.dll");
    orig_func2 = (HOOK_TYPE2)GetProcAddress(target_dll, "CreateSolidBrush");
    LPDWORD IAT_ADDRESS = (LPDWORD)((BYTE*)curr_prog + 0x1084);
    DWORD lpProtect;
    LPVOID JumpTo = (LPVOID)&colorHook;
    VirtualProtect(IAT_ADDRESS, 4, PAGE_READWRITE, &lpProtect);
    memcpy(IAT_ADDRESS, &JumpTo, 4);
    VirtualProtect(IAT_ADDRESS, 4, lpProtect, &lpProtect);
}

void setHook3() {
    HMODULE curr_prog = GetModuleHandle(NULL);
    HMODULE target_dll = GetModuleHandleW(L"user32.dll");
    orig_func3 = (HOOK_TYPE3)GetProcAddress(target_dll, "DrawTextW");
    LPDWORD IAT_ADDRESS = (LPDWORD)((BYTE*)curr_prog + 0x1190);
    DWORD lpProtect;
    LPVOID JumpTo = (LPVOID)&scoreHook;
    VirtualProtect(IAT_ADDRESS, 4, PAGE_READWRITE, &lpProtect);
    memcpy(IAT_ADDRESS, &JumpTo, 4);
    VirtualProtect(IAT_ADDRESS, 4, lpProtect, &lpProtect);

    // Reserve the actual custom-label width, so Solitaire does not erase it.
    orig_GetTextExtentPoint32W = (SIZE_HOOK)GetProcAddress(GetModuleHandleW(L"gdi32.dll"), "GetTextExtentPoint32W");
    IAT_ADDRESS = (LPDWORD)((BYTE*)curr_prog + 0x1050);
    JumpTo = (LPVOID)&textSizeHook;
    VirtualProtect(IAT_ADDRESS, 4, PAGE_READWRITE, &lpProtect);
    memcpy(IAT_ADDRESS, &JumpTo, 4);
    VirtualProtect(IAT_ADDRESS, 4, lpProtect, &lpProtect);

    // Same IAT template: one keyboard hook shared by Peek, Hints, and Layout.
    orig_GetMessageW = (MESSAGE_HOOK)GetProcAddress(target_dll, "GetMessageW");
    IAT_ADDRESS = (LPDWORD)((BYTE*)curr_prog + 0x1164);
    JumpTo = (LPVOID)&messageHook;
    VirtualProtect(IAT_ADDRESS, 4, PAGE_READWRITE, &lpProtect);
    memcpy(IAT_ADDRESS, &JumpTo, 4);
    VirtualProtect(IAT_ADDRESS, 4, lpProtect, &lpProtect);

    // Draw hint borders after Solitaire finishes painting its board.
    orig_EndPaint = (PAINT_HOOK)GetProcAddress(target_dll, "EndPaint");
    IAT_ADDRESS = (LPDWORD)((BYTE*)curr_prog + 0x10DC);
    JumpTo = (LPVOID)&paintHook;
    VirtualProtect(IAT_ADDRESS, 4, PAGE_READWRITE, &lpProtect);
    memcpy(IAT_ADDRESS, &JumpTo, 4);
    VirtualProtect(IAT_ADDRESS, 4, lpProtect, &lpProtect);
}

BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID) {
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        program = (BYTE*)GetModuleHandle(NULL);
        colorSeed = GetTickCount();
        setHook1();
        setHook2();
        setHook3();
        break;
    }
    return TRUE;
}

