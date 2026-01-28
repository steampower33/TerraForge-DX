#pragma once

class Camera;

class Constant
{
public:
	// Nested type aliases to avoid global namespace pollution
	using Vector2 = DirectX::SimpleMath::Vector2;
	using Vector3 = DirectX::SimpleMath::Vector3;

public:
	struct GlobalConstants
	{
		Vector3 CameraPos;      // Camera Position in World Space
		float   Time;           // Elapsed Time (Packed into w channel)

		Vector3 CameraDir;      // Camera Look Direction
		float   Padding0;       // Padding for 16-byte alignment

		Vector3 CameraRight;    // Camera Right Vector
		float   Padding1;

		Vector3 CameraUp;       // Camera Up Vector
		float   Padding2;

		Vector2 Resolution;     // Viewport Resolution (Width, Height)
		Vector2 Padding3;       // Padding to fill 16-byte boundary
	} m_GlobalConstants;

	struct CloudConstants
	{
		Vector3 SunDir;
		float   SunIntensity;

		float   CloudScale;
		float   ShapeStrength;
		float   DetailStrength;
		float   DensityMult;
	} m_CloudConstants;

public:
	Constant() = default;
	~Constant() = default;

	// [Rule] System classes should NOT be copied.
	Constant(const Constant&) = delete;
	Constant& operator=(const Constant&) = delete;

	void Initialize(ID3D11Device* device, ID3D11DeviceContext* context);
	void UpdateGlobal(Camera& camera, float totalTime, float width, float height);
	void UpdateCloud();
	void BindConstantBuffer();

private:
	void CreateConstantBuffer();
	void InitData();

private:
	ID3D11Device* m_pDevice = nullptr;
	ID3D11DeviceContext* m_pContext = nullptr;
	ComPtr<ID3D11Buffer> m_GlobalConstantBuffer;
	ComPtr<ID3D11Buffer> m_CloudConstantBuffer;
};