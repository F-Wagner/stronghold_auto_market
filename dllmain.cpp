// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <tchar.h>
#include <stdio.h>

#define _WIN32_WINNT 0x0600   // or higher, e.g. 0x0601 for Win7, etc.
#include <windows.h>
#include <commctrl.h>
#include <uxtheme.h>         // For SetWindowTheme
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "uxtheme.lib")

// Global instance handle for our DLL
HINSTANCE g_hInstance;

//-------------------------------------------------------------------
// Global definitions for resources and settings
//-------------------------------------------------------------------
#define NUM_RESOURCES 20

// Define the INI file name for saving settings.
#define SETTINGS_FILE _T("settings.ini")

// Autosell thresholds: if the in-game resource count exceeds this value, sell.
unsigned short settingsSell[NUM_RESOURCES] = {
    200,200,200,200,200,200,200,200,200,200,
    200,200,200,200,200,200,200,200,200,200
};

// Autobuy thresholds: if the in-game resource count is below this value, buy.
// Initialize to 0 so nothing is bought by default.
unsigned short settingsBuy[NUM_RESOURCES] = {
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0
};

// Resource IDs corresponding to each material.
const unsigned char MATERIAL_IDS[NUM_RESOURCES] = {
    2u,  3u,  4u,  6u,  7u,  9u,  10u, 11u, 12u, 13u,
    14u, 16u, 17u, 18u, 19u, 20u, 21u, 22u, 23u, 24u
};

// Names for each resource (used in the UI).
const TCHAR* resourceNames[NUM_RESOURCES] = {
    _T("wood"),
    _T("hops"),
    _T("stone"),
    _T("iron"),
    _T("pitch"),
    _T("wheat"),
    _T("bread"),
    _T("cheese"),
    _T("meat"),
    _T("fruit"),
    _T("ale"),
    _T("flour"),
    _T("bows"),
    _T("crossbows"),
    _T("spears"),
    _T("pikes"),
    _T("maces"),
    _T("swords"),
    _T("leather armor"),
    _T("metal armor")
};

//-------------------------------------------------------------------
// Settings Persistence Functions
//-------------------------------------------------------------------
void LoadSettings() {
    // For each resource, load autosell and autobuy thresholds from the INI file.
    for (int i = 0; i < NUM_RESOURCES; i++) {
        TCHAR key[16];
        // Load autosell threshold. Default is 200.
        _stprintf_s(key, _countof(key), _T("Sell%d"), i);
        settingsSell[i] = (unsigned short)GetPrivateProfileInt(_T("Thresholds"), key, 200, SETTINGS_FILE);
        // Load autobuy threshold. Default is 0.
        _stprintf_s(key, _countof(key), _T("Buy%d"), i);
        settingsBuy[i] = (unsigned short)GetPrivateProfileInt(_T("Thresholds"), key, 0, SETTINGS_FILE);
    }
}

void SaveSettings() {
    TCHAR buffer[16];
    TCHAR key[16];
    for (int i = 0; i < NUM_RESOURCES; i++) {
        // Save autosell threshold.
        _stprintf_s(key, _countof(key), _T("Sell%d"), i);
        _stprintf_s(buffer, _countof(buffer), _T("%d"), settingsSell[i]);
        WritePrivateProfileString(_T("Thresholds"), key, buffer, SETTINGS_FILE);
        // Save autobuy threshold.
        _stprintf_s(key, _countof(key), _T("Buy%d"), i);
        _stprintf_s(buffer, _countof(buffer), _T("%d"), settingsBuy[i]);
        WritePrivateProfileString(_T("Thresholds"), key, buffer, SETTINGS_FILE);
    }
}

//-------------------------------------------------------------------
// Auto trading (sell and buy) functionality definitions
//-------------------------------------------------------------------

// Market existence check (example address).
unsigned* const MarketBuildOrder = (unsigned* const)0x115F998;
inline bool doesMarketExist() {
    return *MarketBuildOrder > 0;
}

// Material addresses in memory (example addresses).
int* const MATERIAL_ADDRESSES[NUM_RESOURCES] = {
    (int*)0x115fcc4, (int*)0x115fcc8, (int*)0x115fccc, (int*)0x115fcd4,
    (int*)0x115fcd8, (int*)0x115fce0, (int*)0x115fce4, (int*)0x115fce8,
    (int*)0x115fcec, (int*)0x115fcf0, (int*)0x115fcf4, (int*)0x115fcfc,
    (int*)0x115fd00, (int*)0x115fd04, (int*)0x115fd08, (int*)0x115fd0c,
    (int*)0x115fd10, (int*)0x115fd14, (int*)0x115fd18, (int*)0x115fd1c
};

// Trade function pointer.
// The second parameter indicates: 1 for selling, 0 for buying.
typedef void(__cdecl* _Trade)(int player, int buy0_sell1, int item_type);
_Trade trade = nullptr;

