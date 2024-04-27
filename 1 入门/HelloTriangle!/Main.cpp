#include "D3DApp.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
#if defined(DEBUG) || defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    D3DApp app;

    try
    {
        return app.Run(hInstance, iCmdShow);
    }
    catch(DXException& e)
    {
        MessageBeep(0);
        MessageBox(NULL, e.ToString(), TEXT("HR Failed"), MB_OK);
        return 0;
    }

    return 0;
}