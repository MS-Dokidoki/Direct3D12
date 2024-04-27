#include "D3DApp.h"

D3DApp::D3DApp() : Win32(800, 600, TEXT("D3D App")), stViewport(0.0f, 0.0f, 800.0f, 600.0f),
stScissorRect(0, 0, 800, 600), uiCurrentFence(0)
{
	
}

void D3DApp::OnInit()
{
	LoadPipeline();
}

void D3DApp::LoadPipeline()
{
	UINT uiFactoryFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
	ComPtr<ID3D12Debug> pDebugController;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&pDebugController)));
	pDebugController->EnableDebugLayer();
	uiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
	
	// ---- Create Device
	ComPtr<IDXGIFactory4> pDXGIFactory;
	
	ThrowIfFailedEx(CreateDXGIFactory2(uiFactoryFlags, IID_PPV_ARGS(&pDXGIFactory)));

	HRESULT hr = D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&pDevice));
	if (FAILED(hr))
	{
		/* Create warp adapter device*/
		ComPtr<IDXGIAdapter4> pWarpAdapter;
		ThrowIfFailedEx(pDXGIFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));
		ThrowIfFailedEx(D3D12CreateDevice(pWarpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&pDevice)));
	}

	// ---- Create Queue
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailedEx(pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&pCommandQueue)));

	// ---- Create SawpChain
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = uiSwapChainBufferCount;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.Format = emDXGIFormat;
	swapChainDesc.Width = GetWidth();
	swapChainDesc.Height = GetHeight();
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> pSwapChain1;
	ThrowIfFailedEx(pDXGIFactory->CreateSwapChainForHwnd(
		pCommandQueue.Get(),
		GetHWND(),
		&swapChainDesc,
		NULL, NULL, &pSwapChain1
	));

	ThrowIfFailedEx(pSwapChain1.As(&pSwapChain));

	// ---- Create Descriptor Heap
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = uiSwapChainBufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;

	ThrowIfFailedEx(pDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&pRTVHeap)));
	uiRTVDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// ---- Create RTVs
	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(pRTVHeap->GetCPUDescriptorHandleForHeapStart());

	for (unsigned int i = 0; i < uiSwapChainBufferCount; ++i)
	{
		ThrowIfFailedEx(pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pRenderTargets[i])));
		pDevice->CreateRenderTargetView(pRenderTargets[i].Get(), NULL, handle);
		handle.Offset(1, uiRTVDescriptorSize);
	}
	
	iCurrBackBuffer = pSwapChain->GetCurrentBackBufferIndex();

	// ---- Create Root signature
	CD3DX12_ROOT_SIGNATURE_DESC rsd;
	rsd.Init(0, NULL, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	ThrowIfFailedEx(D3D12SerializeRootSignature(&rsd, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));

	ThrowIfFailedEx(pDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&pRootSignature)));
	
	// ---- Load Shader
	ComPtr<ID3DBlob> pVS;
	ComPtr<ID3DBlob> pPS;
#if defined(DEBUG) || defined(_DEBUG)
	UINT uiCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT uiCompileFlags = 0;
