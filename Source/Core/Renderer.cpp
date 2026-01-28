#include "Vertex.h"
#include "ResourceManager.h"

#include "Renderer.h"

void Renderer::Initialize(ID3D11Device* device, ID3D11DeviceContext* context, ResourceManager* pResMgr)
{
	m_pDevice = device;
	m_pContext = context;
	m_pResMgr = pResMgr;

	CreateShader();
	CreateTexture();
	CreateSamplerState();
	//CreateQuadVertexBuffer();
}

void Renderer::CreateShader()
{
	ID3DBlob* vsBlob = nullptr;
	ID3DBlob* psBlob = nullptr;
	ID3DBlob* csBlob = nullptr;

	if (SUCCEEDED(CompileShader(L"FullScreenVS.hlsl", "vs_5_0", &vsBlob)))
	{
		m_pDevice->CreateVertexShader(
			vsBlob->GetBufferPointer(),
			vsBlob->GetBufferSize(),
			nullptr,
			&m_FullScreenVS
		);

		//// Define Input Layout
		//D3D11_INPUT_ELEMENT_DESC layout[] =
		//{
		//	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		//};

		//m_pDevice->CreateInputLayout(layout, ARRAYSIZE(layout), vertexshaderCSO->GetBufferPointer(), vertexshaderCSO->GetBufferSize(), &m_InputLayout);

		//m_Stride = sizeof(Vertex);
	}

	if (SUCCEEDED(CompileShader(L"Distance2DPS.hlsl", "ps_5_0", &psBlob)))
	{
		m_pDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_Distance2DPS);
		psBlob->Release();
		psBlob = nullptr;
	}
	if (SUCCEEDED(CompileShader(L"Distance3DPS.hlsl", "ps_5_0", &psBlob)))
	{
		m_pDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_Distance3DPS);
		psBlob->Release();
		psBlob = nullptr;
	}

	if (SUCCEEDED(CompileShader(L"CloudPS.hlsl", "ps_5_0", &psBlob)))
	{
		m_pDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_CloudPS);
		psBlob->Release();
		psBlob = nullptr;
	}

	if (SUCCEEDED(CompileShader(L"NoiseBaker.hlsl", "cs_5_0", &csBlob)))
	{
		ThrowIfFailed(m_pDevice->CreateComputeShader(csBlob->GetBufferPointer(), csBlob->GetBufferSize(), nullptr, &m_NoiseBakerCS));
		csBlob->Release();
		csBlob = nullptr;
	}

	if (vsBlob) vsBlob->Release();
}

void Renderer::PrepareShader()
{
	m_pContext->VSSetShader(m_FullScreenVS.Get(), nullptr, 0);

	if (m_Scene.bDistance2D)
		m_pContext->PSSetShader(m_Distance2DPS.Get(), nullptr, 0);
	if (m_Scene.bDistance3D)
		m_pContext->PSSetShader(m_Distance3DPS.Get(), nullptr, 0);
	if (m_Scene.bCloud)
	{
		m_pContext->PSSetShader(m_CloudPS.Get(), nullptr, 0);
		m_pContext->PSSetShaderResources(0, 1, m_NoiseSRV.GetAddressOf());
		m_pContext->PSSetShaderResources(1, 1, m_pResMgr->GetTexture("BlueNoise"));
		m_pContext->PSSetSamplers(0, 1, m_LinearSampler.GetAddressOf());

		ID3D11SamplerState* samplers[] = { m_LinearSampler.Get(), m_PointSampler.Get() };
		m_pContext->PSSetSamplers(0, 2, samplers);
	}
	//m_pContext->IASetInputLayout(m_InputLayout.Get());

}

void Renderer::Render()
{
	UINT offset = 0;
	m_pContext->IASetVertexBuffers(0, 1, m_VertexBuffer.GetAddressOf(), &m_Stride, &offset);
	m_pContext->Draw(3, 0);
}

