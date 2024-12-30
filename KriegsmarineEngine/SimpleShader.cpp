#include "SimpleShader.h"
#include "Core.Memory.h"

///////////////////////////////////////////////////////////////////////////////
// ------ SIMPLE VERTEX SHADER ------------------------------------------------
///////////////////////////////////////////////////////////////////////////////

// --------------------------------------------------------
// Constructor just calls the base
// --------------------------------------------------------
SimpleVertexShader::SimpleVertexShader(const std::shared_ptr<DirectX11::DeviceResources>& resources)
	: ShaderResource(resources)
{
	// Ensure we set to zero to successfully trigger
	// the Input Layout creation during LoadShader()
	this->inputLayout = nullptr;
	this->shader = nullptr;
	this->perInstanceCompatible = false;
}

// --------------------------------------------------------
// Constructor overload which takes a custom input layout
//
// Passing in a valid input layout will stop LoadShader()
// from creating an input layout from shader reflection
// --------------------------------------------------------
SimpleVertexShader::SimpleVertexShader(const std::shared_ptr<DirectX11::DeviceResources>& resources, ID3D11InputLayout* inputLayout, bool perInstanceCompatible)
	: ShaderResource(resources)
{
	// Save the custom input layout
	this->inputLayout = inputLayout;
	this->shader = nullptr;

	// Unable to determine from an input layout, require user to tell us
	this->perInstanceCompatible = perInstanceCompatible;
}

// --------------------------------------------------------
// Destructor - Clean up actual shader (base will be called automatically)
// --------------------------------------------------------
SimpleVertexShader::~SimpleVertexShader()
{
	CleanUp();
}

// --------------------------------------------------------
// Handles cleaning up shader and base class clean up
// --------------------------------------------------------
void SimpleVertexShader::CleanUp()
{
	ShaderResource::CleanUp();
	Memory::SafeDelete(shader);
	Memory::SafeDelete(inputLayout);
}

// --------------------------------------------------------
// Creates the DirectX vertex shader
//
// shaderBlob - The shader's compiled code
//
// Returns true if shader is created correctly, false otherwise
// --------------------------------------------------------
bool SimpleVertexShader::CreateShader(ID3DBlob* shaderBlob)
{
	// Clean up first, in the event this method is
	// called more than once on the same object
	this->CleanUp();

	// Create the shader from the blob
	HRESULT result = device->CreateVertexShader(
		shaderBlob->GetBufferPointer(),
		shaderBlob->GetBufferSize(),
		0,
		&shader);

	// Did the creation work?
	if (result != S_OK)
		return false;

	// Do we already have an input layout?
	// (This would come from one of the constructor overloads)
	if (inputLayout)
		return true;

	// Vertex shader was created successfully, so we now use the
	// shader code to re-reflect and create an input layout that 
	// matches what the vertex shader expects.  Code adapted from:
	// https://takinginitiative.wordpress.com/2011/12/11/directx-1011-basic-shader-reflection-automatic-input-layout-creation/

	// Reflect shader info
	ID3D11ShaderReflection* refl{};
	D3DReflect(
		shaderBlob->GetBufferPointer(),
		shaderBlob->GetBufferSize(),
		IID_ID3D11ShaderReflection,
		(void**)&refl);

	// Get shader info
	D3D11_SHADER_DESC shaderDesc;
	refl->GetDesc(&shaderDesc);

	// Read input layout description from shader info
	std::vector<D3D11_INPUT_ELEMENT_DESC> inputLayoutDesc;
	for (unsigned int i = 0; i < shaderDesc.InputParameters; i++)
	{
		D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
		refl->GetInputParameterDesc(i, &paramDesc);

		// Check the semantic name for "_PER_INSTANCE"
		std::string perInstanceStr = "_PER_INSTANCE";
		std::string sem = paramDesc.SemanticName;
		int lenDiff = static_cast<int>(sem.size() - perInstanceStr.size());
		bool isPerInstance =
			lenDiff >= 0 &&
			sem.compare(lenDiff, perInstanceStr.size(), perInstanceStr) == 0;

		// Fill out input element desc
		D3D11_INPUT_ELEMENT_DESC elementDesc{};
		elementDesc.SemanticName = paramDesc.SemanticName;
		elementDesc.SemanticIndex = paramDesc.SemanticIndex;
		elementDesc.InputSlot = 0;
		elementDesc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		elementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		elementDesc.InstanceDataStepRate = 0;

		// Replace anything affected by "per instance" data
		if (isPerInstance)
		{
			elementDesc.InputSlot = 1; // Assume per instance data comes from another input slot!
			elementDesc.InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
			elementDesc.InstanceDataStepRate = 1;

			perInstanceCompatible = true;
		}

		// Determine DXGI format
		elementDesc.Format = DetermineFormatFromComponentType(paramDesc.Mask, paramDesc.ComponentType);

		// Save element desc
		inputLayoutDesc.push_back(elementDesc);
	}

	// Try to create Input Layout
	HRESULT hr = device->CreateInputLayout(
		&inputLayoutDesc[0],
		static_cast<uint32>(inputLayoutDesc.size()),
		shaderBlob->GetBufferPointer(),
		shaderBlob->GetBufferSize(),
		&inputLayout);

	// All done, clean up
	refl->Release();
	return true;
}

