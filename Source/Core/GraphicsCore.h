#pragma once

class GraphicsCore
{
public:
    GraphicsCore() {}
    ~GraphicsCore() {}

    // [Rule] System classes should NOT be copied.
    // Copying a core system creates ambiguity in resource ownership.
    GraphicsCore(const GraphicsCore&) = delete;
    GraphicsCore& operator=(const GraphicsCore&) = delete;

    // Initialize Device, Context, and SwapChain
    bool Initialize(HWND hwnd, float width, float height);

    // --- Per-Frame Operations ---

    // Clear the back buffer with a specific color
    void BeginFrame(const float color[4]);

    // Present the back buffer to the screen (Swap Chain)
    void EndFrame();

    // Handle window resize events
    void OnResize(int width, int height);

    // --- Getters ---

    // [Important] Provide raw pointers for other wrapper classes (Observer Pattern)
    // We return raw pointers because the caller uses them but does not own them.
    ID3D11Device* GetDevice() const { return m_Device.Get(); }
    ID3D11DeviceContext* GetContext() const { return m_Context.Get(); }

    const D3D11_VIEWPORT& GetViewport() const { return m_Viewport; }

private:
    // Helper function to create RTV (Used in Init and Resize)
    void CreateRenderTarget(int width, int height);
    void CreateRasterizerState();

private:
    // --- Core D3D11 Objects (The Holy Trinity) ---
    ComPtr<ID3D11Device>        m_Device;
    ComPtr<ID3D11DeviceContext> m_Context;
    ComPtr<IDXGISwapChain>      m_SwapChain;
    ComPtr<ID3D11RasterizerState> m_RasterizerState;

    // Render Target View (Back Buffer)
    ComPtr<ID3D11RenderTargetView> m_RTV;

    // Viewport configuration
    D3D11_VIEWPORT m_Viewport = {};
};