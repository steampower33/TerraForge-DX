#include "Camera.h"

#include "Constant.h"

using namespace DirectX;

void Constant::UpdateConstant(Camera& camera, float deltaTime, float totalTime, float width, float height)
{
	if (!m_ConstantBuffer)
	{
		OutputDebugStringA("[Error] Constant Buffer is NULL!\n");
		return;
	}

	// Update local struct first
	m_Constants.Time = totalTime;
	m_Constants.Resolution = Vector2(width, height);

	m_Constants.CameraPos = camera.m_Pos;
	m_Constants.CameraForward = camera.m_LookDir;
	m_Constants.CameraRight = camera.m_RightDir;
	m_Constants.CameraUp = Vector3(0.0f, 1.0f, 0.0f);

	// Map to GPU
	D3D11_MAPPED_SUBRESOURCE msr;
	if (SUCCEEDED(m_Context->Map(m_ConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr)))
	{
		memcpy(msr.pData, &m_Constants, sizeof(Constants));
		m_Context->Unmap(m_ConstantBuffer.Get(), 0);
	}
}

void Constant::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
	m_Device = device;
	m_Context = context;

	InitData();
	CreateConstantBuffer();
}

void Constant::CreateConstantBuffer()
{
	D3D11_BUFFER_DESC constantbufferdesc = {};
	// Ensure size is a multiple of 16 bytes for HLSL compatibility
	constantbufferdesc.ByteWidth = sizeof(Constants);
	constantbufferdesc.Usage = D3D11_USAGE_DYNAMIC; // Updated every frame from CPU
	constantbufferdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	constantbufferdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	HRESULT hr = m_Device->CreateBuffer(&constantbufferdesc, nullptr, &m_ConstantBuffer);
	if (FAILED(hr))
	{
		OutputDebugStringA("[Error] Constant Buffer is NULL!\n");
	}
}

void Constant::BindConstantBuffer()
{
	m_Context->PSSetConstantBuffers(0, 1, m_ConstantBuffer.GetAddressOf());
}
void Constant::InitData()
{
	m_Constants.Time = 0.0f;
	m_Constants.StepSize = 0.1f;
	m_Constants.CloudScale = 1.2f;
	m_Constants.CloudThreshold = 0.1f;
	m_Constants.Absorption = 0.5f;
	m_Constants.FogDensity = 0.01f;

	m_Constants.SunDir = Vector3(0.577f, -0.577f, 0.577f);
	m_Constants.SunColor = Vector3(1.0f, 0.95f, 0.85f);
	m_Constants.FogColor = Vector3(0.5f, 0.6f, 0.7f);

	m_Constants.padding1 = 0.0f;
	m_Constants.padding2 = 0.0f;
	m_Constants.padding3 = 0.0f;
}