#include "stdafx.h"
#include "Material.h"

MaterialPipelineState DiffuseOnlyOpaqueMaterialPipeline::m_pipelineState;
MaterialPipelineState DiffuseOnlyMaskedMaterialPipeline::m_pipelineState;
MaterialPipelineState NormalMappedOpaqueMaterialPipeline::m_pipelineState;
MaterialPipelineState NormalMappedMaskedMaterialPipeline::m_pipelineState;

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
	DX_VERIFY(D3DCreateBlob(size, blob.GetAddressOf()));
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
		DX_VERIFY(device->CreateRootSignature(
			0,
			rsBytecode->GetBufferPointer(),
			rsBytecode->GetBufferSize(),
			IID_PPV_ARGS(m_pipelineState.rootSignature.GetAddressOf())
		));
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

		DX_VERIFY(device->CreateGraphicsPipelineState(
			&psoDesc,
			IID_PPV_ARGS(m_pipelineState.pso.GetAddressOf())
		));
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
		DX_VERIFY(device->CreateRootSignature(
			0,
			rsBytecode->GetBufferPointer(),
			rsBytecode->GetBufferSize(),
			IID_PPV_ARGS(m_pipelineState.rootSignature.GetAddressOf())
		));
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

		DX_VERIFY(device->CreateGraphicsPipelineState(
			&psoDesc,
			IID_PPV_ARGS(m_pipelineState.pso.GetAddressOf())
		));
	}
}

void DiffuseOnlyMaskedMaterialPipeline::Bind(ID3D12GraphicsCommandList* cmdList)
{
	cmdList->SetGraphicsRootSignature(m_pipelineState.rootSignature.Get());
	cmdList->SetPipelineState(m_pipelineState.pso.Get());
}

// --------------------------------------------------------------------------

void NormalMappedOpaqueMaterialPipeline::Init(ID3D12Device* device, D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc)
{
	// Shaders
	{
		m_pipelineState.vsByteCode = LoadBlob(R"(CompiledShaders\mtl_normal_mapped_opaque.vs.cso)");
		m_pipelineState.psByteCode = LoadBlob(R"(CompiledShaders\mtl_normal_mapped_opaque.ps.cso)");
	}

	// Root signature
	{
		Microsoft::WRL::ComPtr<ID3DBlob> rsBytecode = LoadBlob(R"(CompiledShaders\mtl_normal_mapped_opaque.rootsig.cso)");
		DX_VERIFY(device->CreateRootSignature(
			0,
			rsBytecode->GetBufferPointer(),
			rsBytecode->GetBufferSize(),
			IID_PPV_ARGS(m_pipelineState.rootSignature.GetAddressOf())
		));
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

		DX_VERIFY(device->CreateGraphicsPipelineState(
			&psoDesc,
			IID_PPV_ARGS(m_pipelineState.pso.GetAddressOf())
		));
	}
}

void NormalMappedOpaqueMaterialPipeline::Bind(ID3D12GraphicsCommandList* cmdList)
{
	cmdList->SetGraphicsRootSignature(m_pipelineState.rootSignature.Get());
	cmdList->SetPipelineState(m_pipelineState.pso.Get());
}

// --------------------------------------------------------------------------

void NormalMappedMaskedMaterialPipeline::Init(ID3D12Device* device, D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc)
{
	// Shaders
	{
		m_pipelineState.vsByteCode = LoadBlob(R"(CompiledShaders\mtl_normal_mapped_masked.vs.cso)");
		m_pipelineState.psByteCode = LoadBlob(R"(CompiledShaders\mtl_normal_mapped_masked.ps.cso)");
	}

	// Root signature
	{
		Microsoft::WRL::ComPtr<ID3DBlob> rsBytecode = LoadBlob(R"(CompiledShaders\mtl_normal_mapped_masked.rootsig.cso)");
		DX_VERIFY(device->CreateRootSignature(
			0,
			rsBytecode->GetBufferPointer(),
			rsBytecode->GetBufferSize(),
			IID_PPV_ARGS(m_pipelineState.rootSignature.GetAddressOf())
		));
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

		DX_VERIFY(device->CreateGraphicsPipelineState(
			&psoDesc,
			IID_PPV_ARGS(m_pipelineState.pso.GetAddressOf())
		));
	}
}

void NormalMappedMaskedMaterialPipeline::Bind(ID3D12GraphicsCommandList* cmdList)
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

// --------------------------------------------------------------------------

NormalMappedOpaqueMaterial::NormalMappedOpaqueMaterial(std::string& name, const D3D12_GPU_DESCRIPTOR_HANDLE srvHandle) :
	m_name{ std::move(name) },
	m_srvBegin{ srvHandle },
	m_valid{ true }
{
}

void NormalMappedOpaqueMaterial::BindPipeline(ID3D12GraphicsCommandList* cmdList) const
{
	pipeline_type::Bind(cmdList);
}

void NormalMappedOpaqueMaterial::BindConstants(ID3D12GraphicsCommandList* cmdList) const
{
	cmdList->SetGraphicsRootDescriptorTable(
		pipeline_type::k_srvDescriptorTableIndex,
		m_srvBegin
	);
}

bool NormalMappedOpaqueMaterial::IsValid() const
{
	return m_valid;
}

// --------------------------------------------------------------------------

NormalMappedMaskedMaterial::NormalMappedMaskedMaterial(std::string& name, const D3D12_GPU_DESCRIPTOR_HANDLE srvHandle) :
	m_name{ std::move(name) },
	m_srvBegin{ srvHandle },
	m_valid{ true }
{
}

void NormalMappedMaskedMaterial::BindPipeline(ID3D12GraphicsCommandList* cmdList) const
{
	pipeline_type::Bind(cmdList);
}

void NormalMappedMaskedMaterial::BindConstants(ID3D12GraphicsCommandList* cmdList) const
{
	cmdList->SetGraphicsRootDescriptorTable(
		pipeline_type::k_srvDescriptorTableIndex,
		m_srvBegin
	);
}

bool NormalMappedMaskedMaterial::IsValid() const
{
	return m_valid;
}
