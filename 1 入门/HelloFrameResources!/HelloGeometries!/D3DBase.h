#ifndef _D3DBASE_H
#define _D3DBASE_H
#include <sdkddkver.h>
#include <windows.h>

#include <directx/d3dx12.h>
#include <wrl.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <DirectXColors.h>
#include <d3d12.h>
#include <d3d12shader.h>
#include <d3dcompiler.h>

#if defined(_DEBUG)
#include <dxgidebug.h>
#endif

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")
#endif