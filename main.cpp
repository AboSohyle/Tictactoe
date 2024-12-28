#ifndef UNICODE
#define UNICODE 1
#endif

#ifndef _UNICODE
#define _UNICODE 1
#endif

#define _WIN32_WINNT _WIN32_WINNT_WIN10
#define _WIN32_IE _WIN32_IE_IE100

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN

#include <Windows.h>
#include <commctrl.h>
#include "resource.h"
#include "TicTac.h"

TicTac *game;
int cx = 600;
int cy = 650;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {

    case WM_GETMINMAXINFO:
    {
        MINMAXINFO *minMax = (MINMAXINFO *)lParam;
        minMax->ptMinTrackSize.x = cx;
        minMax->ptMinTrackSize.y = cy;

        break;
    }

    case WM_CREATE:
    {
        game = new TicTac();
        if (!game->Initialize(hWnd))
        {
            MessageBox(hWnd, L"Creating game class failed.", L"Error", MB_ICONERROR);
            PostQuitMessage(0);
        }
        break;
    }

    case WM_SIZE:
    {
        game->ReSize(LOWORD(lParam), HIWORD(lParam));
        break;
    }

    case WM_DESTROY:
    {
        PostQuitMessage(0);
        break;
    }
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
    int ret = -1;

    if (SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)))
    {
        WNDCLASSEX wc = {};

        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = &WndProc;
        wc.hInstance = hInstance;
        wc.hCursor = (HCURSOR)LoadImage(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_SHARED);
        wc.hbrBackground = CreateSolidBrush(0x2E2722);
        wc.lpszClassName = L"TicTacToe";
        wc.hIcon = (HICON)LoadImage(hInstance,
                                    MAKEINTRESOURCE(IDI_APPICON),
                                    IMAGE_ICON, 0, 0,
                                    LR_DEFAULTSIZE | LR_DEFAULTCOLOR | LR_SHARED);
        wc.hIconSm = (HICON)LoadImage(hInstance,
                                      MAKEINTRESOURCE(IDI_APPICON),
                                      IMAGE_ICON,
                                      GetSystemMetrics(SM_CXSMICON),
                                      GetSystemMetrics(SM_CYSMICON),
                                      LR_DEFAULTCOLOR | LR_SHARED);

        if (!RegisterClassEx(&wc))
        {
            MessageBox(NULL, L"Registering main class failed.", L"Error", MB_ICONERROR);
            CoUninitialize();
            return ret;
        }

        RECT rect;
        GetClientRect(GetDesktopWindow(), &rect);

        rect.left = (rect.right / 2) - (cx / 2);
        rect.top = (rect.bottom / 2) - (cy / 2);

        HWND hWnd = CreateWindowEx(0, wc.lpszClassName, wc.lpszClassName,
                                   WS_OVERLAPPEDWINDOW,
                                   rect.left, rect.top,
                                   cx, cy,
                                   NULL, NULL, hInstance, NULL);

        if (!hWnd)
        {
            MessageBox(NULL, L"Creating main window failed.", L"Error", MB_ICONERROR);
            CoUninitialize();
            return ret;
        }

        ShowWindow(hWnd, nCmdShow);
        UpdateWindow(hWnd);

        MSG msg;

        while ((ret = GetMessage(&msg, NULL, 0, 0)) != 0)
        {
            if (ret == -1)
                break;
            else
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        // DeleteObject(wc.hbrBackground);
        ret = (int)msg.wParam;
        CoUninitialize();
    }

    return ret;
}