// Auto-sell: If resource count is above the sell threshold, sell.
void autoSell() {
    if (!doesMarketExist())
        return;
    for (int i = 0; i < NUM_RESOURCES; i++) {
        if (*MATERIAL_ADDRESSES[i] > settingsSell[i]) {
            trade(1, 1, MATERIAL_IDS[i]);
        }
    }
}

// Auto-buy: If resource count is below the buy threshold, buy.
void autoBuy() {
    if (!doesMarketExist())
        return;
    for (int i = 0; i < NUM_RESOURCES; i++) {
        if (*MATERIAL_ADDRESSES[i] < settingsBuy[i]) {
            trade(1, 0, MATERIAL_IDS[i]);
        }
    }
}

//-------------------------------------------------------------------
// MainThread: Set up the trade function and run auto trading
//-------------------------------------------------------------------
DWORD WINAPI MainThread(LPVOID param) {
    // Get the module base and set the trade function pointer.
    uintptr_t moduleBase = (uintptr_t)GetModuleHandle(NULL);
    const unsigned tradeFunctionAddress = 0x0065E60; // Adjust as needed.
    trade = (_Trade)(moduleBase + tradeFunctionAddress);

    // Main loop: perform auto-sell and auto-buy until VK_END is pressed.
    while (!(GetAsyncKeyState(VK_END))) {
        autoSell();
        autoBuy();
        Sleep(50); // Small delay to reduce CPU usage.
    }
    return 0;
}

