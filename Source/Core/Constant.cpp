#include "Camera.h"

#include "Constant.h"

using namespace DirectX;

void Constant::UpdateGlobal(Camera& camera, float totalTime, float width, float height)
{
	if (!m_GlobalConstantBuffer) return;

	m_GlobalConstants.Time = totalTime;
	m_GlobalConstants.Resolution = Vector2(width, height);
	m_GlobalConstants.CameraPos = camera.m_Pos;
	m_GlobalConstants.CameraDir = camera.m_LookDir;
	m_GlobalConstants.CameraRight = camera.m_RightDir;
	m_GlobalConstants.CameraUp = camera.m_UpDir;

	D3D11_MAPPED_SUBRESOURCE msr;
	if (SUCCEEDED(m_pContext->Map(m_GlobalConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr)))
	{
		memcpy(msr.pData, &m_GlobalConstants, sizeof(GlobalConstants));
		m_pContext->Unmap(m_GlobalConstantBuffer.Get(), 0);
	}
}

void Constant::UpdateCloud()
{
	if (!m_CloudConstantBuffer) return;

	D3D11_MAPPED_SUBRESOURCE msr;
	if (SUCCEEDED(m_pContext->Map(m_CloudConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr)))
	{
		memcpy(msr.pData, &m_CloudConstants, sizeof(CloudConstants));
		m_pContext->Unmap(m_CloudConstantBuffer.Get(), 0);
	}
}

void Constant::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
	m_pDevice = device;
	m_pContext = context;

	InitData();
	CreateConstantBuffer();
	UpdateCloud();
}

void Constant::CreateConstantBuffer()
{
	{
		D3D11_BUFFER_DESC constantbufferdesc = {};
		// Ensure size is a multiple of 16 bytes for HLSL compatibility
		constantbufferdesc.ByteWidth = sizeof(GlobalConstants);
		constantbufferdesc.Usage = D3D11_USAGE_DYNAMIC; // Updated every frame from CPU
		constantbufferdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		constantbufferdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		HRESULT hr = m_pDevice->CreateBuffer(&constantbufferdesc, nullptr, &m_GlobalConstantBuffer);
		if (FAILED(hr))
		{
			OutputDebugStringA("[Error] Constant Buffer is NULL!\n");
		}
	}

	{
		D3D11_BUFFER_DESC constantbufferdesc = {};
		// Ensure size is a multiple of 16 bytes for HLSL compatibility
		constantbufferdesc.ByteWidth = sizeof(CloudConstants);
		constantbufferdesc.Usage = D3D11_USAGE_DYNAMIC; // Updated every frame from CPU
		constantbufferdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		constantbufferdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		HRESULT hr = m_pDevice->CreateBuffer(&constantbufferdesc, nullptr, &m_CloudConstantBuffer);
		if (FAILED(hr))
		{
			OutputDebugStringA("[Error] Constant Buffer is NULL!\n");
		}
	}
}

void Constant::BindConstantBuffer()
{
	m_pContext->PSSetConstantBuffers(0, 1, m_GlobalConstantBuffer.GetAddressOf());
	m_pContext->PSSetConstantBuffers(1, 1, m_CloudConstantBuffer.GetAddressOf());
}

void Constant::InitData()
{
	m_GlobalConstants.Time = 0.0f;

	m_CloudConstants.SunDir = Vector3(0.6f, 0.35f, 0.6f);
	m_CloudConstants.SunDir.Normalize();
	m_CloudConstants.SunIntensity = 200.0f;

	m_CloudConstants.CloudScale = 2.5f;
	m_CloudConstants.ShapeStrength = 0.6f;
	m_CloudConstants.DetailStrength = 0.35f;
	m_CloudConstants.DensityMult = 1.0f;
}