void Renderer::CreateQuadVertexBuffer()
{
	D3D11_BUFFER_DESC vertexbufferdesc = {};
	vertexbufferdesc.ByteWidth = sizeof(quad_vertices);
	vertexbufferdesc.Usage = D3D11_USAGE_IMMUTABLE; // Static data
	vertexbufferdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA vertexbufferSRD = { quad_vertices };

	m_pDevice->CreateBuffer(&vertexbufferdesc, &vertexbufferSRD, &m_VertexBuffer);
}

HRESULT Renderer::CompileShader(const std::wstring& filename, const std::string& profile, ID3DBlob** shaderBlob)
{
	ID3DBlob* errorBlob = nullptr;

	std::wstring path = L"Shaders/" + filename;

	HRESULT hr = D3DCompileFromFile(
		path.c_str(),
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"main",                 // Entry Point
		profile.c_str(),        // version
		0, 0,
		shaderBlob,             // result
		&errorBlob
	);

	if (FAILED(hr)) {
		if (errorBlob) {
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
			errorBlob->Release();
		}
		return hr;
	}

	if (errorBlob) errorBlob->Release();
	return hr;
}

void Renderer::CreateTexture()
{
	// Texture Descriptor setup
	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width = 204;
	texDesc.Height = 204;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; // High precision for noise data
	texDesc.SampleDesc.Count = 1;
	texDesc.Usage = D3D11_USAGE_DEFAULT; // GPU will both read and write
	texDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;

	// Create the Texture Resource
	ThrowIfFailed(m_pDevice->CreateTexture2D(&texDesc, nullptr, &m_NoiseTexture));

	// Create Unordered Access View (UAV) for Compute Shader writing
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = texDesc.Format;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	ThrowIfFailed(m_pDevice->CreateUnorderedAccessView(m_NoiseTexture.Get(), &uavDesc, &m_NoiseUAV));

	// Create Shader Resource View (SRV) for ImGui/Pixel Shader reading
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	ThrowIfFailed(m_pDevice->CreateShaderResourceView(m_NoiseTexture.Get(), &srvDesc, &m_NoiseSRV));
}

void Renderer::Bake3DNoise()
{
	// Early exit if essential resources are not initialized
	if (!m_NoiseBakerCS || !m_NoiseUAV) return;

	// 1. Bind the pre-compiled Compute Shader to the pipeline
	m_pContext->CSSetShader(m_NoiseBakerCS.Get(), nullptr, 0);

	// 2. Link the UAV (output buffer) to the u0 register
	// pUAVInitialCounts is typically nullptr for standard write operations
	m_pContext->CSSetUnorderedAccessViews(0, 1, m_NoiseUAV.GetAddressOf(), nullptr);

	// 3. Execute the Compute Shader (Dispatch)
	// For a 204x204 atlas with [numthreads(8, 8, 1)], 
	// we calculate: ceil(204 / 8) = 26 thread groups per axis
	UINT groupCount = (UINT)ceil(204.0f / 8.0f);
	m_pContext->Dispatch(groupCount, groupCount, 1);

	// 4. CRITICAL: Unbind the UAV after execution
	// This prevents "Resource Hazard" errors when the texture is later read as an SRV
	ID3D11UnorderedAccessView* nullUAV = nullptr;
	m_pContext->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);

	// Optional: Unbind the Compute Shader to maintain a clean pipeline state
	m_pContext->CSSetShader(nullptr, nullptr, 0);
}

void Renderer::CreateSamplerState()
{
	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR; // Enable linear interpolation
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;    // Essential for tileable noise
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

	// Create the sampler state
	HRESULT hr = m_pDevice->CreateSamplerState(&sampDesc, &m_LinearSampler);
	if (FAILED(hr)) {
		// Handle error
	}

	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	m_pDevice->CreateSamplerState(&sampDesc, &m_PointSampler);
}