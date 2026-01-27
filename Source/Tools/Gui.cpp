#include "Constant.h"
#include "Camera.h"
#include "Renderer.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#include "Gui.h"

void Gui::Render(Constant& constant, Camera& camera, Renderer& renderer, float totalTime)
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (ImGui::Begin("Settings"))
	{
		auto io = ImGui::GetIO();

		ImGui::Text("Time: %.2f s", totalTime);
		ImGui::Text("Average FPS: %.1f", ImGui::GetIO().Framerate);
		ImGui::Text("Frame Time: %.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
		ImGui::Text("IO DisplaySize: %.1f, %.1f", io.DisplaySize.x, io.DisplaySize.y);

		ImGui::Text("Camera: %.1f, %.1f, %.1f", camera.m_Pos.x, camera.m_Pos.y, camera.m_Pos.z);
		ImGui::Text("Mouse Pos: %.1f, %.1f", io.MousePos.x, io.MousePos.y);

		if (ImGui::CollapsingHeader("Scene"))
		{
			ImGui::Checkbox("Distance2D", &renderer.m_Scene.bDistance2D);
			ImGui::Checkbox("Distance3D", &renderer.m_Scene.bDistance3D);
			ImGui::Checkbox("Cloud", &renderer.m_Scene.bCloud);
		}

		if (ImGui::CollapsingHeader("Cloud & Atmosphere Settings", ImGuiTreeNodeFlags_DefaultOpen))
		{
			// Section 1: Performance
			ImGui::Text("Performance");
			ImGui::SliderFloat("Step Size", &constant.m_Constants.StepSize, 0.01f, 0.5f, "%.3f");
			ImGui::Separator();

			// Section 2: Cloud Shape
			ImGui::Text("Cloud Shape");
			ImGui::SliderFloat("Cloud Scale", &constant.m_Constants.CloudScale, 0.1f, 5.0f);
			ImGui::SliderFloat("Coverage (Threshold)", &constant.m_Constants.CloudThreshold, 0.0f, 1.0f);
			ImGui::Separator();

			// Section 3: Lighting (Sun)
			ImGui::Text("Sun Lighting");
			// Passing &vector.x allows ImGui to access it as a float array
			if (ImGui::SliderFloat3("Sun Direction", &constant.m_Constants.SunDir.x, -1.0f, 1.0f))
			{
				// Direction must be normalized after manual adjustment
				constant.m_Constants.SunDir.Normalize();
			}
			ImGui::ColorEdit3("Sun Color", &constant.m_Constants.SunColor.x);
			ImGui::SliderFloat("Absorption", &constant.m_Constants.Absorption, 0.0f, 1.0f);
			ImGui::Separator();

			// Section 4: Atmosphere & Fog
			ImGui::Text("Atmosphere");
			ImGui::SliderFloat("Fog Density", &constant.m_Constants.FogDensity, 0.0f, 0.5f);
			ImGui::ColorEdit3("Fog Color", &constant.m_Constants.FogColor.x);
		}

	}

	ImGui::End();

	if (ImGui::Begin("Noise Preview"))
	{
		ImGui::Image((void*)renderer.m_NoiseSRV.Get(), ImVec2(204, 204));

		if (ImGui::Button("Re-Bake"))
		{
			renderer.BakeNoise();
		}
	}
	ImGui::End();

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

Gui::~Gui()
{
	// Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void Gui::Initialize(HWND hWnd, ID3D11Device* device, ID3D11DeviceContext* context)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImGui_ImplWin32_Init((void*)hWnd);
	ImGui_ImplDX11_Init(device, context);

	SetStyle();
}

void Gui::SetStyle()
{
	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;

	//ImGui::StyleColorsDark();
	ImGui::StyleColorsLight();
}