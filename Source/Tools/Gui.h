#pragma once

class Constant;
class Camera;
class Renderer;
class ResourceManager;

class Gui
{
public:
    Gui() {}
    ~Gui();

    // [Rule] System classes should NOT be copied.
    // Copying a core system creates ambiguity in resource ownership.
    Gui(const Gui&) = delete;
    Gui& operator=(const Gui&) = delete;

    void Initialize(HWND hWnd, ID3D11Device* device, ID3D11DeviceContext* context);

    bool Update(float totalTime, Constant& constant, Camera& camera, Renderer& renderer, ResourceManager& resMgr);

    void Render();

private:
    void SetStyle();
};