// --------------------------------------------------------
// Sets the vertex shader, input layout and constant buffers
// for future DirectX drawing
// --------------------------------------------------------
void SimpleVertexShader::SetShaderAndCBs()
{
	// Is shader valid?
	if (!shaderValid) return;

	// Set the shader and input layout
	deviceContext->IASetInputLayout(inputLayout);
	deviceContext->VSSetShader(shader, 0, 0);

	// Set the constant buffers
	for (unsigned int i = 0; i < constantBufferCount; i++)
	{
		deviceContext->VSSetConstantBuffers(
			constantBuffers[i].BindIndex,
			1,
			&constantBuffers[i].ConstantBuffer);
	}
}

// --------------------------------------------------------
// Sets a shader resource view in the vertex shader stage
//
// name - The name of the texture resource in the shader
// srv - The shader resource view of the texture in GPU memory
//
// Returns true if a texture of the given name was found, false otherwise
// --------------------------------------------------------
bool SimpleVertexShader::SetShaderResourceView(std::string name, ID3D11ShaderResourceView* srv)
{
	// Look for the variable and verify
	const ShaderResourceViewIndex* srvInfo = GetShaderResourceViewInfo(name);
	if (srvInfo == nullptr)
		return false;

	// Set the shader resource view
	deviceContext->VSSetShaderResources(srvInfo->BindIndex, 1, &srv);

	// Success
	return true;
}

// --------------------------------------------------------
// Sets a sampler state in the vertex shader stage
//
// name - The name of the sampler state in the shader
// samplerState - The sampler state in GPU memory
//
// Returns true if a sampler of the given name was found, false otherwise
// --------------------------------------------------------
bool SimpleVertexShader::SetSamplerState(std::string name, ID3D11SamplerState* samplerState)
{
	// Look for the variable and verify
	const ShaderSampler* sampInfo = GetSamplerInfo(name);
	if (sampInfo == nullptr)
		return false;

	// Set the shader resource view
	deviceContext->VSSetSamplers(sampInfo->BindIndex, 1, &samplerState);

	// Success
	return true;
}


///////////////////////////////////////////////////////////////////////////////
// ------ SIMPLE PIXEL SHADER -------------------------------------------------
///////////////////////////////////////////////////////////////////////////////

// --------------------------------------------------------
// Constructor just calls the base
// --------------------------------------------------------
SimplePixelShader::SimplePixelShader(const std::shared_ptr<DirectX11::DeviceResources>& resources)
	: ShaderResource(resources)
{
	this->shader = nullptr;
}

// --------------------------------------------------------
// Destructor - Clean up actual shader (base will be called automatically)
// --------------------------------------------------------
SimplePixelShader::~SimplePixelShader()
{
	CleanUp();
}

// --------------------------------------------------------
// Handles cleaning up shader and base class clean up
// --------------------------------------------------------
void SimplePixelShader::CleanUp()
{
	ShaderResource::CleanUp();
	Memory::SafeDelete(shader);
}

// --------------------------------------------------------
// Creates the DirectX pixel shader
//
// shaderBlob - The shader's compiled code
//
// Returns true if shader is created correctly, false otherwise
// --------------------------------------------------------
bool SimplePixelShader::CreateShader(ID3DBlob* shaderBlob)
{
	// Clean up first, in the event this method is
	// called more than once on the same object
	this->CleanUp();

	// Create the shader from the blob
	HRESULT result = device->CreatePixelShader(
		shaderBlob->GetBufferPointer(),
		shaderBlob->GetBufferSize(),
		0,
		&shader);

	// Check the result
	return (result == S_OK);
}

