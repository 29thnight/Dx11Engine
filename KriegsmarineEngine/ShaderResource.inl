#include "ShaderResource.h"
// --------------------------------------------------------
// Sets a variable by name with arbitrary data of the specified size
//
// name - The name of the shader variable
// data - The data to set in the buffer
// size - The size of the data (this must match the variable's size)
//
// Returns true if data is copied, false if variable doesn't 
// exist or sizes don't match
// --------------------------------------------------------
template <typename T>
inline bool ShaderResource::SetData(std::string name, const T& data)
{
    ConstantBufferVariable* var = FindVariable(name, sizeof(T));
    if (var == 0)
    {
        return false;
    }

    byte* buffer = constantBuffers[var->ConstantBufferIndex].LocalDataBuffer + var->ByteOffset;
    size_t size = sizeof(T);

    memcpy(buffer, &data, size);
    return true;
}

template<typename T, size_t N>
inline bool ShaderResource::SetData(std::string name, const T data[N])
{
    ConstantBufferVariable* var = FindVariable(name, sizeof(T) * N);
    if (var == 0 || var->ElementCount != N)
    {
        return false;
    }

    byte* buffer = constantBuffers[var->ConstantBufferIndex].LocalDataBuffer + var->ByteOffset;
    size_t size = sizeof(T) * N;

    memcpy(buffer, &data, size);
    return true;
}
