#pragma once
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#include <unordered_map>
#include <vector>
#include <string>
#include <array>
#include "TypeDefinition.h"

enum class SHADER_TYPE
{
    VERTEX_SHADER,
    PIXEL_SHADER,
    DOMAIN_SHADER,
    HULL_SHADER,
    GEOMETRY_SHADER,
    COMPUTE_SHADER,
};

// --------------------------------------------------------
// Used by simple shaders to store information about
// specific variables in constant buffers
// --------------------------------------------------------
struct ConstantBufferVariable
{
    uint32 ByteOffset{};
    uint32 Size{};
    uint32 ConstantBufferIndex{};
    uint32 ElementCount{};
};

// --------------------------------------------------------
// Contains information about a specific
// constant buffer in a shader, as well as
// the local data buffer for it
// --------------------------------------------------------
struct ShaderConstantBuffer
{
    std::string Name{};
    uint32 Size{};
    uint32 BindIndex{};
    ID3D11Buffer* ConstantBuffer{ nullptr };
    byte* LocalDataBuffer{ nullptr };
    std::vector<ConstantBufferVariable> Variables;
};

// --------------------------------------------------------
// Contains info about a single SRV in a shader
// --------------------------------------------------------
struct ShaderResourceViewIndex
{
    uint32 Index{};		// The raw index of the SRV
    uint32 BindIndex{}; // The register of the SRV
};

// --------------------------------------------------------
// Contains info about a single Sampler in a shader
// --------------------------------------------------------
struct ShaderSampler
{
    uint32 Index{};		// The raw index of the Sampler
    uint32 BindIndex{}; // The register of the Sampler
};

inline DXGI_FORMAT FormatForSingleComponent(uint32 componentType)
{
    DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN;
    switch (componentType)
    {
    case D3D_REGISTER_COMPONENT_UINT32:
        Format = DXGI_FORMAT_R32_UINT;
        break;
    case D3D_REGISTER_COMPONENT_SINT32:
        Format = DXGI_FORMAT_R32_SINT;
        break;
    case D3D_REGISTER_COMPONENT_FLOAT32:
        Format = DXGI_FORMAT_R32_FLOAT;
        break;
    }
    return Format;
}

inline DXGI_FORMAT FormatForTwoComponents(uint32 componentType)
{
    DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN;
    switch (componentType)
    {
    case D3D_REGISTER_COMPONENT_UINT32:
        Format = DXGI_FORMAT_R32G32_UINT;
        break;
    case D3D_REGISTER_COMPONENT_SINT32:
        Format = DXGI_FORMAT_R32G32_SINT;
        break;
    case D3D_REGISTER_COMPONENT_FLOAT32:
        Format = DXGI_FORMAT_R32G32_FLOAT;
        break;
    }
    return Format;
}

inline DXGI_FORMAT FormatForThreeComponents(uint32 componentType)
{
    DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN;
    switch (componentType)
    {
    case D3D_REGISTER_COMPONENT_UINT32:
        Format = DXGI_FORMAT_R32G32B32_UINT;
        break;
    case D3D_REGISTER_COMPONENT_SINT32:
        Format = DXGI_FORMAT_R32G32B32_SINT;
        break;
    case D3D_REGISTER_COMPONENT_FLOAT32:
        Format = DXGI_FORMAT_R32G32B32_FLOAT;
        break;
    }
    return Format;
}

inline DXGI_FORMAT FormatForFourComponents(uint32 componentType)
{
    DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN;
    switch (componentType)
    {
    case D3D_REGISTER_COMPONENT_UINT32:
        Format = DXGI_FORMAT_R32G32B32A32_UINT;
        break;
    case D3D_REGISTER_COMPONENT_SINT32:
        Format = DXGI_FORMAT_R32G32B32A32_SINT;
        break;
    case D3D_REGISTER_COMPONENT_FLOAT32:
        Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        break;
    }
    return Format;
}

inline DXGI_FORMAT DetermineFormatFromComponentType(byte mask, uint32 componentType)
{
    DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN;
    if (mask == 1)
    {
        Format = FormatForSingleComponent(componentType);
    }
    else if (mask == 3)
    {
        Format = FormatForTwoComponents(componentType);
    }
    else if (mask == 7)
    {
        Format = FormatForThreeComponents(componentType);
    }
    else if (mask == 15)
    {
        Format = FormatForFourComponents(componentType);
    }

    return Format;
}