#endif
	
	// Compile shader
	ThrowIfFailedEx(D3DCompileFromFile(
		(LPCWSTR)L"shader.hlsl", NULL, NULL, "VsMain", "vs_5_0", uiCompileFlags, 0, &pVS, NULL
	));
	ThrowIfFailedEx(D3DCompileFromFile(
		(LPCWSTR)L"shader.hlsl", NULL, NULL, "PsMain", "ps_5_0", uiCompileFlags, NULL, &pPS, NULL
	));

	// Define input layout
	D3D12_INPUT_ELEMENT_DESC inputLayouts[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	// Create Pipeline
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psd = {};
	psd.InputLayout = { inputLayouts, _countof(inputLayouts) };
	psd.pRootSignature = pRootSignature.Get();
	psd.PS = CD3DX12_SHADER_BYTECODE(pPS.Get());
	psd.VS = CD3DX12_SHADER_BYTECODE(pVS.Get());
	psd.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psd.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psd.DepthStencilState.DepthEnable = 0;
	psd.DepthStencilState.StencilEnable = 0;
	psd.SampleMask = UINT_MAX;
	psd.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psd.NumRenderTargets = 1;
	psd.RTVFormats[0] = emDXGIFormat;
	psd.SampleDesc.Count = 1;

	ThrowIfFailedEx(pDevice->CreateGraphicsPipelineState(
		&psd, IID_PPV_ARGS(&pPipelineState)
	));

	// ---- Create CommandObjects
	ThrowIfFailedEx(pDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pCommandAllocator)
	));

	ThrowIfFailedEx(pDevice->CreateCommandList(
		0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		pCommandAllocator.Get(), pPipelineState.Get(),
		IID_PPV_ARGS(&pCommandList)
	));

	pCommandList->Close();

	Vertex triangles[] =
	{
		{ { 0.0f, 0.25f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
		{ { 0.25f, -0.25f,  0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
		{ { -0.25f, -0.25f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
	};

	const UINT uiVertexBufferSize = sizeof(triangles);
	ThrowIfFailedEx(pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(uiVertexBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		NULL, IID_PPV_ARGS(&pVertexBuffer)
	));

	UINT8* pBegin;
	CD3DX12_RANGE range(0, 0);
	pVertexBuffer->Map(0, &range, (void**)&pBegin);
	memcpy(pBegin, triangles, uiVertexBufferSize);
	pVertexBuffer->Unmap(0, NULL);

	vertexBufferView.BufferLocation = pVertexBuffer->GetGPUVirtualAddress();
	vertexBufferView.StrideInBytes = sizeof(Vertex);
	vertexBufferView.SizeInBytes = uiVertexBufferSize;

	ThrowIfFailedEx(pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence)));
	uiCurrentFence = 0;

	hFenceEvent = CreateEvent(NULL, 0, 0, NULL);

	WaitForPreviousFrame();
}

void D3DApp::OnUpdate()
{

}

void D3DApp::OnRender()
{
	PopulateCommandList();
	
	ID3D12CommandList* ppCommandLists[] = { pCommandList.Get() };
	pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	ThrowIfFailedEx(pSwapChain->Present(1, 0));

	WaitForPreviousFrame();
}

void D3DApp::PopulateCommandList()
{
	ThrowIfFailedEx(pCommandAllocator->Reset());

	pCommandList->Reset(pCommandAllocator.Get(), pPipelineState.Get());
	
	pCommandList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(
			pRenderTargets[iCurrBackBuffer].Get(),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET
	));

	pCommandList->SetGraphicsRootSignature(pRootSignature.Get());
	pCommandList->RSSetScissorRects(1, &stScissorRect);
	pCommandList->RSSetViewports(1, &stViewport);

	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(pRTVHeap->GetCPUDescriptorHandleForHeapStart(), iCurrBackBuffer, uiRTVDescriptorSize);
	pCommandList->OMSetRenderTargets(1, &handle, 0, NULL);

	const float clearColor[] = { 5.0f, 1.0f, 1.0f, 1.0f };
	pCommandList->ClearRenderTargetView(handle, clearColor, 0, NULL);

	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCommandList->IASetVertexBuffers(0, 1, &vertexBufferView);
	pCommandList->DrawInstanced(3, 1, 0, 0);

	pCommandList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(
		pRenderTargets[iCurrBackBuffer].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT
	));
	pCommandList->Close();
}
void D3DApp::OnDestroy()
{
	WaitForPreviousFrame();
	CloseHandle(hFenceEvent);
}
void D3DApp::WaitForPreviousFrame()
{
	ThrowIfFailedEx(pCommandQueue->Signal(pFence.Get(), ++uiCurrentFence));

	if (pFence->GetCompletedValue() < uiCurrentFence)
	{
		ThrowIfFailedEx(pFence->SetEventOnCompletion(uiCurrentFence, hFenceEvent));
		WaitForSingleObject(hFenceEvent, INFINITE);
	}

	iCurrBackBuffer = pSwapChain->GetCurrentBackBufferIndex();
}
