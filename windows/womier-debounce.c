#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdint.h>

#define DEBOUNCE_MS 35

static HHOOK hHook = NULL;
static uint64_t last_up_time[256] = {0};
static int key_down[256] = {0};

static inline uint64_t get_time_ms(void) {
    return GetTickCount64();
}

static inline int is_modifier(DWORD vkCode) {
    return (vkCode == VK_SHIFT || vkCode == VK_LSHIFT || vkCode == VK_RSHIFT ||
            vkCode == VK_CONTROL || vkCode == VK_LCONTROL || vkCode == VK_RCONTROL ||
            vkCode == VK_MENU || vkCode == VK_LMENU || vkCode == VK_RMENU ||
            vkCode == VK_LWIN || vkCode == VK_RWIN ||
            vkCode == VK_TAB || vkCode == VK_CAPITAL);
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT *p = (KBDLLHOOKSTRUCT *)lParam;
        DWORD vk = p->vkCode;
        uint64_t now = get_time_ms();

        if (vk < 256) {
            if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
                if (!key_down[vk]) {
                    // Filter chatter only if switch was recently released
                    if (!is_modifier(vk) && last_up_time[vk] > 0 && (now - last_up_time[vk] < DEBOUNCE_MS)) {
                        // Drop spurious bouncing keystroke
                        return 1;
                    }
                    key_down[vk] = 1;
                }
            } else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
                key_down[vk] = 0;
                last_up_time[vk] = now;
            }
        }
    }
    return CallNextHookEx(hHook, nCode, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    // Ensure only a single instance runs
    HANDLE hMutex = CreateMutexA(NULL, TRUE, "WomierDebounceSingleInstanceMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return 0;
    }

    hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
    if (!hHook) {
        return 1;
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hHook);
    if (hMutex) CloseHandle(hMutex);
    return 0;
}