// --------------------------------------------------------
// Sets the pixel shader and constant buffers for
// future DirectX drawing
// --------------------------------------------------------
void SimplePixelShader::SetShaderAndCBs()
{
	// Is shader valid?
	if (!shaderValid) return;

	// Set the shader
	deviceContext->PSSetShader(shader, 0, 0);

	// Set the constant buffers
	for (unsigned int i = 0; i < constantBufferCount; i++)
	{
		deviceContext->PSSetConstantBuffers(
			constantBuffers[i].BindIndex,
			1,
			&constantBuffers[i].ConstantBuffer);
	}
}

// --------------------------------------------------------
// Sets a shader resource view in the pixel shader stage
//
// name - The name of the texture resource in the shader
// srv - The shader resource view of the texture in GPU memory
//
// Returns true if a texture of the given name was found, false otherwise
// --------------------------------------------------------
bool SimplePixelShader::SetShaderResourceView(std::string name, ID3D11ShaderResourceView* srv)
{
	// Look for the variable and verify
	const ShaderResourceViewIndex* srvInfo = GetShaderResourceViewInfo(name);
	if (srvInfo == nullptr)
		return false;

	// Set the shader resource view
	deviceContext->PSSetShaderResources(srvInfo->BindIndex, 1, &srv);

	// Success
	return true;
}

// --------------------------------------------------------
// Sets a sampler state in the pixel shader stage
//
// name - The name of the sampler state in the shader
// samplerState - The sampler state in GPU memory
//
// Returns true if a sampler of the given name was found, false otherwise
// --------------------------------------------------------
bool SimplePixelShader::SetSamplerState(std::string name, ID3D11SamplerState* samplerState)
{
	// Look for the variable and verify
	const ShaderSampler* sampInfo = GetSamplerInfo(name);
	if (sampInfo == nullptr)
		return false;

	// Set the shader resource view
	deviceContext->PSSetSamplers(sampInfo->BindIndex, 1, &samplerState);

	// Success
	return true;
}




///////////////////////////////////////////////////////////////////////////////
// ------ SIMPLE DOMAIN SHADER ------------------------------------------------
///////////////////////////////////////////////////////////////////////////////

// --------------------------------------------------------
// Constructor just calls the base
// --------------------------------------------------------
SimpleDomainShader::SimpleDomainShader(const std::shared_ptr<DirectX11::DeviceResources>& resources)
	: ShaderResource(resources)
{
	this->shader = nullptr;
}

// --------------------------------------------------------
// Destructor - Clean up actual shader (base will be called automatically)
// --------------------------------------------------------
SimpleDomainShader::~SimpleDomainShader()
{
	CleanUp();
}

// --------------------------------------------------------
// Handles cleaning up shader and base class clean up
// --------------------------------------------------------
void SimpleDomainShader::CleanUp()
{
	ShaderResource::CleanUp();
	Memory::SafeDelete(shader);
}

// --------------------------------------------------------
// Creates the DirectX domain shader
//
// shaderBlob - The shader's compiled code
//
// Returns true if shader is created correctly, false otherwise
// --------------------------------------------------------
bool SimpleDomainShader::CreateShader(ID3DBlob* shaderBlob)
{
	// Clean up first, in the event this method is
	// called more than once on the same object
	this->CleanUp();

	// Create the shader from the blob
	HRESULT result = device->CreateDomainShader(
		shaderBlob->GetBufferPointer(),
		shaderBlob->GetBufferSize(),
		0,
		&shader);

	// Check the result
	return (result == S_OK);
}

// --------------------------------------------------------
// Sets the domain shader and constant buffers for
// future DirectX drawing
// --------------------------------------------------------
void SimpleDomainShader::SetShaderAndCBs()
{
	// Is shader valid?
	if (!shaderValid) return;

	// Set the shader
	deviceContext->DSSetShader(shader, 0, 0);

	// Set the constant buffers
	for (unsigned int i = 0; i < constantBufferCount; i++)
	{
		deviceContext->DSSetConstantBuffers(
			constantBuffers[i].BindIndex,
			1,
			&constantBuffers[i].ConstantBuffer);
	}
}

