#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <string>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "gdi32.lib")

#define ID_BTN_UNINSTALL 101

HWND hBtnUninstall, hStatusText;
HBRUSH hBrushBackground = NULL;

bool RemoveDirectoryRecursive(const std::wstring& path) {
    std::wstring searchPath = path + L"\\*.*";
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &fd);
    
    if (hFind == INVALID_HANDLE_VALUE) return false;

    do {
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
        
        std::wstring filePath = path + L"\\" + fd.cFileName;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            RemoveDirectoryRecursive(filePath);
        } else {
            DeleteFileW(filePath.c_str());
        }
    } while (FindNextFileW(hFind, &fd));
    
    FindClose(hFind);
    return RemoveDirectoryW(path.c_str()) == TRUE;
}

void PerformUninstallation(HWND hwnd) {
    std::wstring installDir = L"";
    bool allUsers = false;

    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\MiiraCrypt", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        wchar_t buffer[MAX_PATH];
        DWORD bufSize = sizeof(buffer);
        if (RegQueryValueExW(hKey, L"InstallPath", NULL, NULL, (LPBYTE)buffer, &bufSize) == ERROR_SUCCESS) {
            installDir = buffer;
        }
        RegCloseKey(hKey);
    }

    if (installDir.empty() && RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\MiiraCrypt", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        wchar_t buffer[MAX_PATH];
        DWORD bufSize = sizeof(buffer);
        if (RegQueryValueExW(hKey, L"InstallPath", NULL, NULL, (LPBYTE)buffer, &bufSize) == ERROR_SUCCESS) {
            installDir = buffer;
            allUsers = true;
        }
        RegCloseKey(hKey);
    }

    if (installDir.empty()) {
        wchar_t currentPath[MAX_PATH];
        GetModuleFileNameW(NULL, currentPath, MAX_PATH);
        PathRemoveFileSpecW(currentPath);
        installDir = currentPath;
    }

    wchar_t startMenuPath[MAX_PATH];
    SHGetFolderPathW(NULL, allUsers ? CSIDL_COMMON_STARTMENU : CSIDL_STARTMENU, NULL, 0, startMenuPath);
    std::wstring programsFolder = std::wstring(startMenuPath) + L"\\Programs\\MiiraCrypt";
    RemoveDirectoryRecursive(programsFolder);

    RegDeleteKeyW(allUsers ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER, L"Software\\MiiraCrypt");

    std::wstring cmdCommand = L"cmd.exe /c timeout /t 2 /nobreak > nul & rmdir /s /q \"" + installDir + L"\"";
    
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    CreateProcessW(NULL, (LPWSTR)cmdCommand.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

    MessageBoxW(hwnd, L"MiiraCrypt успешно удален с вашего компьютера.", L"Удаление завершено", MB_OK | MB_ICONINFORMATION);
    PostQuitMessage(0);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        hBrushBackground = CreateSolidBrush(RGB(24, 24, 24));
        CreateWindowW(L"STATIC", L"Вы действительно хотите удалить MiiraCrypt?", WS_VISIBLE | WS_CHILD, 30, 30, 340, 20, hwnd, NULL, NULL, NULL);
        hStatusText = CreateWindowW(L"STATIC", L"Все файлы программы и ярлыки будут удалены.", WS_VISIBLE | WS_CHILD, 30, 60, 340, 20, hwnd, NULL, NULL, NULL);
        hBtnUninstall = CreateWindowW(L"BUTTON", L"Удалить", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 110, 110, 160, 38, hwnd, (HMENU)ID_BTN_UNINSTALL, NULL, NULL);
        break;

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, RGB(240, 240, 240));
        SetBkMode(hdc, TRANSPARENT);
        return (INT_PTR)hBrushBackground;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_BTN_UNINSTALL) {
            PerformUninstallation(hwnd);
        }
        break;

    case WM_DESTROY:
        if (hBrushBackground) DeleteObject(hBrushBackground);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"MiiraCryptUninstallerClass";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(0, CLASS_NAME, L"Удаление MiiraCrypt", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 400, 210, NULL, NULL, hInstance, NULL);

    if (!hwnd) return 0;
    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}