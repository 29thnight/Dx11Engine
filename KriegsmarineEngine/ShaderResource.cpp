#include "ShaderResource.h"
#include "Core.Memory.h"
#include <DirectXMath.h>

///////////////////////////////////////////////////////////////////////////////
// ------ BASE SIMPLE SHADER --------------------------------------------------
///////////////////////////////////////////////////////////////////////////////

ShaderResource::ShaderResource(ID3D11Device* device, ID3D11DeviceContext* deviceContext) :
    device(device),
    deviceContext(deviceContext),
    shaderBlob(nullptr),
    shaderValid(false),
    constantBufferCount(0),
    constantBuffers(nullptr)
{
}

// --------------------------------------------------------
// Constructor accepts DirectX device & context
// --------------------------------------------------------
ShaderResource::ShaderResource(const std::shared_ptr<DirectX11::DeviceResources>& resources) :
    device(resources->GetD3DDevice()),
    deviceContext(resources->GetD3DDeviceContext()),
    shaderBlob(nullptr),
    shaderValid(false),
    constantBufferCount(0),
    constantBuffers(nullptr)
{
}

// --------------------------------------------------------
// Destructor
// --------------------------------------------------------
ShaderResource::~ShaderResource()
{
    // Derived class destructors will call this class's CleanUp method
    Memory::SafeDelete(shaderBlob);
}

// --------------------------------------------------------
// Cleans up the variable table and buffers - Some things will
// be handled by derived classes
// --------------------------------------------------------
void ShaderResource::CleanUp()
{
    // Handle constant buffers and local data buffers
    for (unsigned int i = 0; i < constantBufferCount; i++)
    {
        Memory::SafeDelete(constantBuffers[i].ConstantBuffer);
        Memory::SafeDeleteArray(constantBuffers[i].LocalDataBuffer);
    }

    Memory::SafeDeleteArray(constantBuffers);
    constantBufferCount = 0;

    {
        DeferredDeleter deleter(&shaderResourceViews);
        DeferredDeleter deleter2(&samplerStates);
    }

    // Clean up tables
    varTable.clear();
    cbTable.clear();
    samplerTable.clear();
    textureTable.clear();
}