// --------------------------------------------------------
// Sets a shader resource view in the domain shader stage
//
// name - The name of the texture resource in the shader
// srv - The shader resource view of the texture in GPU memory
//
// Returns true if a texture of the given name was found, false otherwise
// --------------------------------------------------------
bool SimpleDomainShader::SetShaderResourceView(std::string name, ID3D11ShaderResourceView* srv)
{
	// Look for the variable and verify
	const ShaderResourceViewIndex* srvInfo = GetShaderResourceViewInfo(name);
	if (srvInfo == nullptr)
		return false;

	// Set the shader resource view
	deviceContext->DSSetShaderResources(srvInfo->BindIndex, 1, &srv);

	// Success
	return true;
}

// --------------------------------------------------------
// Sets a sampler state in the domain shader stage
//
// name - The name of the sampler state in the shader
// samplerState - The sampler state in GPU memory
//
// Returns true if a sampler of the given name was found, false otherwise
// --------------------------------------------------------
bool SimpleDomainShader::SetSamplerState(std::string name, ID3D11SamplerState* samplerState)
{
	// Look for the variable and verify
	const ShaderSampler* sampInfo = GetSamplerInfo(name);
	if (sampInfo == nullptr)
		return false;

	// Set the shader resource view
	deviceContext->DSSetSamplers(sampInfo->BindIndex, 1, &samplerState);

	// Success
	return true;
}



///////////////////////////////////////////////////////////////////////////////
// ------ SIMPLE HULL SHADER --------------------------------------------------
///////////////////////////////////////////////////////////////////////////////

// --------------------------------------------------------
// Constructor just calls the base
// --------------------------------------------------------
SimpleHullShader::SimpleHullShader(const std::shared_ptr<DirectX11::DeviceResources>& resources)
	: ShaderResource(resources)
{
	this->shader = 0;
}

// --------------------------------------------------------
// Destructor - Clean up actual shader (base will be called automatically)
// --------------------------------------------------------
SimpleHullShader::~SimpleHullShader()
{
	CleanUp();
}

// --------------------------------------------------------
// Handles cleaning up shader and base class clean up
// --------------------------------------------------------
void SimpleHullShader::CleanUp()
{
	ShaderResource::CleanUp();
	Memory::SafeDelete(shader);
}

// --------------------------------------------------------
// Creates the DirectX hull shader
//
// shaderBlob - The shader's compiled code
//
// Returns true if shader is created correctly, false otherwise
// --------------------------------------------------------
bool SimpleHullShader::CreateShader(ID3DBlob* shaderBlob)
{
	// Clean up first, in the event this method is
	// called more than once on the same object
	this->CleanUp();

	// Create the shader from the blob
	HRESULT result = device->CreateHullShader(
		shaderBlob->GetBufferPointer(),
		shaderBlob->GetBufferSize(),
		0,
		&shader);

	// Check the result
	return (result == S_OK);
}

// --------------------------------------------------------
// Sets the hull shader and constant buffers for
// future DirectX drawing
// --------------------------------------------------------
void SimpleHullShader::SetShaderAndCBs()
{
	// Is shader valid?
	if (!shaderValid) return;

	// Set the shader
	deviceContext->HSSetShader(shader, 0, 0);

	// Set the constant buffers?
	for (unsigned int i = 0; i < constantBufferCount; i++)
	{
		deviceContext->HSSetConstantBuffers(
			constantBuffers[i].BindIndex,
			1,
			&constantBuffers[i].ConstantBuffer);
	}
}

// --------------------------------------------------------
// Sets a shader resource view in the hull shader stage
//
// name - The name of the texture resource in the shader
// srv - The shader resource view of the texture in GPU memory
//
// Returns true if a texture of the given name was found, false otherwise
// --------------------------------------------------------
bool SimpleHullShader::SetShaderResourceView(std::string name, ID3D11ShaderResourceView* srv)
{
	// Look for the variable and verify
	const ShaderResourceViewIndex* srvInfo = GetShaderResourceViewInfo(name);
	if (srvInfo == nullptr)
		return false;

	// Set the shader resource view
	deviceContext->HSSetShaderResources(srvInfo->BindIndex, 1, &srv);

	// Success
	return true;
}

// --------------------------------------------------------
// Sets a sampler state in the hull shader stage
//
// name - The name of the sampler state in the shader
// samplerState - The sampler state in GPU memory
//
// Returns true if a sampler of the given name was found, false otherwise
// --------------------------------------------------------
bool SimpleHullShader::SetSamplerState(std::string name, ID3D11SamplerState* samplerState)
{
	// Look for the variable and verify
	const ShaderSampler* sampInfo = GetSamplerInfo(name);
	if (sampInfo == nullptr)
		return false;

	// Set the shader resource view
	deviceContext->HSSetSamplers(sampInfo->BindIndex, 1, &samplerState);

	// Success
	return true;
}




