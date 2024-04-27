#include "D3DFrame.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
    try
    {
        D3DFrame app(hInstance);

        if(!app.Initialize())
            return 0;
        return app.Run();
    }
    catch(DirectX_Exception& e)
    {
        MessageBox(NULL, e.ToString(), TEXT("DirectX Failed"), MB_OK);
    }
    catch(D3DHelper_Exception& e)
    {
        MessageBox(NULL, e.ToString(), TEXT("D3DHelper Failed"), MB_OK);
    }
    return 0;
}
