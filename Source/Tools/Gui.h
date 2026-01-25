#pragma once

class Constant;
class Camera;
class Renderer;

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

    void Render(Constant& constant, Camera& camera, Renderer& renderer, float totalTime);

private:
    void SetStyle();
};