///////////////////////////////////////////////////////////////////////////////
// ------ SIMPLE GEOMETRY SHADER ----------------------------------------------
///////////////////////////////////////////////////////////////////////////////

// --------------------------------------------------------
// Constructor calls the base and sets up potential stream-out options
// --------------------------------------------------------
SimpleGeometryShader::SimpleGeometryShader(const std::shared_ptr<DirectX11::DeviceResources>& resources, bool useStreamOut, bool allowStreamOutRasterization)
	: ShaderResource(resources)
{
	this->shader = 0;
	this->useStreamOut = useStreamOut;
	this->allowStreamOutRasterization = allowStreamOutRasterization;
}

// --------------------------------------------------------
// Destructor - Clean up actual shader (base will be called automatically)
// --------------------------------------------------------
SimpleGeometryShader::~SimpleGeometryShader()
{
	CleanUp();
}

// --------------------------------------------------------
// Handles cleaning up shader and base class clean up
// --------------------------------------------------------
void SimpleGeometryShader::CleanUp()
{
	ShaderResource::CleanUp();
	Memory::SafeDelete(shader);
}

// --------------------------------------------------------
// Creates the DirectX Geometry shader
//
// shaderBlob - The shader's compiled code
//
// Returns true if shader is created correctly, false otherwise
// --------------------------------------------------------
bool SimpleGeometryShader::CreateShader(ID3DBlob* shaderBlob)
{
	// Clean up first, in the event this method is
	// called more than once on the same object
	this->CleanUp();

	// Using stream out?
	if (useStreamOut)
		return this->CreateShaderWithStreamOut(shaderBlob);

	// Create the shader from the blob
	HRESULT result = device->CreateGeometryShader(
		shaderBlob->GetBufferPointer(),
		shaderBlob->GetBufferSize(),
		0,
		&shader);

	// Check the result
	return (result == S_OK);
}

// --------------------------------------------------------
// Creates the DirectX Geometry shader and sets it up for
// stream output, if possible.
//
// shaderBlob - The shader's compiled code
//
// Returns true if shader is created correctly, false otherwise
// --------------------------------------------------------
bool SimpleGeometryShader::CreateShaderWithStreamOut(ID3DBlob* shaderBlob)
{
	// Clean up first, in the event this method is
	// called more than once on the same object
	this->CleanUp();

	// Reflect shader info
	ID3D11ShaderReflection* refl{};
	D3DReflect(
		shaderBlob->GetBufferPointer(),
		shaderBlob->GetBufferSize(),
		IID_ID3D11ShaderReflection,
		(void**)&refl);

	// Get shader info
	D3D11_SHADER_DESC shaderDesc;
	refl->GetDesc(&shaderDesc);

	// Set up the output signature
	streamOutVertexSize = 0;
	std::vector<D3D11_SO_DECLARATION_ENTRY> soDecl;
	for (unsigned int i = 0; i < shaderDesc.OutputParameters; i++)
	{
		// Get the info about this entry
		D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
		refl->GetOutputParameterDesc(i, &paramDesc);

		// Create the SO Declaration
		D3D11_SO_DECLARATION_ENTRY entry{};
		entry.SemanticIndex = paramDesc.SemanticIndex;
		entry.SemanticName = paramDesc.SemanticName;
		entry.Stream = paramDesc.Stream;
		entry.StartComponent = 0; // Assume starting at 0
		entry.OutputSlot = 0; // Assume the first output slot

		// Check the mask to determine how many components are used
		entry.ComponentCount = CalcComponentCount(paramDesc.Mask);

		// Increment the size
		streamOutVertexSize += entry.ComponentCount * sizeof(float);

		// Add to the declaration
		soDecl.push_back(entry);
	}

	// Rasterization allowed?
	unsigned int rast = allowStreamOutRasterization ? 0 : D3D11_SO_NO_RASTERIZED_STREAM;

	// Create the shader
	HRESULT result = device->CreateGeometryShaderWithStreamOutput(
		shaderBlob->GetBufferPointer(), // Shader blob pointer
		shaderBlob->GetBufferSize(),    // Shader blob size
		&soDecl[0],                     // Stream out declaration
		static_cast<uint32>(soDecl.size()),                  // Number of declaration entries
		NULL,                           // Buffer strides (not used - assume tightly packed?)
		0,                              // No buffer strides
		rast,                           // Index of the stream to rasterize (if any)
		NULL,                           // Not using class linkage
		&shader);

	return (result == S_OK);
}

