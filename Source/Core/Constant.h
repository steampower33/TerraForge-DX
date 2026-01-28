#pragma once

#include <wrl.h>
#include <d3d11.h>
#include <SimpleMath.h>

class Camera;

class Constant
{
public:
    // Nested type aliases to avoid global namespace pollution
    using Vector2 = DirectX::SimpleMath::Vector2;
    using Vector3 = DirectX::SimpleMath::Vector3;

public:
    struct Constants
    {
        float   Time;              // Elapsed time in seconds
        Vector2 Resolution;        // Viewport resolution (Width, Height)
        float   StepSize;          // Raymarching step size

        Vector3 CameraPos;         // World space camera position
        float   CloudScale;        // Noise frequency for cloud shaping

        Vector3 CameraForward;     // Camera look direction
        float   CloudThreshold;    // Cloud coverage cutoff

        Vector3 CameraRight;       // Camera right vector
        float   Absorption;        // Light absorption coefficient

        Vector3 CameraUp;          // Camera up vector
        float   FogDensity;        // Global fog density

        Vector3 SunDir;            // Direction to the sun
        float   padding1;           // 16-byte alignment padding

        Vector3 SunColor;          // RGB color of the sunlight
        float   padding2;           // 16-byte alignment padding

        Vector3 FogColor;          // RGB color of the atmospheric fog
        float   padding3;           // 16-byte alignment padding
    };

public:
    Constant() = default;
    ~Constant() = default;

    // [Rule] System classes should NOT be copied.
    Constant(const Constant&) = delete;
    Constant& operator=(const Constant&) = delete;

    void Initialize(ID3D11Device* device, ID3D11DeviceContext* context);
    void UpdateConstant(Camera& camera, float deltaTime, float totalTime, float width, float height);
    void BindConstantBuffer();

    Constants m_Constants;

private:
    void CreateConstantBuffer();
    void InitData();

private:
    ID3D11Device* m_pDevice = nullptr;
    ID3D11DeviceContext* m_pContext = nullptr;
    ComPtr<ID3D11Buffer> m_ConstantBuffer;
};