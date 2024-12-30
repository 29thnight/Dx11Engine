#pragma once
#include "SimpleShaderDefine.h"
#include "DeviceResources.h"
// --------------------------------------------------------
// Base abstract class for simplifying shader handling
// --------------------------------------------------------
class ShaderResource abstract
{
public:
    ShaderResource(ID3D11Device* device, ID3D11DeviceContext* deviceContext);
    ShaderResource(const std::shared_ptr<DirectX11::DeviceResources>& resources);
    virtual ~ShaderResource();

    // Initialization method (since we can't invoke derived class
    // overrides in the base class constructor)
    bool LoadShaderFile(file::path shaderFile);

    // Simple helpers
    bool IsShaderValid() const { return shaderValid; }

    // Activating the shader and copying data
    void SetShader();
    void CopyAllBufferData();
    void CopyBufferData(unsigned int index);
    void CopyBufferData(std::string bufferName);

    template<typename T>
    bool SetData(std::string name, const T& data);

    template<typename T, size_t N>
    bool SetData(std::string name, const T data[N]);

    // Setting shader resources
    virtual bool SetShaderResourceView(std::string name, ID3D11ShaderResourceView* srv) = 0;
    virtual bool SetSamplerState(std::string name, ID3D11SamplerState* samplerState) = 0;

    // Getting data about variables and resources
    const ConstantBufferVariable* GetVariableInfo(std::string name);

    const ShaderResourceViewIndex* GetShaderResourceViewInfo(std::string name);
    const ShaderResourceViewIndex* GetShaderResourceViewInfo(unsigned int index);
    unsigned int GetShaderResourceViewCount() { return static_cast<unsigned int>(textureTable.size()); }

    const ShaderSampler* GetSamplerInfo(std::string name);
    const ShaderSampler* GetSamplerInfo(unsigned int index);
    unsigned int GetSamplerCount() { return static_cast<unsigned int>(samplerTable.size()); }

    // Get data about constant buffers
    unsigned int GetBufferCount() const;
    unsigned int GetBufferSize(unsigned int index);
    const ShaderConstantBuffer* GetBufferInfo(std::string name);
    const ShaderConstantBuffer* GetBufferInfo(unsigned int index);

    // Misc getters
    ID3DBlob* GetShaderBlob() { return shaderBlob; }

protected:
    // Pure virtual functions for dealing with shader types
    virtual bool CreateShader(ID3DBlob* shaderBlob) = 0;
    virtual void SetShaderAndCBs() = 0;

    virtual void CleanUp();

    // Helpers for finding data by name
    ConstantBufferVariable* FindVariable(std::string name, int size);
    ShaderConstantBuffer* FindConstantBuffer(std::string name);

protected:
    bool shaderValid;
    ID3DBlob* shaderBlob;
    ID3D11Device* device;
    ID3D11DeviceContext* deviceContext;

    // Resource counts
    unsigned int constantBufferCount;

    // Maps for variables and buffers
    ShaderConstantBuffer* constantBuffers; // For index-based lookup
    std::vector<ShaderResourceViewIndex*> shaderResourceViews;
    std::vector<ShaderSampler*>	samplerStates;
    std::unordered_map<std::string, ShaderConstantBuffer*> cbTable;
    std::unordered_map<std::string, ConstantBufferVariable> varTable;
    std::unordered_map<std::string, ShaderResourceViewIndex*> textureTable;
    std::unordered_map<std::string, ShaderSampler*> samplerTable;

};

#include "ShaderResource.inl"
