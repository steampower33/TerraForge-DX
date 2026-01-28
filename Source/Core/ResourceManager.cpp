#include "ResourceManager.h"

void ResourceManager::Initialize(ID3D11Device* device)
{
    m_Device = device;
}

bool ResourceManager::LoadTexture(const std::string& name, const std::wstring& path)
{
    if (m_TextureMap.find(name) != m_TextureMap.end())
    {
        return true;
    }

    ComPtr<ID3D11ShaderResourceView> srv;
    HRESULT hr = DirectX::CreateWICTextureFromFile(m_Device, nullptr, path.c_str(), nullptr, srv.GetAddressOf());

    if (FAILED(hr))
    {
        OutputDebugStringA(("Failed to load texture: " + name + "\n").c_str());
        return false;
    }

    m_TextureMap[name] = srv;
    return true;
}

ID3D11ShaderResourceView** ResourceManager::GetTexture(const std::string& name)
{
    if (m_TextureMap.find(name) != m_TextureMap.end())
    {
        return m_TextureMap[name].GetAddressOf();
    }
    return nullptr;
}