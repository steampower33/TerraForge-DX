#include "Constant.h"
#include "Camera.h"
#include "Renderer.h"
#include "ResourceManager.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#include "Gui.h"

bool Gui::Update(float totalTime, Constant & constant, Camera & camera, Renderer & renderer, ResourceManager& resMgr)
{
    bool bCloudParamsChanged = false;

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Access the new CloudConstants struct
    // Assuming 'm_CloudData' is the instance name in your Constant class
    auto& cloudParams = constant.m_CloudConstants;

    if (ImGui::Begin("Settings"))
    {
        // --- Scene Control ---
        if (ImGui::CollapsingHeader("Scene Control"))
        {
            ImGui::Checkbox("Distance2D", &renderer.m_Scene.bDistance2D);
            ImGui::Checkbox("Distance3D", &renderer.m_Scene.bDistance3D);
            ImGui::Checkbox("Volumetric Cloud", &renderer.m_Scene.bCloud);
        }

        // --- Cloud Physics & Visuals ---
        if (ImGui::CollapsingHeader("Cloud Parameters", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::TextColored(ImVec4(0, 0, 0, 1), "[ Lighting ]");

            if (ImGui::SliderFloat3("Sun Direction", &cloudParams.SunDir.x, -1.0f, 1.0f))
            {
                cloudParams.SunDir.Normalize();
                bCloudParamsChanged = true;
            }

            bCloudParamsChanged |= ImGui::SliderFloat("Sun Intensity", &cloudParams.SunIntensity, 0.0f, 500.0f);

            ImGui::TextColored(ImVec4(0, 0, 0, 1), "[ Shape & Detail ]");

            bCloudParamsChanged |= ImGui::SliderFloat("Cloud Scale", &cloudParams.CloudScale, 0.1f, 5.0f);
            bCloudParamsChanged |= ImGui::SliderFloat("ShapeStrength", &cloudParams.ShapeStrength, 0.0f, 1.0f);
            bCloudParamsChanged |= ImGui::SliderFloat("DetailStrength", &cloudParams.DetailStrength, 0.0f, 1.0f);
            bCloudParamsChanged |= ImGui::SliderFloat("Density Multiplier", &cloudParams.DensityMult, 0.0f, 5.0f);
        }

        // --- Debug Views ---
        if (ImGui::CollapsingHeader("Noise Texture", ImGuiTreeNodeFlags_DefaultOpen))
        {
            // Show the noise texture being used
            ImGui::Image((void*)renderer.m_CloudMapSRV.Get(), ImVec2(204, 204));

            auto blueNoise = resMgr.GetTexture("BlueNoise");
            if (blueNoise != nullptr)
            {
                ImGui::SameLine();
                ImGui::Image(*blueNoise, ImVec2(64, 64));
            }
        }
    }

    ImGui::End();

    return bCloudParamsChanged;
}

void Gui::Render()
{
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