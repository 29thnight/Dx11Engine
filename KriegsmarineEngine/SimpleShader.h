#pragma once
#include "ShaderResource.h"

// --------------------------------------------------------
// Derived class for VERTEX shaders ///////////////////////
// --------------------------------------------------------
class SimpleVertexShader : public ShaderResource
{
public:
	SimpleVertexShader(const std::shared_ptr<DirectX11::DeviceResources>& resources);
	SimpleVertexShader(const std::shared_ptr<DirectX11::DeviceResources>& resources, ID3D11InputLayout* inputLayout, bool perInstanceCompatible);
	~SimpleVertexShader();
	ID3D11VertexShader* GetDirectXShader() { return shader; }
	ID3D11InputLayout* GetInputLayout() { return inputLayout; }
	bool GetPerInstanceCompatible() const { return perInstanceCompatible; }

	bool SetShaderResourceView(std::string name, ID3D11ShaderResourceView* srv);
	bool SetSamplerState(std::string name, ID3D11SamplerState* samplerState);

protected:
	bool CreateShader(ID3DBlob* shaderBlob);
	void SetShaderAndCBs();
	void CleanUp();

protected:
	bool perInstanceCompatible{};
	ID3D11InputLayout* inputLayout{};
	ID3D11VertexShader* shader{};
};


// --------------------------------------------------------
// Derived class for PIXEL shaders ////////////////////////
// --------------------------------------------------------
class SimplePixelShader : public ShaderResource
{
public:
	SimplePixelShader(const std::shared_ptr<DirectX11::DeviceResources>& resources);
	~SimplePixelShader();
	ID3D11PixelShader* GetDirectXShader() { return shader; }

	bool SetShaderResourceView(std::string name, ID3D11ShaderResourceView* srv);
	bool SetSamplerState(std::string name, ID3D11SamplerState* samplerState);

protected:
	bool CreateShader(ID3DBlob* shaderBlob);
	void SetShaderAndCBs();
	void CleanUp();

protected:
	ID3D11PixelShader* shader{};
};

// --------------------------------------------------------
// Derived class for DOMAIN shaders ///////////////////////
// --------------------------------------------------------
class SimpleDomainShader : public ShaderResource
{
public:
	SimpleDomainShader(const std::shared_ptr<DirectX11::DeviceResources>& resources);
	~SimpleDomainShader();
	ID3D11DomainShader* GetDirectXShader() { return shader; }

	bool SetShaderResourceView(std::string name, ID3D11ShaderResourceView* srv);
	bool SetSamplerState(std::string name, ID3D11SamplerState* samplerState);

protected:
	bool CreateShader(ID3DBlob* shaderBlob);
	void SetShaderAndCBs();
	void CleanUp();

protected:
	ID3D11DomainShader* shader{};

};

// --------------------------------------------------------
// Derived class for HULL shaders /////////////////////////
// --------------------------------------------------------
class SimpleHullShader : public ShaderResource
{
public:
	SimpleHullShader(const std::shared_ptr<DirectX11::DeviceResources>& resources);
	~SimpleHullShader();
	ID3D11HullShader* GetDirectXShader() { return shader; }

	bool SetShaderResourceView(std::string name, ID3D11ShaderResourceView* srv);
	bool SetSamplerState(std::string name, ID3D11SamplerState* samplerState);

protected:
	bool CreateShader(ID3DBlob* shaderBlob);
	void SetShaderAndCBs();
	void CleanUp();

protected:
	ID3D11HullShader* shader{};

};

// --------------------------------------------------------
// Derived class for GEOMETRY shaders /////////////////////
// --------------------------------------------------------
class SimpleGeometryShader : public ShaderResource
{
public:
	SimpleGeometryShader(const std::shared_ptr<DirectX11::DeviceResources>& resources, bool useStreamOut = 0, bool allowStreamOutRasterization = 0);
	~SimpleGeometryShader();
	ID3D11GeometryShader* GetDirectXShader() { return shader; }

	bool SetShaderResourceView(std::string name, ID3D11ShaderResourceView* srv);
	bool SetSamplerState(std::string name, ID3D11SamplerState* samplerState);

	bool CreateCompatibleStreamOutBuffer(ID3D11Buffer** buffer, int vertexCount);

	static void UnbindStreamOutStage(ID3D11DeviceContext* deviceContext);

protected:
	bool CreateShader(ID3DBlob* shaderBlob);
	bool CreateShaderWithStreamOut(ID3DBlob* shaderBlob);
	void SetShaderAndCBs();
	void CleanUp();

	// Helpers
	unsigned int CalcComponentCount(unsigned int mask);

protected:
	// Shader itself
	ID3D11GeometryShader* shader{};

	// Stream out related
	bool useStreamOut{};
	bool allowStreamOutRasterization{};
	unsigned int streamOutVertexSize{};

};

// --------------------------------------------------------
// Derived class for COMPUTE shaders //////////////////////
// --------------------------------------------------------
class SimpleComputeShader : public ShaderResource
{
public:
	SimpleComputeShader(const std::shared_ptr<DirectX11::DeviceResources>& resources);
	~SimpleComputeShader();
	ID3D11ComputeShader* GetDirectXShader() { return shader; }

	void DispatchByGroups(unsigned int groupsX, unsigned int groupsY, unsigned int groupsZ);
	void DispatchByThreads(unsigned int threadsX, unsigned int threadsY, unsigned int threadsZ);

	bool SetShaderResourceView(std::string name, ID3D11ShaderResourceView* srv);
	bool SetSamplerState(std::string name, ID3D11SamplerState* samplerState);
	bool SetUnorderedAccessView(std::string name, ID3D11UnorderedAccessView* uav, unsigned int appendConsumeOffset = -1);

	int GetUnorderedAccessViewIndex(std::string name);

protected:
	bool CreateShader(ID3DBlob* shaderBlob);
	void SetShaderAndCBs();
	void CleanUp();

protected:
	ID3D11ComputeShader* shader{};
	std::unordered_map<std::string, unsigned int> uavTable;

	unsigned int threadsX{};
	unsigned int threadsY{};
	unsigned int threadsZ{};
	unsigned int threadsTotal{};

};