// --------------------------------------------------------
// Creates a vertex buffer that is compatible with the stream output
// delcaration that was used to create the shader.  This buffer will
// not be cleaned up (Released) by the simple shader - you must clean
// it up yourself when you're done with it.  Immediately returns
// false if the shader was not created with stream output, the shader
// isn't valid or the determined stream out vertex size is zero.
//
// buffer - Pointer to an ID3D11Buffer pointer to hold the buffer ref
// vertexCount - Amount of vertices the buffer should hold
//
// Returns true if buffer is created successfully AND stream output
// was used to create the shader.  False otherwise.
// --------------------------------------------------------
bool SimpleGeometryShader::CreateCompatibleStreamOutBuffer(ID3D11Buffer** buffer, int vertexCount)
{
	// Was stream output actually used?
	if (!this->useStreamOut || !shaderValid || streamOutVertexSize == 0)
		return false;

	// Set up the buffer description
	D3D11_BUFFER_DESC desc{};
	desc.BindFlags = D3D11_BIND_STREAM_OUTPUT | D3D11_BIND_VERTEX_BUFFER;
	desc.ByteWidth = streamOutVertexSize * vertexCount;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;

	// Attempt to create the buffer and return the result
	HRESULT result = device->CreateBuffer(&desc, 0, buffer);
	return (result == S_OK);
}

// --------------------------------------------------------
// Helper method to unbind all stream out buffers from the SO stage
// --------------------------------------------------------
void SimpleGeometryShader::UnbindStreamOutStage(ID3D11DeviceContext* deviceContext)
{
	unsigned int offset = 0;
	ID3D11Buffer* unset[1] = { 0 };
	deviceContext->SOSetTargets(1, unset, &offset);
}

// --------------------------------------------------------
// Sets the geometry shader and constant buffers for
// future DirectX drawing
// --------------------------------------------------------
void SimpleGeometryShader::SetShaderAndCBs()
{
	// Is shader valid?
	if (!shaderValid) return;

	// Set the shader
	deviceContext->GSSetShader(shader, 0, 0);

	// Set the constant buffers?
	for (unsigned int i = 0; i < constantBufferCount; i++)
	{
		deviceContext->GSSetConstantBuffers(
			constantBuffers[i].BindIndex,
			1,
			&constantBuffers[i].ConstantBuffer);
	}
}

// --------------------------------------------------------
// Sets a shader resource view in the Geometry shader stage
//
// name - The name of the texture resource in the shader
// srv - The shader resource view of the texture in GPU memory
//
// Returns true if a texture of the given name was found, false otherwise
// --------------------------------------------------------
bool SimpleGeometryShader::SetShaderResourceView(std::string name, ID3D11ShaderResourceView* srv)
{
	// Look for the variable and verify
	const ShaderResourceViewIndex* srvInfo = GetShaderResourceViewInfo(name);
	if (srvInfo == nullptr)
		return false;

	// Set the shader resource view
	deviceContext->GSSetShaderResources(srvInfo->BindIndex, 1, &srv);

	// Success
	return true;
}

// --------------------------------------------------------
// Sets a sampler state in the Geometry shader stage
//
// name - The name of the sampler state in the shader
// samplerState - The sampler state in GPU memory
//
// Returns true if a sampler of the given name was found, false otherwise
// --------------------------------------------------------
bool SimpleGeometryShader::SetSamplerState(std::string name, ID3D11SamplerState* samplerState)
{
	// Look for the variable and verify
	const ShaderSampler* sampInfo = GetSamplerInfo(name);
	if (sampInfo == nullptr)
		return false;

	// Set the shader resource view
	deviceContext->GSSetSamplers(sampInfo->BindIndex, 1, &samplerState);

	// Success
	return true;
}

// --------------------------------------------------------
// Calculates the number of components specified by a parameter description mask
//
// mask - The mask to check (only values 0 - 15 are considered)
//
// Returns an integer between 0 - 4 inclusive
// --------------------------------------------------------
unsigned int SimpleGeometryShader::CalcComponentCount(unsigned int mask)
{
	unsigned int result = 0;
	result += (unsigned int)((mask & 1) == 1);
	result += (unsigned int)((mask & 2) == 2);
	result += (unsigned int)((mask & 4) == 4);
	result += (unsigned int)((mask & 8) == 8);
	return result;
}


