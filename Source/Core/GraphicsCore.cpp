#include "GraphicsCore.h"

bool GraphicsCore::Initialize(HWND hwnd, float width, float height)
{
    // Setup SwapChain description (Configuration boilerplate)
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 2;

    scd.BufferDesc.Width = width;
    scd.BufferDesc.Height = height;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.RefreshRate.Numerator = 60;
    scd.BufferDesc.RefreshRate.Denominator = 1;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hwnd;

    scd.SampleDesc.Count = 1;
    scd.SampleDesc.Quality = 0;

    scd.Windowed = TRUE;

    scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    // Create Device and SwapChain
    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG; // Enable Debug Layer for detailed error messages
#endif

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,                    // Adapter (nullptr = default adapter)
        D3D_DRIVER_TYPE_HARDWARE,   // Driver Type
        nullptr,                    // Software Rasterizer (unused)
        createDeviceFlags,          // Flags
        nullptr,                    // Feature Levels (nullptr = default set)
        0,                          // Feature Levels count
        D3D11_SDK_VERSION,          // SDK Version
        &scd,                       // SwapChain Description
        &m_SwapChain,               // Output: SwapChain
        &m_Device,                  // Output: Device
        nullptr,                    // Output: Feature Level
        &m_Context                  // Output: Device Context
    );

    if (FAILED(hr)) return false;

    // Create Render Target View (RTV) and Setup Viewport
    CreateRenderTarget(width, height);
    CreateRasterizerState();

    return true;
}

void GraphicsCore::CreateRenderTarget(int width, int height)
{
    // Get the BackBuffer texture from the SwapChain
    // The SwapChain created the texture internally; we just need access to it.
    ComPtr<ID3D11Texture2D> backBuffer;
    m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)backBuffer.GetAddressOf());

    // Create the Render Target View (RTV)
    m_Device->CreateRenderTargetView(backBuffer.Get(), nullptr, &m_RTV);

    // Configure the Viewport
    m_Viewport.TopLeftX = 0;
    m_Viewport.TopLeftY = 0;
    m_Viewport.Width = (float)width;
    m_Viewport.Height = (float)height;
    m_Viewport.MinDepth = 0.0f;
    m_Viewport.MaxDepth = 1.0f;
}

void GraphicsCore::BeginFrame(const float color[4])
{
    m_Context->ClearRenderTargetView(m_RTV.Get(), color);
    m_Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_Context->RSSetViewports(1, &m_Viewport);
    m_Context->RSSetState(m_RasterizerState.Get());
    m_Context->OMSetRenderTargets(1, m_RTV.GetAddressOf(), nullptr);
}

void GraphicsCore::EndFrame()
{
    // Present the back buffer to the screen
    // 1 = VSync On, 0 = VSync Off (Uncapped FPS)
    m_SwapChain->Present(0, 0);
}

void GraphicsCore::OnResize(int width, int height)
{
    if (!m_SwapChain) return;

    // Critical: Release the existing RTV before resizing!
    // If we hold a reference to the back buffer, ResizeBuffers will fail.
    m_Context->OMSetRenderTargets(0, nullptr, nullptr);
    m_RTV.Reset();

    // Resize the SwapChain buffers
    m_SwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);

    // Recreate the RTV with the new size
    CreateRenderTarget(width, height);
}

// Set up Rasterizer State
void GraphicsCore::CreateRasterizerState()
{
    D3D11_RASTERIZER_DESC rasterizerdesc = {};
    rasterizerdesc.FillMode = D3D11_FILL_SOLID;
    rasterizerdesc.CullMode = D3D11_CULL_NONE; // Important: Disable culling for the full-screen quad

    m_Device->CreateRasterizerState(&rasterizerdesc, &m_RasterizerState);
}