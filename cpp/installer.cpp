#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <string>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "gdi32.lib")

#define ID_BTN_INSTALL  101
#define ID_RADIO_USER   102
#define ID_RADIO_ALL    103
#define ID_BTN_BROWSE   104
#define ID_EDIT_PATH    105

HWND hRadioUser, hRadioAll, hBtnInstall, hBtnBrowse, hEditPath;
HBRUSH hBrushBackground = NULL;

bool ExtractResource(int resourceId, const wchar_t* outputPath) {
    HRSRC hRes = FindResourceW(NULL, MAKEINTRESOURCEW(resourceId), (LPCWSTR)RT_RCDATA);
    if (!hRes) return false;
    
    HGLOBAL hData = LoadResource(NULL, hRes);
    if (!hData) return false;
    
    LPVOID pData = LockResource(hData);
    DWORD dataSize = SizeofResource(NULL, hRes);
    
    HANDLE hFile = CreateFileW(outputPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;
    
    DWORD bytesWritten;
    BOOL writeSuccess = WriteFile(hFile, pData, dataSize, &bytesWritten, NULL);
    CloseHandle(hFile);
    
    return writeSuccess && (bytesWritten == dataSize);
}

bool CreateShortcut(const std::wstring& targetPath, const std::wstring& shortcutPath, const std::wstring& desc) {
    HRESULT hres;
    IShellLinkW* psl;
    bool success = false;

    hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (LPVOID*)&psl);
    if (SUCCEEDED(hres)) {
        IPersistFile* ppf;
        psl->SetPath(targetPath.c_str());
        psl->SetDescription(desc.c_str());
        
        hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);
        if (SUCCEEDED(hres)) {
            hres = ppf->Save(shortcutPath.c_str(), TRUE);
            if (SUCCEEDED(hres)) success = true;
            ppf->Release();
        }
        psl->Release();
    }
    return success;
}

void SaveInstallPathToRegistry(const std::wstring& installDir, bool allUsers) {
    HKEY hKey;
    HKEY rootKey = allUsers ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
    const wchar_t* regPath = L"Software\\MiiraCrypt";

    if (RegCreateKeyExW(rootKey, regPath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, L"InstallPath", 0, REG_SZ, (LPBYTE)installDir.c_str(), (DWORD)(installDir.size() + 1) * sizeof(wchar_t));
        RegCloseKey(hKey);
    }
}

void AddToPath(const std::wstring& folderPath, bool allUsers) {
    HKEY hKey;
    const wchar_t* subKey = allUsers ? 
        L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment" : 
        L"Environment";
    HKEY rootKey = allUsers ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;

    if (RegOpenKeyExW(rootKey, subKey, 0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        wchar_t pathBuffer[4096];
        DWORD bufferSize = sizeof(pathBuffer);
        DWORD type = REG_EXPAND_SZ;

        if (RegQueryValueExW(hKey, L"PATH", NULL, &type, (LPBYTE)pathBuffer, &bufferSize) == ERROR_SUCCESS) {
            std::wstring currentPath(pathBuffer);
            if (currentPath.find(folderPath) == std::wstring::npos) {
                if (currentPath.back() != L';') currentPath += L";";
                currentPath += folderPath;
                RegSetValueExW(hKey, L"PATH", 0, REG_EXPAND_SZ, (LPBYTE)currentPath.c_str(), (DWORD)(currentPath.size() + 1) * sizeof(wchar_t));
            }
        }
        RegCloseKey(hKey);
        SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"Environment", SMTO_ABORTIFHUNG, 5000, NULL);
    }
}

void SelectInstallFolder(HWND hwnd) {
    wchar_t path[MAX_PATH];
    BROWSEINFOW bi = { 0 };
    bi.hwndOwner = hwnd;
    bi.lpszTitle = L"Выберите папку для установки MiiraCrypt:";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    
    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (pidl != NULL) {
        if (SHGetPathFromIDListW(pidl, path)) {
            std::wstring fullPath = std::wstring(path) + L"\\MiiraCrypt";
            SetWindowTextW(hEditPath, fullPath.c_str());
        }
        CoTaskMemFree(pidl);
    }
}

void UpdateDefaultPath() {
    bool allUsers = BST_CHECKED == SendMessageW(hRadioAll, BM_GETCHECK, 0, 0);
    wchar_t installDir[MAX_PATH];
    if (allUsers) {
        SHGetFolderPathW(NULL, CSIDL_PROGRAM_FILES, NULL, 0, installDir);
        wcscat_s(installDir, MAX_PATH, L"\\MiiraCrypt");
    } else {
        SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, installDir);
        wcscat_s(installDir, MAX_PATH, L"\\Programs\\MiiraCrypt");
    }
    SetWindowTextW(hEditPath, installDir);
}