///////////////////////////////////////////////////////////////////////////////
// ------ SIMPLE COMPUTE SHADER -----------------------------------------------
///////////////////////////////////////////////////////////////////////////////

// --------------------------------------------------------
// Constructor just calls the base
// --------------------------------------------------------
SimpleComputeShader::SimpleComputeShader(const std::shared_ptr<DirectX11::DeviceResources>& resources)
	: ShaderResource(resources)
{
	this->shader = nullptr;
}

// --------------------------------------------------------
// Destructor - Clean up actual shader (base will be called automatically)
// --------------------------------------------------------
SimpleComputeShader::~SimpleComputeShader()
{
	CleanUp();
}

// --------------------------------------------------------
// Handles cleaning up shader and base class clean up
// --------------------------------------------------------
void SimpleComputeShader::CleanUp()
{
	ShaderResource::CleanUp();
	Memory::SafeDelete(shader);

	uavTable.clear();
}

// --------------------------------------------------------
// Creates the DirectX Compute shader
//
// shaderBlob - The shader's compiled code
//
// Returns true if shader is created correctly, false otherwise
// --------------------------------------------------------
bool SimpleComputeShader::CreateShader(ID3DBlob* shaderBlob)
{
	// Clean up first, in the event this method is
	// called more than once on the same object
	this->CleanUp();

	// Create the shader from the blob
	HRESULT result = device->CreateComputeShader(
		shaderBlob->GetBufferPointer(),
		shaderBlob->GetBufferSize(),
		0,
		&shader);

	// Was the shader created correctly?
	if (result != S_OK)
		return false;

	// Set up shader reflection to get information about UAV's
	ID3D11ShaderReflection* refl{};
	D3DReflect(
		shaderBlob->GetBufferPointer(),
		shaderBlob->GetBufferSize(),
		IID_ID3D11ShaderReflection,
		(void**)&refl);

	// Get the description of the shader
	D3D11_SHADER_DESC shaderDesc;
	refl->GetDesc(&shaderDesc);

	// Grab the thread info
	threadsTotal = refl->GetThreadGroupSize(
		&threadsX,
		&threadsY,
		&threadsZ);

	// Loop and get all UAV resources
	unsigned int resourceCount = shaderDesc.BoundResources;
	for (unsigned int r = 0; r < resourceCount; r++)
	{
		// Get this resource's description
		D3D11_SHADER_INPUT_BIND_DESC resourceDesc;
		refl->GetResourceBindingDesc(r, &resourceDesc);

		// Check the type, looking for any kind of UAV
		switch (resourceDesc.Type)
		{
		case D3D_SIT_UAV_APPEND_STRUCTURED:
		case D3D_SIT_UAV_CONSUME_STRUCTURED:
		case D3D_SIT_UAV_RWBYTEADDRESS:
		case D3D_SIT_UAV_RWSTRUCTURED:
		case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
		case D3D_SIT_UAV_RWTYPED:
			uavTable.insert(std::pair<std::string, unsigned int>(resourceDesc.Name, resourceDesc.BindPoint));
		}
	}

	// All set
	refl->Release();
	return true;
}

// --------------------------------------------------------
// Sets the Compute shader and constant buffers for
// future DirectX drawing
// --------------------------------------------------------
void SimpleComputeShader::SetShaderAndCBs()
{
	// Is shader valid?
	if (!shaderValid) return;

	// Set the shader
	deviceContext->CSSetShader(shader, 0, 0);

	// Set the constant buffers?
	for (unsigned int i = 0; i < constantBufferCount; i++)
	{
		deviceContext->CSSetConstantBuffers(
			constantBuffers[i].BindIndex,
			1,
			&constantBuffers[i].ConstantBuffer);
	}
}

