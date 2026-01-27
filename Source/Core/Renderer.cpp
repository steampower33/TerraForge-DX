#include "Vertex.h"

#include "Renderer.h"

void Renderer::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
	m_Device = device;
	m_Context = context;

	CreateShader();
	CreateTexture();
	//CreateQuadVertexBuffer();
}

void Renderer::CreateShader()
{
	ID3DBlob* vsBlob = nullptr;
	ID3DBlob* psBlob = nullptr;
	ID3DBlob* csBlob = nullptr;

	if (SUCCEEDED(CompileShader(L"FullScreenVS.hlsl", "vs_5_0", &vsBlob)))
	{
		m_Device->CreateVertexShader(
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

		//m_Device->CreateInputLayout(layout, ARRAYSIZE(layout), vertexshaderCSO->GetBufferPointer(), vertexshaderCSO->GetBufferSize(), &m_InputLayout);

		//m_Stride = sizeof(Vertex);
	}

	if (SUCCEEDED(CompileShader(L"Distance2DPS.hlsl", "ps_5_0", &psBlob)))
	{
		m_Device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_Distance2DPS);
		psBlob->Release();
		psBlob = nullptr;
	}
	if (SUCCEEDED(CompileShader(L"Distance3DPS.hlsl", "ps_5_0", &psBlob)))
	{
		m_Device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_Distance3DPS);
		psBlob->Release();
		psBlob = nullptr;
	}

	if (SUCCEEDED(CompileShader(L"CloudPS.hlsl", "ps_5_0", &psBlob)))
	{
		m_Device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_CloudPS);
		psBlob->Release();
		psBlob = nullptr;
	}

	if (SUCCEEDED(CompileShader(L"NoiseBaker.hlsl", "cs_5_0", &csBlob)))
	{
		ThrowIfFailed(m_Device->CreateComputeShader(csBlob->GetBufferPointer(), csBlob->GetBufferSize(), nullptr, &m_NoiseBakerCS));
		csBlob->Release();
		csBlob = nullptr;
	}

	if (vsBlob) vsBlob->Release();
}

void Renderer::PrepareShader()
{
	m_Context->VSSetShader(m_FullScreenVS.Get(), nullptr, 0);

	if (m_Scene.bDistance2D)
		m_Context->PSSetShader(m_Distance2DPS.Get(), nullptr, 0);
	if (m_Scene.bDistance3D)
		m_Context->PSSetShader(m_Distance3DPS.Get(), nullptr, 0);
	if (m_Scene.bCloud)
		m_Context->PSSetShader(m_CloudPS.Get(), nullptr, 0);
	//m_Context->IASetInputLayout(m_InputLayout.Get());
}

void Renderer::Render()
{
	UINT offset = 0;
	m_Context->IASetVertexBuffers(0, 1, m_VertexBuffer.GetAddressOf(), &m_Stride, &offset);
	m_Context->Draw(3, 0);
}

void Renderer::CreateQuadVertexBuffer()
{
	D3D11_BUFFER_DESC vertexbufferdesc = {};
	vertexbufferdesc.ByteWidth = sizeof(quad_vertices);
	vertexbufferdesc.Usage = D3D11_USAGE_IMMUTABLE; // Static data
	vertexbufferdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA vertexbufferSRD = { quad_vertices };

	m_Device->CreateBuffer(&vertexbufferdesc, &vertexbufferSRD, &m_VertexBuffer);
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
	ThrowIfFailed(m_Device->CreateTexture2D(&texDesc, nullptr, &m_NoiseTexture));

	// Create Unordered Access View (UAV) for Compute Shader writing
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = texDesc.Format;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	ThrowIfFailed(m_Device->CreateUnorderedAccessView(m_NoiseTexture.Get(), &uavDesc, &m_NoiseUAV));

	// Create Shader Resource View (SRV) for ImGui/Pixel Shader reading
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	ThrowIfFailed(m_Device->CreateShaderResourceView(m_NoiseTexture.Get(), &srvDesc, &m_NoiseSRV));
}

void Renderer::BakeNoise()
{
	// Early exit if essential resources are not initialized
	if (!m_NoiseBakerCS || !m_NoiseUAV) return;

	// 1. Bind the pre-compiled Compute Shader to the pipeline
	m_Context->CSSetShader(m_NoiseBakerCS.Get(), nullptr, 0);

	// 2. Link the UAV (output buffer) to the u0 register
	// pUAVInitialCounts is typically nullptr for standard write operations
	m_Context->CSSetUnorderedAccessViews(0, 1, m_NoiseUAV.GetAddressOf(), nullptr);

	// 3. Execute the Compute Shader (Dispatch)
	// For a 204x204 atlas with [numthreads(8, 8, 1)], 
	// we calculate: ceil(204 / 8) = 26 thread groups per axis
	UINT groupCount = (UINT)ceil(204.0f / 8.0f);
	m_Context->Dispatch(groupCount, groupCount, 1);

	// 4. CRITICAL: Unbind the UAV after execution
	// This prevents "Resource Hazard" errors when the texture is later read as an SRV
	ID3D11UnorderedAccessView* nullUAV = nullptr;
	m_Context->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);

	// Optional: Unbind the Compute Shader to maintain a clean pipeline state
	m_Context->CSSetShader(nullptr, nullptr, 0);
}