void PerformInstallation(HWND hwnd) {
    bool allUsers = BST_CHECKED == SendMessageW(hRadioAll, BM_GETCHECK, 0, 0);
    
    wchar_t installDir[MAX_PATH];
    GetWindowTextW(hEditPath, installDir, MAX_PATH);

    if (wcslen(installDir) == 0) {
        MessageBoxW(hwnd, L"Укажите путь для установки!", L"Ошибка", MB_OK | MB_ICONERROR);
        return;
    }

    CreateDirectoryW(installDir, NULL);

    std::wstring exeDest = std::wstring(installDir) + L"\\miiracrypt.exe";
    std::wstring uninstDest = std::wstring(installDir) + L"\\uninstall.exe";

    if (!ExtractResource(101, exeDest.c_str()) || !ExtractResource(102, uninstDest.c_str())) {
        MessageBoxW(hwnd, L"Ошибка распаковки файлов! Проверьте права администратора.", L"Ошибка", MB_OK | MB_ICONERROR);
        return;
    }

    SaveInstallPathToRegistry(installDir, allUsers);
    AddToPath(installDir, allUsers);

    wchar_t startMenuPath[MAX_PATH];
    SHGetFolderPathW(NULL, allUsers ? CSIDL_COMMON_STARTMENU : CSIDL_STARTMENU, NULL, 0, startMenuPath);
    std::wstring programsFolder = std::wstring(startMenuPath) + L"\\Programs\\MiiraCrypt";
    CreateDirectoryW(programsFolder.c_str(), NULL);

    CreateShortcut(exeDest, programsFolder + L"\\MiiraCrypt.lnk", L"MiiraCrypt Desktop");
    CreateShortcut(uninstDest, programsFolder + L"\\Uninstall MiiraCrypt.lnk", L"Uninstall MiiraCrypt");

    MessageBoxW(hwnd, L"MiiraCrypt успешно установлен!", L"Установка завершена", MB_OK | MB_ICONINFORMATION);
    PostQuitMessage(0);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        hBrushBackground = CreateSolidBrush(RGB(24, 24, 24)); // Глубокий темный фон
        CreateWindowW(L"STATIC", L"Выберите параметры установки MiiraCrypt:", WS_VISIBLE | WS_CHILD, 20, 20, 420, 20, hwnd, NULL, NULL, NULL);

        hRadioUser = CreateWindowW(L"BUTTON", L"Установить только для текущего пользователя", WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON, 20, 55, 420, 20, hwnd, (HMENU)ID_RADIO_USER, NULL, NULL);
        hRadioAll  = CreateWindowW(L"BUTTON", L"Установить для всех (нужны права администратора)", WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON, 20, 85, 420, 20, hwnd, (HMENU)ID_RADIO_ALL, NULL, NULL);
        SendMessageW(hRadioUser, BM_SETCHECK, BST_CHECKED, 0);

        CreateWindowW(L"STATIC", L"Папка установки:", WS_VISIBLE | WS_CHILD, 20, 125, 120, 20, hwnd, NULL, NULL, NULL);
        
        hEditPath = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 20, 150, 320, 26, hwnd, (HMENU)ID_EDIT_PATH, NULL, NULL);
        hBtnBrowse = CreateWindowW(L"BUTTON", L"Обзор...", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 350, 150, 90, 26, hwnd, (HMENU)ID_BTN_BROWSE, NULL, NULL);

        hBtnInstall = CreateWindowW(L"BUTTON", L"Установить", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 150, 205, 160, 38, hwnd, (HMENU)ID_BTN_INSTALL, NULL, NULL);
        
        UpdateDefaultPath();
        break;

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, RGB(240, 240, 240));
        SetBkMode(hdc, TRANSPARENT); // Убирает черные квадраты вокруг текста
        return (INT_PTR)hBrushBackground;
    }
    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, RGB(255, 255, 255));
        SetBkColor(hdc, RGB(40, 40, 40));
        return (INT_PTR)CreateSolidBrush(RGB(40, 40, 40));
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_RADIO_USER || LOWORD(wParam) == ID_RADIO_ALL) {
            UpdateDefaultPath();
        }
        else if (LOWORD(wParam) == ID_BTN_BROWSE) {
            SelectInstallFolder(hwnd);
        }
        else if (LOWORD(wParam) == ID_BTN_INSTALL) {
            PerformInstallation(hwnd);
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
    const wchar_t CLASS_NAME[] = L"MiiraCryptInstallerClass";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(0, CLASS_NAME, L"Установка MiiraCrypt", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 480, 300, NULL, NULL, hInstance, NULL);

    if (!hwnd) return 0;
    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}