// --------------------------------------------------------
// Loads the specified shader and builds the variable table using shader
// reflection.  This must be a separate step from the constructor since
// we can't invoke derived class overrides in the base class constructor.
//
// shaderFile - A "wide string" specifying the compiled shader to load
// 
// Returns true if shader is loaded properly, false otherwise
// --------------------------------------------------------
bool ShaderResource::LoadShaderFile(file::path shaderFile)
{
    // Load the shader to a blob and ensure it worked
    HRESULT hr = D3DReadFileToBlob(shaderFile.c_str(), &shaderBlob);
    if (hr != S_OK)
    {
        return false;
    }

    // Create the shader - Calls an overloaded version of this abstract
    // method in the appropriate child class
    shaderValid = CreateShader(shaderBlob);
    if (!shaderValid)
    {
        return false;
    }

    // Set up shader reflection to get information about
    // this shader and its variables,  buffers, etc.
    ID3D11ShaderReflection* refl{};
    D3DReflect(
        shaderBlob->GetBufferPointer(),
        shaderBlob->GetBufferSize(),
        IID_ID3D11ShaderReflection,
        (void**)&refl);

    // Get the description of the shader
    D3D11_SHADER_DESC shaderDesc;
    refl->GetDesc(&shaderDesc);

    // Create resource arrays
    constantBufferCount = shaderDesc.ConstantBuffers;
    constantBuffers = new ShaderConstantBuffer[constantBufferCount];

    // Handle bound resources (like shaders and samplers)
    unsigned int resourceCount = shaderDesc.BoundResources;
    for (unsigned int r = 0; r < resourceCount; r++)
    {
        // Get this resource's description
        D3D11_SHADER_INPUT_BIND_DESC resourceDesc;
        refl->GetResourceBindingDesc(r, &resourceDesc);

        // Check the type
        switch (resourceDesc.Type)
        {
        case D3D_SIT_TEXTURE: // A texture resource
        {
            // Create the SRV wrapper
            ShaderResourceViewIndex* srv = new ShaderResourceViewIndex();
            srv->BindIndex = resourceDesc.BindPoint;	// Shader bind point
            srv->Index = static_cast<uint32>(shaderResourceViews.size());	// Raw index

            textureTable.insert(std::pair<std::string, ShaderResourceViewIndex*>(resourceDesc.Name, srv));
            shaderResourceViews.push_back(srv);
        }
        break;

        case D3D_SIT_SAMPLER: // A sampler resource
        {
            // Create the sampler wrapper
            ShaderSampler* samp = new ShaderSampler();
            samp->BindIndex = resourceDesc.BindPoint;	// Shader bind point
            samp->Index = static_cast<uint32>(samplerStates.size());			// Raw index

            samplerTable.insert(std::pair<std::string, ShaderSampler*>(resourceDesc.Name, samp));
            samplerStates.push_back(samp);
        }
        break;
        }
    }

    // Loop through all constant buffers
    for (unsigned int b = 0; b < constantBufferCount; b++)
    {
        // Get this buffer
        ID3D11ShaderReflectionConstantBuffer* cb =
            refl->GetConstantBufferByIndex(b);

        // Get the description of this buffer
        D3D11_SHADER_BUFFER_DESC bufferDesc;
        cb->GetDesc(&bufferDesc);

        // Get the description of the resource binding, so
        // we know exactly how it's bound in the shader
        D3D11_SHADER_INPUT_BIND_DESC bindDesc;
        refl->GetResourceBindingDescByName(bufferDesc.Name, &bindDesc);

        // Set up the buffer and put its pointer in the table
        constantBuffers[b].BindIndex = bindDesc.BindPoint;
        constantBuffers[b].Name = bufferDesc.Name;
        cbTable.insert(std::pair<std::string, ShaderConstantBuffer*>(bufferDesc.Name, &constantBuffers[b]));

        // Create this constant buffer
        D3D11_BUFFER_DESC newBuffDesc{};
        newBuffDesc.Usage = D3D11_USAGE_DEFAULT;
        newBuffDesc.ByteWidth = bufferDesc.Size;
        newBuffDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        newBuffDesc.CPUAccessFlags = 0;
        newBuffDesc.MiscFlags = 0;
        newBuffDesc.StructureByteStride = 0;
        device->CreateBuffer(&newBuffDesc, 0, &constantBuffers[b].ConstantBuffer);

        // Set up the data buffer for this constant buffer
        constantBuffers[b].Size = bufferDesc.Size;
        constantBuffers[b].LocalDataBuffer = new byte[bufferDesc.Size];
        ZeroMemory(constantBuffers[b].LocalDataBuffer, bufferDesc.Size);

        // Loop through all variables in this buffer
        for (unsigned int v = 0; v < bufferDesc.Variables; v++)
        {
            // Get this variable
            ID3D11ShaderReflectionVariable* var = cb->GetVariableByIndex(v);
            ID3D11ShaderReflectionType* varType = var->GetType();

            D3D11_SHADER_VARIABLE_DESC varDesc;

            // Get the description of the variable and its type
            var->GetDesc(&varDesc);

            // Create the variable struct
            ConstantBufferVariable varStruct{};
            varStruct.ConstantBufferIndex = b;
            varStruct.ByteOffset = varDesc.StartOffset;
            varStruct.Size = varDesc.Size;

            // Get a string version
            std::string varName(varDesc.Name);

            // Add this variable to the table and the constant buffer
            varTable.insert(std::pair<std::string, ConstantBufferVariable>(varName, varStruct));
            constantBuffers[b].Variables.push_back(varStruct);
        }
    }

    // All set
    refl->Release();
    return true;
}

// --------------------------------------------------------
// Helper for looking up a variable by name and also
// verifying that it is the requested size
// 
// name - the name of the variable to look for
// size - the size of the variable (for verification), or -1 to bypass
// --------------------------------------------------------
ConstantBufferVariable* ShaderResource::FindVariable(std::string name, int size)
{
    // Look for the key
    std::unordered_map<std::string, ConstantBufferVariable>::iterator result =
        varTable.find(name);

    // Did we find the key?
    if (result == varTable.end())
        return 0;

    // Grab the result from the iterator
    ConstantBufferVariable* var = &(result->second);

    // Is the data size correct ?
    if (size > 0 && var->Size != size)
        return 0;

    // Success
    return var;
}

// --------------------------------------------------------
// Helper for looking up a constant buffer by name
// --------------------------------------------------------
ShaderConstantBuffer* ShaderResource::FindConstantBuffer(std::string name)
{
    // Look for the key
    std::unordered_map<std::string, ShaderConstantBuffer*>::iterator result =
        cbTable.find(name);

    // Did we find the key?
    if (result == cbTable.end())
        return 0;

    // Success
    return result->second;
}

// --------------------------------------------------------
// Sets the shader and associated constant buffers in DirectX
// --------------------------------------------------------
void ShaderResource::SetShader()
{
    // Ensure the shader is valid
    if (!shaderValid) return;

    // Set the shader and any relevant constant buffers, which
    // is an overloaded method in a subclass
    SetShaderAndCBs();
}

// --------------------------------------------------------
// Copies the relevant data to the all of this 
// shader's constant buffers.  To just copy one
// buffer, use CopyBufferData()
// --------------------------------------------------------
void ShaderResource::CopyAllBufferData()
{
    // Ensure the shader is valid
    if (!shaderValid) return;

    // Loop through the constant buffers and copy all data
    for (unsigned int i = 0; i < constantBufferCount; i++)
    {
        // Copy the entire local data buffer
        deviceContext->UpdateSubresource(
            constantBuffers[i].ConstantBuffer, 0, 0,
            constantBuffers[i].LocalDataBuffer, 0, 0);
    }
}

