#pragma once

class ResourceManager
{
public:
    ResourceManager() {}
    ~ResourceManager() {}

    // [Rule] System classes should NOT be copied.
    // Copying a core system creates ambiguity in resource ownership.
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    void Initialize(ID3D11Device* device);

    bool LoadTexture(const std::string& name, const std::wstring& path);
    ID3D11ShaderResourceView** GetTexture(const std::string& name);

private:
    ID3D11Device* m_pDevice = nullptr;
    std::unordered_map<std::string, ComPtr<ID3D11ShaderResourceView>> m_TextureMap;
};

