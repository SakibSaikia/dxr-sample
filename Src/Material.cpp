#include "stdafx.h"
#include "Material.h"

Microsoft::WRL::ComPtr<ID3DBlob> LoadBlob(const std::string& filename)
{
	std::ifstream fileHandle(filename, std::ios::binary);
	assert(fileHandle.good() && L"Error opening file");

	// file size
	fileHandle.seekg(0, std::ios::end);
	std::ifstream::pos_type size = fileHandle.tellg();
	fileHandle.seekg(0, std::ios::beg);

	// serialize bytecode
	Microsoft::WRL::ComPtr<ID3DBlob> blob;
	HRESULT hr = D3DCreateBlob(size, blob.GetAddressOf());
	assert(hr == S_OK && L"Failed to create blob");
	fileHandle.read(static_cast<char*>(blob->GetBufferPointer()), size);

	fileHandle.close();

	return blob;
}

void Material::Init(ID3D12Device* device, D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc, std::string&& name, const uint32_t diffuseIdx)
{
	m_name = std::move(name);
	m_diffuseTextureIndex = diffuseIdx;

	// Shaders
	{
		m_vsByteCode = LoadBlob(R"(CompiledShaders\VertexShader.cso)");
		m_psByteCode = LoadBlob(R"(CompiledShaders\PixelShader.cso)");
	}

	// Root signature
	{
		Microsoft::WRL::ComPtr<ID3DBlob> rsBytecode = LoadBlob(R"(CompiledShaders\Material.sig)");
		HRESULT hr = device->CreateRootSignature(
			0,
			rsBytecode->GetBufferPointer(),
			rsBytecode->GetBufferSize(),
			IID_PPV_ARGS(m_rootSignature.GetAddressOf())
		);
		assert(hr == S_OK && L"Failed to create root signature");
	}

	// Pipeline State
	{
		// root sig specified in shader
		psoDesc.pRootSignature = nullptr;

		// shaders
		psoDesc.VS.pShaderBytecode = m_vsByteCode.Get()->GetBufferPointer();
		psoDesc.VS.BytecodeLength = m_vsByteCode.Get()->GetBufferSize();
		psoDesc.PS.pShaderBytecode = m_psByteCode.Get()->GetBufferPointer();
		psoDesc.PS.BytecodeLength = m_psByteCode.Get()->GetBufferSize();

		HRESULT hr = device->CreateGraphicsPipelineState(
			&psoDesc,
			IID_PPV_ARGS(m_pso.GetAddressOf())
		);
		assert(hr == S_OK && L"Failed to create PSO");
	}

	m_valid = true;
}

void Material::Bind(ID3D12GraphicsCommandList* cmdList) const
{
	cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
	cmdList->SetPipelineState(m_pso.Get());
}

bool Material::IsValid() const
{
	return m_valid;
}

uint32_t Material::GetDiffuseTextureIndex() const
{
	return m_diffuseTextureIndex;
}

uint32_t Material::GetDiffusemapRootParamIndex()
{
	return 2;
}