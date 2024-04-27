#include "Win32.h"
#pragma warning(disable: 4996)

LRESULT CALLBACK Win32_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    Win32* win32 = (Win32*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch(msg)
    {
    case WM_CREATE:
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG)((LPCREATESTRUCT)lParam)->lpCreateParams);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        if(win32)
            return win32->MsgProc(msg, wParam, lParam);
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

//**************
// Win32

Win32::Win32(UINT width, UINT height, TCHAR* title) : width(width), height(height), wndCaption(title)
{
    cursorInputMode = WIN32_INPUTMODE_CURSOR_NORMAL;
    
}

int Win32::Run(HINSTANCE hInstance, int iCmdShow)
{
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    ParseCommandLineArgs(argv, argc);
    LocalFree(argv);

    WNDCLASSEX wc;
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS | CS_OWNDC;
    wc.cbWndExtra = 0;
    wc.cbClsExtra = 0;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm = NULL;
    wc.hInstance = hInstance;
    wc.lpfnWndProc = Win32_WndProc;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = TEXT("Win32");

    RegisterClassEx(&wc);

    RECT rect = {0, 0, (LONG)width, (LONG)height};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, 0);

    hwnd = CreateWindowEx(0, TEXT("Win32"), wndCaption, WS_OVERLAPPEDWINDOW, 
        CW_USEDEFAULT, CW_USEDEFAULT, 
        rect.right - rect.left,
        rect.bottom - rect.top,
        NULL, NULL, hInstance, this);

    OnInit();

    ShowWindow(hwnd, iCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while(GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    ClipCursor(NULL);
    ShowCursor(1);
    
    OnDestroy();
    return msg.wParam;
}

LRESULT Win32::MsgProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
    POINT cursorCoords, cursorCoordsOnScreen;
    POINT cursorOffset;
    UINT type;
    UINT fActive;

    switch(msg)
    {
    case WM_PAINT:
        OnUpdate();
        OnRender();
        return 0;
    
    case WM_KEYDOWN:
        OnKeyDown((UINT8)wParam);
        return 0;

    case WM_KEYUP:
        OnKeyUp((UINT8)wParam);
        return 0;

    case WM_ACTIVATE:
        if(cursorInputMode == WIN32_INPUTMODE_CURSOR_VIRTUAL)
        {
            fActive = LOWORD(wParam);
            if(fActive != WA_INACTIVE)
            {
                RECT rect;
                rect.left = cursorVirtualOriginOnScreen.x;
                rect.top = cursorVirtualOriginOnScreen.y;
                rect.right = cursorVirtualEdgeOnScreen.x;
                rect.bottom = cursorVirtualEdgeOnScreen.y;

                ClipCursor(&rect);
            }
            else 
                ClipCursor(NULL);
        }
        return 0;

    case WM_MOUSEMOVE:
        cursorCoords.x = LOWORD(lParam);
        cursorCoords.y = HIWORD(lParam);

        switch(cursorInputMode)
        {
        case WIN32_INPUTMODE_CURSOR_VIRTUAL:
            cursorCoordsOnScreen = cursorCoords;
            ClientToScreen(hwnd, &cursorCoordsOnScreen);

            cursorOffset.x = cursorCoords.x - cursorLastPoint.x;
            cursorOffset.y = cursorCoords.y - cursorLastPoint.y; 

            cursorVirtualCoords.x += cursorOffset.x;
            cursorVirtualCoords.y += cursorOffset.y;

            if(cursorCoords.x <= 0)
            {
                SetCursorPos(cursorVirtualEdgeOnScreen.x - 1, cursorCoordsOnScreen.y);
                cursorCoords.x = width - 1;
                cursorCoordsOnScreen.x = cursorVirtualEdgeOnScreen.x - 1;
            }
            else if(cursorCoords.x >= width - 1)
            {
                SetCursorPos(cursorVirtualOriginOnScreen.x + 1, cursorCoordsOnScreen.y);
                cursorCoords.x = 1;
                cursorCoordsOnScreen.x = cursorVirtualOriginOnScreen.x + 1;
            }

            if(cursorCoords.y <= 0)
            {
                SetCursorPos(cursorCoordsOnScreen.x, cursorVirtualEdgeOnScreen.y - 1);
                cursorCoords.y = height - 1;
                cursorCoordsOnScreen.y = cursorVirtualEdgeOnScreen.y - 1;
            }
            else if(cursorCoords.y >= height - 1)
            {
                SetCursorPos(cursorCoordsOnScreen.x, cursorVirtualOriginOnScreen.y + 1);
                cursorCoords.y = 1;
                cursorCoordsOnScreen.y = cursorVirtualOriginOnScreen.y + 1;
            }

            cursorLastPoint = cursorCoords;
            OnMouseMove(cursorVirtualCoords.x, cursorVirtualCoords.y);
            break;
        case WIN32_INPUTMODE_CURSOR_NORMAL:
            OnMouseMove(cursorCoords.x, cursorCoords.y);
            break;
        }
        return 0;
    case WM_MOVE:
        if(cursorInputMode == WIN32_INPUTMODE_CURSOR_VIRTUAL)
        {
            cursorVirtualEdgeOnScreen = {(long)width, (long)height};
            ClientToScreen(hwnd, &cursorVirtualEdgeOnScreen);
            cursorVirtualCenterPointOnScreen = cursorVirtualCenterPoint;
            ClientToScreen(hwnd, &cursorVirtualCenterPointOnScreen);
            cursorVirtualOriginOnScreen = {0, 0};
            ClientToScreen(hwnd, &cursorVirtualOriginOnScreen);
        
        }
        return 0;
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
        switch(msg)
        {
        case WM_LBUTTONDOWN:
            type = WIN32_MOUSETYPE_LEFT;
            break;
        case WM_RBUTTONDOWN:
            type = WIN32_MOUSETYPE_RIGHT;
            break;
        case WM_MBUTTONDOWN:
            type = WIN32_MOUSETYPE_MIDDLE;
            break;
        case WM_LBUTTONDBLCLK:
            type = WIN32_MOUSETYPE_DLEFT;
            break;
        case WM_RBUTTONDBLCLK:
            type = WIN32_MOUSETYPE_DRIGHT;
            break;
        case WM_MBUTTONDBLCLK:
            type = WIN32_MOUSETYPE_DMIDDLE;
            break;
        }

        OnMouseDown(type, wParam, LOWORD(lParam), HIWORD(lParam));
        return 0;
    
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
        switch(msg)
        {
        case WM_LBUTTONUP:
            type = WIN32_MOUSETYPE_LEFT;
            break;
        case WM_RBUTTONUP:
            type = WIN32_MOUSETYPE_RIGHT;
            break;     
        case WM_MBUTTONUP:
            type = WIN32_MOUSETYPE_MIDDLE;
            break;
        }   

        OnMouseUp(type, wParam, LOWORD(lParam), HIWORD(lParam));
        return 0;
    

    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void Win32::SetTitle(TCHAR* title)
{
    SetWindowText(hwnd, title);
    wndCaption = title;
}

void Win32::SetCursorInputMode(UINT inputMode)
{
    cursorInputMode = inputMode & 1;
    ShowCursor(inputMode >> 1);

    if(cursorInputMode == WIN32_INPUTMODE_CURSOR_VIRTUAL)
    {
        // On Client
        cursorVirtualCenterPoint.x = width / 2;
        cursorVirtualCenterPoint.y = height / 2;
        cursorVirtualCoords = cursorVirtualCenterPoint;
        cursorLastPoint = cursorVirtualCenterPoint;

        // On Screen
        cursorVirtualEdgeOnScreen = {(long)width, (long)height};
        cursorVirtualCenterPointOnScreen = cursorVirtualCenterPoint;
        cursorVirtualOriginOnScreen = {0, 0};
        ClientToScreen(hwnd, &cursorVirtualEdgeOnScreen);
        ClientToScreen(hwnd, &cursorVirtualCenterPointOnScreen);
        ClientToScreen(hwnd, &cursorVirtualOriginOnScreen);
        
        SetCursorPos(cursorVirtualCenterPointOnScreen.x, cursorVirtualCenterPointOnScreen.y);
        RECT rect;
        rect.left = cursorVirtualOriginOnScreen.x;
        rect.top = cursorVirtualOriginOnScreen.y;
        rect.right = cursorVirtualEdgeOnScreen.x;
        rect.bottom = cursorVirtualEdgeOnScreen.y;

        ClipCursor(&rect);
    }
    else if(cursorInputMode == WIN32_INPUTMODE_CURSOR_NORMAL)
    {
        ClipCursor(NULL);
    }
}

void Win32::CloseWindow()
{
    SendMessage(hwnd, WM_DESTROY, 0, 0);
}

UINT Win32::GetKeyState(UINT code)
{
    return GetAsyncKeyState(code) & 0x8000;
}