// --------------------------------------------------------
// Copies local data to the shader's specified constant buffer
//
// index - The index of the buffer to copy.
//         Useful for updating more frequently-changing
//         variables without having to re-copy all buffers.
//  
// NOTE: The "index" of the buffer might NOT be the same
//       as its register, especially if you have buffers
//       bound to non-sequential registers!
// --------------------------------------------------------
void ShaderResource::CopyBufferData(unsigned int index)
{
    // Ensure the shader is valid
    if (!shaderValid) return;

    // Validate the index
    if (index >= this->constantBufferCount)
        return;

    // Check for the buffer
    ShaderConstantBuffer* cb = &this->constantBuffers[index];
    if (!cb) return;

    // Copy the data and get out
    deviceContext->UpdateSubresource(
        cb->ConstantBuffer, 0, 0,
        cb->LocalDataBuffer, 0, 0);
}

// --------------------------------------------------------
// Copies local data to the shader's specified constant buffer
//
// bufferName - Specifies the name of the buffer to copy.
//              Useful for updating more frequently-changing
//              variables without having to re-copy all buffers.
// --------------------------------------------------------
void ShaderResource::CopyBufferData(std::string bufferName)
{
    // Ensure the shader is valid
    if (!shaderValid) return;

    // Check for the buffer
    ShaderConstantBuffer* cb = this->FindConstantBuffer(bufferName);
    if (!cb) return;

    // Copy the data and get out
    deviceContext->UpdateSubresource(
        cb->ConstantBuffer, 0, 0,
        cb->LocalDataBuffer, 0, 0);
}

// --------------------------------------------------------
// Gets info about a shader variable, if it exists
// --------------------------------------------------------
const ConstantBufferVariable* ShaderResource::GetVariableInfo(std::string name)
{
    return FindVariable(name, -1);
}

// --------------------------------------------------------
// Gets info about an SRV in the shader (or null)
//
// name - the name of the SRV
// --------------------------------------------------------
const ShaderResourceViewIndex* ShaderResource::GetShaderResourceViewInfo(std::string name)
{
    // Look for the key
    std::unordered_map<std::string, ShaderResourceViewIndex*>::iterator result =
        textureTable.find(name);

    // Did we find the key?
    if (result == textureTable.end())
        return 0;

    // Success
    return result->second;
}


// --------------------------------------------------------
// Gets info about an SRV in the shader (or null)
//
// index - the index of the SRV
// --------------------------------------------------------
const ShaderResourceViewIndex* ShaderResource::GetShaderResourceViewInfo(unsigned int index)
{
    // Valid index?
    if (index >= shaderResourceViews.size()) return 0;

    // Grab the bind index
    return shaderResourceViews[index];
}


// --------------------------------------------------------
// Gets info about a sampler in the shader (or null)
// 
// name - the name of the sampler
// --------------------------------------------------------
const ShaderSampler* ShaderResource::GetSamplerInfo(std::string name)
{
    // Look for the key
    std::unordered_map<std::string, ShaderSampler*>::iterator result =
        samplerTable.find(name);

    // Did we find the key?
    if (result == samplerTable.end())
        return 0;

    // Success
    return result->second;
}

// --------------------------------------------------------
// Gets info about a sampler in the shader (or null)
// 
// index - the index of the sampler
// --------------------------------------------------------
const ShaderSampler* ShaderResource::GetSamplerInfo(unsigned int index)
{
    // Valid index?
    if (index >= samplerStates.size()) return 0;

    // Grab the bind index
    return samplerStates[index];
}

// --------------------------------------------------------
// Gets the number of constant buffers in this shader
// --------------------------------------------------------
unsigned int ShaderResource::GetBufferCount() const { return constantBufferCount; }


// --------------------------------------------------------
// Gets the size of a particular constant buffer, or -1
// --------------------------------------------------------
unsigned int ShaderResource::GetBufferSize(unsigned int index)
{
    // Valid index?
    if (index >= constantBufferCount)
        return -1;

    // Grab the size
    return constantBuffers[index].Size;
}

// --------------------------------------------------------
// Gets info about a particular constant buffer 
// by name, if it exists
// --------------------------------------------------------
const ShaderConstantBuffer* ShaderResource::GetBufferInfo(std::string name)
{
    return FindConstantBuffer(name);
}

// --------------------------------------------------------
// Gets info about a particular constant buffer 
//
// index - the index of the constant buffer
// --------------------------------------------------------
const ShaderConstantBuffer* ShaderResource::GetBufferInfo(unsigned int index)
{
    // Check for valid index
    if (index >= constantBufferCount) return 0;

    // Return the specific buffer
    return &constantBuffers[index];
}
