#include "AmbientOcclusion.h"

using namespace DirectX;

void AmbientOcclusion::OnMouseMove(WPARAM state, int x, int y)
{
	 if((state & MK_LBUTTON) != 0)
    {
        float dx = XMConvertToRadians(0.25f*(x - lastPos.x));
        float dy = XMConvertToRadians(0.25f*(y - lastPos.y));

		camera.Pitch(dy);
		camera.RotateY(dx);
    }
    lastPos.x = x;
    lastPos.y = y;
}

void AmbientOcclusion::OnMouseDown(WPARAM state, int x, int y)
{
	lastPos.x = x;
    lastPos.y = y;

    SetCapture(hwnd);
}

void AmbientOcclusion::OnMouseUp(WPARAM state, int x, int y)
{
	ReleaseCapture();
}

