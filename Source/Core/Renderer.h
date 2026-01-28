#pragma once

class ResourceManager;

class Renderer
{
public:
	Renderer() {}
	~Renderer() {}

	// [Rule] System classes should NOT be copied.
	// Copying a core system creates ambiguity in resource ownership.
	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;

	void Initialize(ID3D11Device* device, ID3D11DeviceContext* context, ResourceManager* ResMgr);
	void PrepareShader();
	void Render();

	struct Scene {
		bool bDistance2D = false;
		bool bDistance3D = false;
		bool bCloud = true;
	} m_Scene;

private:
	ID3D11Device* m_pDevice = nullptr;
	ID3D11DeviceContext* m_pContext = nullptr;
	ResourceManager* m_pResMgr = nullptr;

private:
	void CreateShader();
	ComPtr<ID3D11VertexShader> m_FullScreenVS;

	ComPtr<ID3D11PixelShader> m_Distance2DPS;
	ComPtr<ID3D11PixelShader> m_Distance3DPS;
	ComPtr<ID3D11PixelShader> m_CloudPS;

	ComPtr<ID3D11ComputeShader> m_NoiseBakerCS;

	ComPtr<ID3D11InputLayout> m_InputLayout;
	unsigned int m_Stride;

	void CreateQuadVertexBuffer();
	ComPtr<ID3D11Buffer> m_VertexBuffer;

	HRESULT CompileShader(const std::wstring& filename, const std::string& profile, ID3DBlob** shaderBlob);

	void CreateTexture();

	ComPtr<ID3D11Texture2D> m_NoiseTexture;
	ComPtr<ID3D11UnorderedAccessView> m_NoiseUAV;

	void CreateSamplerState();
	ComPtr<ID3D11SamplerState> m_LinearSampler;
	ComPtr<ID3D11SamplerState> m_PointSampler;

public:
	ComPtr<ID3D11ShaderResourceView> m_NoiseSRV;
	void Bake3DNoise();
};