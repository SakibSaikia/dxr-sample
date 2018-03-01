#include "stdafx.h"
#include "Material.h"

MaterialPipelineState DiffuseOnlyOpaqueMaterialPipeline::m_pipelineState;
MaterialPipelineState DiffuseOnlyMaskedMaterialPipeline::m_pipelineState;

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

// --------------------------------------------------------------------------

void DiffuseOnlyOpaqueMaterialPipeline::Init(ID3D12Device* device, D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc)
{
	// Shaders
	{
		m_pipelineState.vsByteCode = LoadBlob(R"(CompiledShaders\mtl_diffuse_only_opaque.vs.cso)");
		m_pipelineState.psByteCode = LoadBlob(R"(CompiledShaders\mtl_diffuse_only_opaque.ps.cso)");
	}

	// Root signature
	{
		Microsoft::WRL::ComPtr<ID3DBlob> rsBytecode = LoadBlob(R"(CompiledShaders\mtl_diffuse_only_opaque.rootsig.cso)");
		HRESULT hr = device->CreateRootSignature(
			0,
			rsBytecode->GetBufferPointer(),
			rsBytecode->GetBufferSize(),
			IID_PPV_ARGS(m_pipelineState.rootSignature.GetAddressOf())
		);
		assert(hr == S_OK && L"Failed to create root signature");
	}

	// Pipeline State
	{
		// root sig specified in shader
		psoDesc.pRootSignature = nullptr;

		// shaders
		psoDesc.VS.pShaderBytecode = m_pipelineState.vsByteCode.Get()->GetBufferPointer();
		psoDesc.VS.BytecodeLength = m_pipelineState.vsByteCode.Get()->GetBufferSize();
		psoDesc.PS.pShaderBytecode = m_pipelineState.psByteCode.Get()->GetBufferPointer();
		psoDesc.PS.BytecodeLength = m_pipelineState.psByteCode.Get()->GetBufferSize();

		HRESULT hr = device->CreateGraphicsPipelineState(
			&psoDesc,
			IID_PPV_ARGS(m_pipelineState.pso.GetAddressOf())
		);
		assert(hr == S_OK && L"Failed to create PSO");
	}
}

void DiffuseOnlyOpaqueMaterialPipeline::Bind(ID3D12GraphicsCommandList* cmdList)
{
	cmdList->SetGraphicsRootSignature(m_pipelineState.rootSignature.Get());
	cmdList->SetPipelineState(m_pipelineState.pso.Get());
}

// --------------------------------------------------------------------------

void DiffuseOnlyMaskedMaterialPipeline::Init(ID3D12Device* device, D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc)
{
	// Shaders
	{
		m_pipelineState.vsByteCode = LoadBlob(R"(CompiledShaders\mtl_diffuse_only_masked.vs.cso)");
		m_pipelineState.psByteCode = LoadBlob(R"(CompiledShaders\mtl_diffuse_only_masked.ps.cso)");
	}

	// Root signature
	{
		Microsoft::WRL::ComPtr<ID3DBlob> rsBytecode = LoadBlob(R"(CompiledShaders\mtl_diffuse_only_masked.rootsig.cso)");
		HRESULT hr = device->CreateRootSignature(
			0,
			rsBytecode->GetBufferPointer(),
			rsBytecode->GetBufferSize(),
			IID_PPV_ARGS(m_pipelineState.rootSignature.GetAddressOf())
		);
		assert(hr == S_OK && L"Failed to create root signature");
	}

	// Pipeline State
	{
		// root sig specified in shader
		psoDesc.pRootSignature = nullptr;

		// shaders
		psoDesc.VS.pShaderBytecode = m_pipelineState.vsByteCode.Get()->GetBufferPointer();
		psoDesc.VS.BytecodeLength = m_pipelineState.vsByteCode.Get()->GetBufferSize();
		psoDesc.PS.pShaderBytecode = m_pipelineState.psByteCode.Get()->GetBufferPointer();
		psoDesc.PS.BytecodeLength = m_pipelineState.psByteCode.Get()->GetBufferSize();

		HRESULT hr = device->CreateGraphicsPipelineState(
			&psoDesc,
			IID_PPV_ARGS(m_pipelineState.pso.GetAddressOf())
		);
		assert(hr == S_OK && L"Failed to create PSO");
	}
}

void DiffuseOnlyMaskedMaterialPipeline::Bind(ID3D12GraphicsCommandList* cmdList)
{
	cmdList->SetGraphicsRootSignature(m_pipelineState.rootSignature.Get());
	cmdList->SetPipelineState(m_pipelineState.pso.Get());
}

// --------------------------------------------------------------------------

DiffuseOnlyOpaqueMaterial::DiffuseOnlyOpaqueMaterial(std::string& name, const D3D12_GPU_DESCRIPTOR_HANDLE srvHandle) :
	m_name{ std::move(name) },
	m_srvBegin{ srvHandle },
	m_valid{ true }
{
}

void DiffuseOnlyOpaqueMaterial::BindPipeline(ID3D12GraphicsCommandList* cmdList) const
{
	pipeline_type::Bind(cmdList);
}

void DiffuseOnlyOpaqueMaterial::BindConstants(ID3D12GraphicsCommandList* cmdList) const
{
	cmdList->SetGraphicsRootDescriptorTable(
		pipeline_type::k_srvDescriptorTableIndex,
		m_srvBegin
	);
}

bool DiffuseOnlyOpaqueMaterial::IsValid() const
{
	return m_valid;
}

// --------------------------------------------------------------------------

DiffuseOnlyMaskedMaterial::DiffuseOnlyMaskedMaterial(std::string& name, const D3D12_GPU_DESCRIPTOR_HANDLE srvHandle) :
	m_name { std::move(name) },
	m_srvBegin{ srvHandle },
	m_valid{ true }
{
}

void DiffuseOnlyMaskedMaterial::BindPipeline(ID3D12GraphicsCommandList* cmdList) const
{
	pipeline_type::Bind(cmdList);
}

void DiffuseOnlyMaskedMaterial::BindConstants(ID3D12GraphicsCommandList* cmdList) const
{
	cmdList->SetGraphicsRootDescriptorTable(
		pipeline_type::k_srvDescriptorTableIndex,
		m_srvBegin
	);
}

bool DiffuseOnlyMaskedMaterial::IsValid() const
{
	return m_valid;
}