//-------------------------------------------------------------------
// Settings Window UI
//-------------------------------------------------------------------
// The window will show for each resource:
//   - Resource name (left column)
//   - Autosell slider (ID range: 3000+index)
//   - Autosell value display (ID range: 4000+index)
//   - Autobuy slider (ID range: 5000+index)
//   - Autobuy value display (ID range: 6000+index)
// Additionally, a "Save" button (ID 7000) is added at the bottom.
LRESULT CALLBACK SettingsWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
    {
        // Load settings from file.
        LoadSettings();

        HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

        // Create a group box to visually contain the threshold controls.
        HWND hGroup = CreateWindowEx(
            0, _T("BUTTON"), _T("Resource Thresholds"),
            WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
            5, 5, 490, NUM_RESOURCES * 25 + 90, // Adjust width/height as needed
            hwnd, NULL, g_hInstance, NULL
        );
        SendMessage(hGroup, WM_SETFONT, (WPARAM)hFont, TRUE);

        int startY = 25;  // Y offset inside the group box
        for (int i = 0; i < NUM_RESOURCES; i++)
        {
            int yPos = startY + i * 25; // 25 px per row
            int xPos = 15;              // indent inside group box

            // Resource name label
            HWND hName = CreateWindowEx(
                0, _T("STATIC"), resourceNames[i],
                WS_CHILD | WS_VISIBLE,
                xPos, yPos, 80, 20,
                hwnd, (HMENU)(100 + i), g_hInstance, NULL
            );
            SendMessage(hName, WM_SETFONT, (WPARAM)hFont, TRUE);

            xPos += 85;

            // "Sell" label
            HWND hSellLabel = CreateWindowEx(
                0, _T("STATIC"), _T("Sell:"),
                WS_CHILD | WS_VISIBLE,
                xPos, yPos, 30, 20,
                hwnd, NULL, g_hInstance, NULL
            );
            SendMessage(hSellLabel, WM_SETFONT, (WPARAM)hFont, TRUE);

            xPos += 35;

            // Create autosell slider (compact width)
            HWND hSellSlider = CreateWindowEx(
                0, TRACKBAR_CLASS, NULL,
                WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
                xPos, yPos - 2, 100, 25,
                hwnd, (HMENU)(3000 + i), g_hInstance, NULL
            );
            SendMessage(hSellSlider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 200));
            SendMessage(hSellSlider, TBM_SETTICFREQ, 5, 0);
            SendMessage(hSellSlider, TBM_SETLINESIZE, 0, 5);
            SendMessage(hSellSlider, TBM_SETPAGESIZE, 0, 5);
            SendMessage(hSellSlider, TBM_SETPOS, TRUE, settingsSell[i]);
            // Force Explorer theme for a modern look
            SetWindowTheme(hSellSlider, L"Explorer", NULL);

            xPos += 105;

            // Sell value label
            TCHAR sellValue[16];
            _stprintf_s(sellValue, _countof(sellValue), _T("%d"), settingsSell[i]);
            HWND hSellVal = CreateWindowEx(
                0, _T("STATIC"), sellValue,
                WS_CHILD | WS_VISIBLE,
                xPos, yPos, 30, 20,
                hwnd, (HMENU)(4000 + i), g_hInstance, NULL
            );
            SendMessage(hSellVal, WM_SETFONT, (WPARAM)hFont, TRUE);

            xPos += 40;

            // "Buy" label
            HWND hBuyLabel = CreateWindowEx(
                0, _T("STATIC"), _T("Buy:"),
                WS_CHILD | WS_VISIBLE,
                xPos, yPos, 30, 20,
                hwnd, NULL, g_hInstance, NULL
            );
            SendMessage(hBuyLabel, WM_SETFONT, (WPARAM)hFont, TRUE);

            xPos += 35;

            // Create autobuy slider
            HWND hBuySlider = CreateWindowEx(
                0, TRACKBAR_CLASS, NULL,
                WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
                xPos, yPos - 2, 100, 25,
                hwnd, (HMENU)(5000 + i), g_hInstance, NULL
            );
            SendMessage(hBuySlider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 200));
            SendMessage(hBuySlider, TBM_SETTICFREQ, 5, 0);
            SendMessage(hBuySlider, TBM_SETLINESIZE, 0, 5);
            SendMessage(hBuySlider, TBM_SETPAGESIZE, 0, 5);
            SendMessage(hBuySlider, TBM_SETPOS, TRUE, settingsBuy[i]);
            // Force Explorer theme
            SetWindowTheme(hBuySlider, L"Explorer", NULL);

            xPos += 105;

            // Buy value label
            TCHAR buyValue[16];
            _stprintf_s(buyValue, _countof(buyValue), _T("%d"), settingsBuy[i]);
            HWND hBuyVal = CreateWindowEx(
                0, _T("STATIC"), buyValue,
                WS_CHILD | WS_VISIBLE,
                xPos, yPos, 30, 20,
                hwnd, (HMENU)(6000 + i), g_hInstance, NULL
            );
            SendMessage(hBuyVal, WM_SETFONT, (WPARAM)hFont, TRUE);
        }

        // Create a "Save" button below the group box
        HWND hSaveBtn = CreateWindowEx(
            0, _T("BUTTON"), _T("Save"),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            15, NUM_RESOURCES * 25 + 45, 80, 25, // adjust as needed
            hwnd, (HMENU)7000, g_hInstance, NULL
        );
        SendMessage(hSaveBtn, WM_SETFONT, (WPARAM)hFont, TRUE);

        return 0;
    }

    case WM_HSCROLL:
    {
        // Handle slider updates
        HWND hSlider = (HWND)lParam;
        if (hSlider)
        {
            int id = GetDlgCtrlID(hSlider);
            int rawPos = (int)SendMessage(hSlider, TBM_GETPOS, 0, 0);
            int pos = ((rawPos + 2) / 5) * 5; // snap to multiples of 5
            if (rawPos != pos)
            {
                SendMessage(hSlider, TBM_SETPOS, TRUE, pos);
            }

            // Sell sliders
            if (id >= 3000 && id < 3000 + NUM_RESOURCES)
            {
                int index = id - 3000;
                settingsSell[index] = (unsigned short)pos;
                TCHAR buf[16];
                _stprintf_s(buf, _countof(buf), _T("%d"), pos);
                HWND hStatic = GetDlgItem(hwnd, 4000 + index);
                if (hStatic)
                    SetWindowText(hStatic, buf);
            }
            // Buy sliders
            else if (id >= 5000 && id < 5000 + NUM_RESOURCES)
            {
                int index = id - 5000;
                settingsBuy[index] = (unsigned short)pos;
                TCHAR buf[16];
                _stprintf_s(buf, _countof(buf), _T("%d"), pos);
                HWND hStatic = GetDlgItem(hwnd, 6000 + index);
                if (hStatic)
                    SetWindowText(hStatic, buf);
            }
        }
        return 0;
    }

    case WM_COMMAND:
    {
        // Save button clicked
        if (LOWORD(wParam) == 7000 && HIWORD(wParam) == BN_CLICKED)
        {
            SaveSettings();
            MessageBox(hwnd, _T("Settings saved."), _T("Info"), MB_OK);
        }
        break;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

//-------------------------------------------------------------------
// Thread function that creates and runs the settings window.
//-------------------------------------------------------------------
DWORD WINAPI SettingsWindowThread(LPVOID lpParameter) {
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);

    const TCHAR* className = _T("SettingsWindowClass");
    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = SettingsWndProc;
    wc.hInstance = g_hInstance;
    wc.lpszClassName = className;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);  // Set standard background brush
    RegisterClassEx(&wc);

    // Create a window sized for the compact layout:
    RECT rc = { 0, 0, 520, NUM_RESOURCES * 25 + 110 };
    AdjustWindowRectEx(&rc, WS_OVERLAPPEDWINDOW, FALSE, 0);
    HWND hwnd = CreateWindowEx(
        0, className, _T("Resource Settings"), WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
        NULL, NULL, g_hInstance, NULL);
    if (hwnd) {
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return 0;
}

//-------------------------------------------------------------------
// DllMain: Launch both the auto trading thread and the settings UI thread
//-------------------------------------------------------------------
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        g_hInstance = hModule;
        CreateThread(NULL, 0, MainThread, hModule, 0, NULL);
        CreateThread(NULL, 0, SettingsWindowThread, NULL, 0, NULL);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
