#pragma once

#include <SimpleMath.h>

class Camera
{
public:
    // Type aliasing for better readability and maintainability
    using Vector3 = DirectX::SimpleMath::Vector3;
    using Matrix = DirectX::SimpleMath::Matrix;

public:
    Camera();

    void Initialize(float aspectRatio);
    void Update(float dt);

    Matrix GetViewMatrix() const;
    Matrix GetProjectionMatrix() const;

    // Camera basis vectors using SimpleMath types
    Vector3 m_Pos{ 0.0f, 0.0f, -20.0f };
    Vector3 m_LookDir{ 0.0f, 0.0f, 1.0f };
    Vector3 m_UpDir{ 0.0f, 1.0f, 0.0f };
    Vector3 m_RightDir{ 1.0f, 0.0f, 0.0f };

private:
    void ProcessKeyboard(float dt);
    void ProcessMouse(float dt);

private:
    float m_Yaw = 0.0f;
    float m_Pitch = 0.0f;

    float m_AspectRatio = 1.777f;
    float m_Fov = 90.0f;
    float m_NearZ = 0.1f;
    float m_FarZ = 1000.0f;

    float m_MoveSpeed = 10.0f;
    float m_MouseSensitivity = 5.0f;

    POINT m_LastMousePos{ 0, 0 };
};