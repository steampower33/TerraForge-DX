#include "Camera.h"

Camera::Camera()
{
    GetCursorPos(&m_LastMousePos);
}

void Camera::Initialize(float aspectRatio, HWND hWnd)
{
    m_AspectRatio = aspectRatio;
    m_hWnd = hWnd;
    Update(0.0f);
}

void Camera::Update(float dt)
{
    ProcessMouse(dt);
    ProcessKeyboard(dt);
}

void Camera::ProcessKeyboard(float dt)
{
    if (m_hWnd != nullptr && GetForegroundWindow() != m_hWnd)
        return;

    float currentSpeed = m_MoveSpeed * dt;

    if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
        currentSpeed *= 4.0f;

    // Direct vector arithmetic with SimpleMath
    if (GetAsyncKeyState('W') & 0x8000) m_Pos += m_LookDir * currentSpeed;
    if (GetAsyncKeyState('S') & 0x8000) m_Pos -= m_LookDir * currentSpeed;
    if (GetAsyncKeyState('D') & 0x8000) m_Pos += m_RightDir * currentSpeed;
    if (GetAsyncKeyState('A') & 0x8000) m_Pos -= m_RightDir * currentSpeed;
    if (GetAsyncKeyState('E') & 0x8000) m_Pos += Vector3(0, 1, 0) * currentSpeed;
    if (GetAsyncKeyState('Q') & 0x8000) m_Pos -= Vector3(0, 1, 0) * currentSpeed;
}

void Camera::ProcessMouse(float dt)
{
    if (m_hWnd != nullptr && GetForegroundWindow() != m_hWnd)
    {
        GetCursorPos(&m_LastMousePos);
        return;
    }

    if (GetAsyncKeyState(VK_RBUTTON) & 0x8000)
    {
        POINT currentPos;
        GetCursorPos(&currentPos);

        float deltaX = static_cast<float>(currentPos.x - m_LastMousePos.x);
        float deltaY = static_cast<float>(currentPos.y - m_LastMousePos.y);

        float sens = m_MouseSensitivity * 0.001f;
        m_Yaw += deltaX * sens;
        m_Pitch += deltaY * sens;

        m_Pitch = std::clamp(m_Pitch, -DirectX::XM_PIDIV2 + 0.01f, DirectX::XM_PIDIV2 - 0.01f);
    }

    GetCursorPos(&m_LastMousePos);

    Matrix rotation = Matrix::CreateFromYawPitchRoll(m_Yaw, m_Pitch, 0.0f);

    m_LookDir = Vector3::TransformNormal(Vector3(0, 0, 1), rotation);
    m_RightDir = Vector3::TransformNormal(Vector3(1, 0, 0), rotation);
    m_UpDir = Vector3::TransformNormal(Vector3(0, 1, 0), rotation);

    m_LookDir.Normalize();
    m_RightDir.Normalize();
    m_UpDir.Normalize();
}

Camera::Matrix Camera::GetViewMatrix() const
{
    return Matrix::CreateLookAt(m_Pos, m_Pos + m_LookDir, m_UpDir);
}

Camera::Matrix Camera::GetProjectionMatrix() const
{
    return Matrix::CreatePerspectiveFieldOfView(DirectX::XMConvertToRadians(m_Fov), m_AspectRatio, m_NearZ, m_FarZ);
}