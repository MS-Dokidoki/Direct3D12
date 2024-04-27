#pragma once
#include <windows.h>

#define WIN32_MOUSETYPE_LEFT   0x01
#define WIN32_MOUSETYPE_RIGHT  0x02
#define WIN32_MOUSETYPE_MIDDLE 0x03
#define WIN32_MOUSETYPE_DLEFT 0x04
#define WIN32_MOUSETYPE_DRIGHT 0x05
#define WIN32_MOUSETYPE_DMIDDLE 0x06

#define WIN32_INPUTMODE_CURSOR_NORMAL 0x00
#define WIN32_INPUTMODE_CURSOR_VIRTUAL 0x01
#define WIN32_INPUTMODE_CURSOR_HIDDEN 0x00
#define WIN32_INPUTMODE_CURSOR_SHOW 0x02

class Win32
{
public:
    Win32(UINT width, UINT height, TCHAR* title);
    virtual ~Win32(){}

    int Run(HINSTANCE hInstance, int iCmdShow);
    
    void CloseWindow();
    LRESULT MsgProc(UINT msg, WPARAM wParam, LPARAM lParam);
protected:
    virtual void OnInit() = 0;
    virtual void OnUpdate() = 0;
    virtual void OnRender() = 0;
    virtual void OnDestroy() = 0;

    virtual void ParseCommandLineArgs(WCHAR* argv[], int argc) {}
protected:
    virtual void OnKeyDown(UINT8 code) {}
    virtual void OnKeyUp(UINT8 code) {}
    virtual void OnMouseDown(UINT type, WPARAM wParam, int x, int y){}
    virtual void OnMouseUp(UINT type, WPARAM wParam, int x, int y){}
    virtual void OnMouseMove(int x, int y){}

protected:
    UINT GetWidth() const { return this->width; }
    UINT GetHeight() const { return this->height; }
    TCHAR* GetTitle() const { return this->wndCaption; }
    HWND GetHWND() const { return this->hwnd; }
    
    UINT GetKeyState(UINT code);

protected:
    void SetTitle(TCHAR* title);
    void SetCursorInputMode(UINT inputMode);
    
private:
    HWND hwnd;
    UINT width, height;
    TCHAR* wndCaption;

    /********************/
    // ÐéÄâ¹â±ê
    UINT cursorInputMode;
    POINT cursorVirtualCoords;
    POINT cursorVirtualCenterPoint;

    POINT cursorVirtualEdgeOnScreen;
    POINT cursorVirtualOriginOnScreen;    // {0, 0}(on screen)
    POINT cursorVirtualCenterPointOnScreen; // {width / 2, height / 2}(on screen)
    POINT cursorLastPoint;
};