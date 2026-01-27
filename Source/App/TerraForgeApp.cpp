#include "GameTimer.h"

#include "TerraForgeApp.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool TerraForgeApp::Run()
{
	GameTimer timer;
	timer.Reset();

	bool bIsExit = false;

	while (bIsExit == false)
	{
		timer.Tick();

		MSG msg;
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT) bIsExit = true;
		}
		if (bIsExit) break;

		// --- Update ---
		m_Camera.Update(timer.GetDeltaTime());
		m_Constant.UpdateConstant(m_Camera, timer.GetDeltaTime(), timer.GetTotalTime(), m_Width, m_Height);

		if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
		{
			PostQuitMessage(0);
			break;
		}

		// --- Rendering ---
		m_Gfx.BeginFrame(m_ClearColor);
		m_Renderer.PrepareShader();
		m_Constant.BindConstantBuffer();
		m_Renderer.Render();
		m_Gui.Render(m_Constant, m_Camera, m_Renderer, timer.GetTotalTime());

		m_Gfx.EndFrame();
	}

	return true;
}

LRESULT CALLBACK TerraForgeApp::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return true;

	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void TerraForgeApp::Initialize(HINSTANCE hInstance)
{
	WCHAR WindowClass[] = L"TerraForge-DX";
	WCHAR Title[] = L"TerraForge-DX (DX11)";

	WNDCLASSW wndclass = { 0, WndProc, 0, 0, 0, 0, 0, 0, 0, WindowClass };
	RegisterClassW(&wndclass);

	DWORD dwStyle = WS_OVERLAPPEDWINDOW;
	RECT wr = { 0, 0, (LONG)m_Width, (LONG)m_Height };
	AdjustWindowRect(&wr, dwStyle, FALSE); // Border + Title Bar calculation

	HWND hWnd = CreateWindowExW(0, L"TerraForge-DX", L"TerraForge-DX",
		dwStyle | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT,
		wr.right - wr.left,   // This will be larger than 1280 (e.g., 1296)
		wr.bottom - wr.top,   // This will be larger than 720 (e.g., 759)
		nullptr, nullptr, hInstance, nullptr);

	RECT cr;
	GetClientRect(hWnd, &cr);
	m_Width = (float)(cr.right - cr.left);   // Should be EXACTLY 1280.0f
	m_Height = (float)(cr.bottom - cr.top);  // Should be EXACTLY 720.0f

	m_Gfx.Initialize(hWnd, m_Width, m_Height);
	m_Renderer.Initialize(m_Gfx.GetDevice(), m_Gfx.GetContext());
	m_Gui.Initialize(hWnd, m_Gfx.GetDevice(), m_Gfx.GetContext());
	m_Constant.Initialize(m_Gfx.GetDevice(), m_Gfx.GetContext());
	m_Camera.Initialize(m_Width / m_Height);

	m_Renderer.BakeNoise();
}