// --------------------------------------------------------
// Dispatches the compute shader with the specified amount 
// of groups, using the number of threads per group
// specified in the shader file itself
//
// For example, calling this method with params (5,1,1) on
// a shader with (8,2,2) threads per group will launch a 
// total of 160 threads: ((5 * 8) * (1 * 2) * (1 * 2))
//
// This is identical to using the device context's 
// Dispatch() method yourself.  
//
// Note: This will dispatch the currently active shader, 
// not necessarily THIS shader. Be sure to activate this
// shader with SetShader() before calling Dispatch
//
// groupsX - Numbers of groups in the X dimension
// groupsY - Numbers of groups in the Y dimension
// groupsZ - Numbers of groups in the Z dimension
// --------------------------------------------------------
void SimpleComputeShader::DispatchByGroups(unsigned int groupsX, unsigned int groupsY, unsigned int groupsZ)
{
	deviceContext->Dispatch(groupsX, groupsY, groupsZ);
}

// --------------------------------------------------------
// Dispatches the compute shader with AT LEAST the 
// specified amount of threads, calculating the number of
// groups to dispatch using the number of threads per group
// specified in the shader file itself
//
// For example, calling this method with params (10,3,3) on
// a shader with (5,2,2) threads per group will launch 
// 8 total groups and 160 total threads, calculated by:
// Groups: ceil(10/5) * ceil(3/2) * ceil(3/2) = 8
// Threads: ((2 * 5) * (2 * 2) * (2 * 2)) = 160
//
// Note: This will dispatch the currently active shader, 
// not necessarily THIS shader. Be sure to activate this
// shader with SetShader() before calling Dispatch
//
// threadsX - Desired numbers of threads in the X dimension
// threadsY - Desired numbers of threads in the Y dimension
// threadsZ - Desired numbers of threads in the Z dimension
// --------------------------------------------------------
void SimpleComputeShader::DispatchByThreads(unsigned int threadsX, unsigned int threadsY, unsigned int threadsZ)
{
	deviceContext->Dispatch(
		max((unsigned int)ceil((float)threadsX / this->threadsX), 1),
		max((unsigned int)ceil((float)threadsY / this->threadsY), 1),
		max((unsigned int)ceil((float)threadsZ / this->threadsZ), 1));
}

// --------------------------------------------------------
// Sets a shader resource view in the Compute shader stage
//
// name - The name of the texture resource in the shader
// srv - The shader resource view of the texture in GPU memory
//
// Returns true if a texture of the given name was found, false otherwise
// --------------------------------------------------------
bool SimpleComputeShader::SetShaderResourceView(std::string name, ID3D11ShaderResourceView* srv)
{
	// Look for the variable and verify
	const ShaderResourceViewIndex* srvInfo = GetShaderResourceViewInfo(name);
	if (srvInfo == nullptr)
		return false;

	// Set the shader resource view
	deviceContext->CSSetShaderResources(srvInfo->BindIndex, 1, &srv);

	// Success
	return true;
}

// --------------------------------------------------------
// Sets a sampler state in the Compute shader stage
//
// name - The name of the sampler state in the shader
// samplerState - The sampler state in GPU memory
//
// Returns true if a sampler of the given name was found, false otherwise
// --------------------------------------------------------
bool SimpleComputeShader::SetSamplerState(std::string name, ID3D11SamplerState* samplerState)
{
	// Look for the variable and verify
	const ShaderSampler* sampInfo = GetSamplerInfo(name);
	if (sampInfo == nullptr)
		return false;

	// Set the shader resource view
	deviceContext->CSSetSamplers(sampInfo->BindIndex, 1, &samplerState);

	// Success
	return true;
}

// --------------------------------------------------------
// Sets an unordered access view in the Compute shader stage
//
// name - The name of the sampler state in the shader
// uav - The UAV in GPU memory
// appendConsumeOffset - Used for append or consume UAV's (optional)
//
// Returns true if a UAV of the given name was found, false otherwise
// --------------------------------------------------------
bool SimpleComputeShader::SetUnorderedAccessView(std::string name, ID3D11UnorderedAccessView* uav, unsigned int appendConsumeOffset)
{
	// Look for the variable and verify
	unsigned int bindIndex = GetUnorderedAccessViewIndex(name);
	if (bindIndex == -1)
		return false;

	// Set the shader resource view
	deviceContext->CSSetUnorderedAccessViews(bindIndex, 1, &uav, &appendConsumeOffset);

	// Success
	return true;
}

// --------------------------------------------------------
// Gets the index of the specified UAV (or -1)
// --------------------------------------------------------
int SimpleComputeShader::GetUnorderedAccessViewIndex(std::string name)
{
	// Look for the key
	std::unordered_map<std::string, unsigned int>::iterator result =
		uavTable.find(name);

	// Did we find the key?
	if (result == uavTable.end())
		return -1;

	// Success
	return result->second;
}
