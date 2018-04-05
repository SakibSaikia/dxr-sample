#include "stdafx.h"
#include "Material.h"

Microsoft::WRL::ComPtr<ID3DBlob> LoadBlob(const std::string& filename)
{
	std::string filepath = R"(CompiledShaders\)" + filename;
	std::ifstream fileHandle(filepath, std::ios::binary);
	assert(fileHandle.good() && L"Error opening file");

	// file size
	fileHandle.seekg(0, std::ios::end);
	std::ifstream::pos_type size = fileHandle.tellg();
	fileHandle.seekg(0, std::ios::beg);

	// serialize bytecode
	Microsoft::WRL::ComPtr<ID3DBlob> blob;
	CHECK(D3DCreateBlob(size, blob.GetAddressOf()));
	fileHandle.read(static_cast<char*>(blob->GetBufferPointer()), size);

	fileHandle.close();

	return blob;
}

MaterialPipeline::MaterialPipeline(const MaterialPipelineDescription& desc, ID3D12Device* device, D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc) :
	m_type{desc.type}, m_descriptorTableIndex{desc.descriptorTableIndex}
{
	// Shaders
	{
		vsByteCode = LoadBlob(desc.vsBlobName);
		psByteCode = LoadBlob(desc.psBlobName);
	}

	// Root signature
	{
		Microsoft::WRL::ComPtr<ID3DBlob> rsBytecode = LoadBlob(desc.rootsigBlobName);
		CHECK(device->CreateRootSignature(
			0,
			rsBytecode->GetBufferPointer(),
			rsBytecode->GetBufferSize(),
			IID_PPV_ARGS(rootSignature.GetAddressOf())
		));
	}

	// Pipeline State
	{
		// root sig specified in shader
		psoDesc.pRootSignature = nullptr;

		// shaders
		psoDesc.VS.pShaderBytecode = vsByteCode.Get()->GetBufferPointer();
		psoDesc.VS.BytecodeLength = vsByteCode.Get()->GetBufferSize();
		psoDesc.PS.pShaderBytecode = psByteCode.Get()->GetBufferPointer();
		psoDesc.PS.BytecodeLength = psByteCode.Get()->GetBufferSize();

		CHECK(device->CreateGraphicsPipelineState(
			&psoDesc,
			IID_PPV_ARGS(pso.GetAddressOf())
		));
	}
}

void MaterialPipeline::Bind(ID3D12GraphicsCommandList* cmdList) const
{
	cmdList->SetGraphicsRootSignature(rootSignature.Get());
	cmdList->SetPipelineState(pso.Get());
}

MaterialType MaterialPipeline::GetType() const
{
	return m_type;
}

uint32_t MaterialPipeline::GetDescriptorTableIndex() const
{
	return m_descriptorTableIndex;
}

Material::Material(const MaterialPipeline* pipeline, std::string& name, const D3D12_GPU_DESCRIPTOR_HANDLE srvHandle) :
	m_pipeline{ pipeline },
	m_name{ std::move(name) },
	m_descriptorTableIndex { pipeline->GetDescriptorTableIndex()},
	m_srvBegin{ srvHandle },
	m_valid{ true }
{
}

void Material::BindPipeline(ID3D12GraphicsCommandList* cmdList)
{
	m_pipeline->Bind(cmdList);
}

void Material::BindConstants(ID3D12GraphicsCommandList* cmdList) const
{
	assert(m_descriptorTableIndex == m_pipeline->GetDescriptorTableIndex());
	cmdList->SetGraphicsRootDescriptorTable(m_descriptorTableIndex, m_srvBegin);
}

bool Material::IsValid() const
{
	return m_valid;
}