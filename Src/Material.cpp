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

MaterialPipeline::MaterialPipeline(ID3D12Device5* device, RenderPass::Id renderPass, cconst std::string rgs, onst std::string chs, const std::string ms, const ID3D12RootSignature* rootSig)
{
	std::vector<D3D12_STATE_SUBOBJECT> rtSubObjects;
	rtSubObjects.reserve(10);

	// -------------------- Ray Generation Shader -------------------------------
	Microsoft::WRL::ComPtr<ID3DBlob> rgsByteCode = LoadBlob(k_rgs);

	D3D12_EXPORT_DESC rgsExportDesc{};
	rgsExportDesc.Name = L"RayGen";
	std::wstring rgsName(rgs.begin(), rgs.end());
	std::wstring rgsExportName = std::wstring(L"RayGen_") + rgsName;

	D3D12_DXIL_LIBRARY_DESC rgsLibDesc{};
	rgsLibDesc.DXILLibrary.pShaderBytecode = rgsByteCode.Get()->GetBufferPointer();
	rgsLibDesc.DXILLibrary.BytecodeLength = rgsByteCode.Get()->GetBufferSize();
	rgsLibDesc.NumExports = 1;
	rgsLibDesc.pExports = &rgsExportDesc;

	D3D12_STATE_SUBOBJECT rgsSubObject{};
	rgsSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	rgsSubObject.pDesc = &rgsLibDesc;
	rtSubObjects.push_back(rgsSubObject);

	// -------------------- Closest Hit Shader -----------------------------------
	Microsoft::WRL::ComPtr<ID3DBlob> chsByteCode = LoadBlob(chs);
	std::wstring chsName(chs.begin(), chs.end());
	std::wstring chsExportName = std::wstring(L"ClosestHit_") + chsName;

	D3D12_EXPORT_DESC chsExportDesc{};
	chsExportDesc.Name = L"ClosestHit";
	chsExportDesc.ExportToRename = chsExportName.c_str();

	D3D12_DXIL_LIBRARY_DESC chsLibDesc{};
	chsLibDesc.DXILLibrary.pShaderBytecode = chsByteCode.Get()->GetBufferPointer();
	chsLibDesc.DXILLibrary.BytecodeLength = chsByteCode.Get()->GetBufferSize();
	chsLibDesc.NumExports = 1;
	chsLibDesc.pExports = &chsExportDesc;

	D3D12_STATE_SUBOBJECT chsSubObject{};
	chsSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	chsSubObject.pDesc = &chsLibDesc;
	rtSubObjects.push_back(chsSubObject);

	// --------------------- Miss Shader ----------------------------------------
	Microsoft::WRL::ComPtr<ID3DBlob> msByteCode = LoadBlob(ms);
	std::wstring msName(ms.begin(), ms.end());
	std::wstring msExportName = std::wstring(L"Miss_") + msName;

	D3D12_EXPORT_DESC msExportDesc{};
	msExportDesc.Name = L"Miss";
	msExportDesc.ExportToRename = msExportName.c_str();

	D3D12_DXIL_LIBRARY_DESC msLibDesc{};
	msLibDesc.DXILLibrary.pShaderBytecode = msByteCode.Get()->GetBufferPointer();
	msLibDesc.DXILLibrary.BytecodeLength = msByteCode.Get()->GetBufferSize();
	msLibDesc.NumExports = 1;
	msLibDesc.pExports = &msExportDesc;

	D3D12_STATE_SUBOBJECT msSubObject{};
	msSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	msSubObject.pDesc = &msLibDesc;
	rtSubObjects.push_back(msSubObject);

	// --------------------- Hit Group ------------------------------------------
	D3D12_HIT_GROUP_DESC hitGroupDesc{};
	hitGroupDesc.ClosestHitShaderImport = L"ClosestHit";
	hitGroupDesc.HitGroupExport = L"HitGroup";

	D3D12_STATE_SUBOBJECT hitGroupSubObject{};
	hitGroupSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
	hitGroupSubObject.pDesc = &hitGroupDesc;
	rtSubObjects.push_back(hitGroupSubObject);

	// ---------------------- Shader Config -------------------------------------
	D3D12_RAYTRACING_SHADER_CONFIG shaderConfigDesc{};
	shaderConfigDesc.MaxPayloadSizeInBytes = sizeof(XMFLOAT4);
	shaderConfigDesc.MaxAttributeSizeInBytes = D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES;

	D3D12_STATE_SUBOBJECT shaderConfigSubObject{};
	shaderConfigSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
	shaderConfigSubObject.pDesc = &shaderConfigDesc;
	rtSubObjects.push_back(shaderConfigSubObject);

	// ---------------- Shader Config Association ------------------------------
	cost wchar_t* shaderExports[] = {
		rgsExportDesc.Name,
		msExportDesc.Name,
		hitGroupDesc.HitGroupExport
	};

	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION shaderConfigAssociationDesc{};
	shaderConfigAssociationDesc.pExports = shaderExports;
	shaderConfigAssociationDesc.NumExports = std::extent<decltype(shaderExports)>::value;
	shaderConfigAssociationDesc.pSubobjectToAssociate = &rtSubObjects.back();

	D3D12_STATE_SUBOBJECT shaderConfigAssociationSubObject{};
	shaderConfigAssociationSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	shaderConfigAssociationSubObject.pDesc = &shaderConfigAssociationDesc;
	rtSubObjects.push_back(shaderConfigAssociationSubObject);

	// --------------- Shared Root Signature ------------------------------------
	D3D12_STATE_SUBOBJECT sharedRootSigSubObject{};
	sharedRootSigSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	sharedRootSigSubObject.pDesc = &rootSig;
	rtSubObjects.push_back(sharedRootSigSubObject);

	// --------------- Shared Root Signature Association ------------------------
	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION sharedRootSignatureAssociationDesc{};
	sharedRootSignatureAssociationDesc.pExports = shaderExports;
	sharedRootSignatureAssociationDesc.NumExports = std::extent<decltype(shaderExports)>::value;
	sharedRootSignatureAssociationDesc.pSubobjectToAssociate = &rtSubObjects.back();

	D3D12_STATE_SUBOBJECT sharedRootSignatureAssociationSubObject{};
	sharedRootSignatureAssociationSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	sharedRootSignatureAssociationSubObject.pDesc = &sharedRootSignatureAssociationDesc;
	rtSubObjects.push_back(sharedRootSignatureAssociationSubObject);

	// ---------------- Raytracing Pipeline Config ------------------------------
	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfigDesc{};
	pipelineConfigDesc.MaxTraceRecursionDepth = 1;

	D3D12_STATE_SUBOBJECT pipelineConfigSubObject{};
	pipelineConfigSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
	pipelineConfigSubObject.pDesc = &pipelineConfigDesc;

	// --------------------------------------------------------------------------


	// Finally, create the pipeline state!
	D3D12_STATE_OBJECT_DESC psoDesc;
	psoDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
	psoDesc.NumSubobjects = rtSubObjects.size();
	psoDesc.pSubobjects = rtSubObjects.data();

	CHECK(device->CreateStateObject(
		&psoDesc,
		IID_PPV_ARGS(m_pso.GetAddressOf())
	));

	// Get the PSO properties
	CHECK(m_pso->QueryInterface(IID_PPV_ARGS(m_psoProperties.GetAddressOf())));
}

void MaterialPipeline::Bind(ID3D12GraphicsCommandList4* cmdList) const
{
	cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
	cmdList->SetPipelineState(m_pso.Get());
}

Material::Material(const std::string& name) :
	m_valid{ true },
	m_name { std::move(name) },
	m_pipelines{}
{
}

bool Material::IsValid() const
{
	return m_valid;
}

DefaultOpaqueMaterial::DefaultOpaqueMaterial(std::string& name, const D3D12_GPU_DESCRIPTOR_HANDLE srvHandle) :
	Material{ name },
	m_srvBegin{ srvHandle }
{
	m_hash[RenderPass::Raytrace] =
		(std::hash<std::string>{}(k_rgs)) ^
		(std::hash<std::string>{}(k_chs) << 1) ^
		(std::hash<std::string>{}(k_ms) << 2);
}

void DefaultOpaqueMaterial::BindPipeline(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, RenderPass::Id pass)
{
	auto& pipeline = m_pipelines[pass];

	// Create a new pipeline and cache it if it has not been created
	if (!pipeline)
	{
		if (pass == RenderPass::Raytrace)
		{
			rootSig = CreateSharedRootSignature();
			pipeline = std::make_unique<MaterialPipeline>(device, pass, k_rgs, k_chs, k_ms, rootSig);
		}
	}

	pipeline->Bind(cmdList);
}

void DefaultOpaqueMaterial::BindConstants(RenderPass::Id pass, ID3D12GraphicsCommandList4* cmdList, D3D12_GPU_VIRTUAL_ADDRESS objConstants, D3D12_GPU_VIRTUAL_ADDRESS viewConstants, D3D12_GPU_VIRTUAL_ADDRESS lightConstants, D3D12_GPU_VIRTUAL_ADDRESS shadowConstants, D3D12_GPU_DESCRIPTOR_HANDLE renderSurfaceSrvBegin) const
{
	if (pass == RenderPass::Geometry)
	{
		cmdList->SetGraphicsRootConstantBufferView(0, viewConstants);
		cmdList->SetGraphicsRootConstantBufferView(1, objConstants);
		cmdList->SetGraphicsRootConstantBufferView(2, lightConstants);
		cmdList->SetGraphicsRootConstantBufferView(3, shadowConstants);
		cmdList->SetGraphicsRootDescriptorTable(5, renderSurfaceSrvBegin);
		cmdList->SetGraphicsRootDescriptorTable(6, m_srvBegin);
	}
	else if (pass == RenderPass::DepthOnly)
	{
		cmdList->SetGraphicsRootConstantBufferView(0, viewConstants);
		cmdList->SetGraphicsRootConstantBufferView(1, objConstants);
	}
	else if (pass == RenderPass::Shadowmap)
	{
		cmdList->SetGraphicsRootConstantBufferView(0, shadowConstants);
		cmdList->SetGraphicsRootConstantBufferView(1, objConstants);
	}
}

size_t DefaultOpaqueMaterial::GetHash(RenderPass::Id renderPass, VertexFormat::Type vertexFormat) const
{
	return	m_hash[renderPass] ^ (renderPass << 3) ^ (static_cast<int>(vertexFormat) << 4);
}

DefaultMaskedMaterial::DefaultMaskedMaterial(std::string& name, const D3D12_GPU_DESCRIPTOR_HANDLE srvHandle, const D3D12_GPU_DESCRIPTOR_HANDLE opacityMaskSrvHandle) :
	Material{ name },
	m_srvBegin{ srvHandle },
	m_opacityMaskSrv{opacityMaskSrvHandle}
{
	m_hash[RenderPass::Raytrace] = 
		(std::hash<std::string>{}(k_rgs)) ^
		(std::hash<std::string>{}(k_chs) << 1) ^
		(std::hash<std::string>{}(k_ms) << 2);
}

void DefaultMaskedMaterial::BindPipeline(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, RenderPass::Id pass)
{
	auto& pipeline = m_pipelines[pass];

	// Create a new pipeline and cache it if it has not been created
	if (!pipeline)
	{
		if (pass == RenderPass::Raytrace)
		{
			rootSig = CreateSharedRootSignature();
			pipeline = std::make_unique<MaterialPipeline>(device, pass, k_rgs, k_chs, k_ms, rootSig);
		}
	}

	pipeline->Bind(cmdList);
}

void DefaultMaskedMaterial::BindConstants(RenderPass::Id pass, ID3D12GraphicsCommandList4* cmdList, D3D12_GPU_VIRTUAL_ADDRESS objConstants, D3D12_GPU_VIRTUAL_ADDRESS viewConstants, D3D12_GPU_VIRTUAL_ADDRESS lightConstants, D3D12_GPU_VIRTUAL_ADDRESS shadowConstants, D3D12_GPU_DESCRIPTOR_HANDLE renderSurfaceSrvBegin) const
{
	if (pass == RenderPass::Geometry)
	{
		cmdList->SetGraphicsRootConstantBufferView(0, viewConstants);
		cmdList->SetGraphicsRootConstantBufferView(1, objConstants);
		cmdList->SetGraphicsRootConstantBufferView(2, lightConstants);
		cmdList->SetGraphicsRootConstantBufferView(3, shadowConstants);
		cmdList->SetGraphicsRootDescriptorTable(5, renderSurfaceSrvBegin);
		cmdList->SetGraphicsRootDescriptorTable(6, m_srvBegin);
	}
	else if (pass == RenderPass::DepthOnly)
	{
		cmdList->SetGraphicsRootConstantBufferView(0, viewConstants);
		cmdList->SetGraphicsRootConstantBufferView(1, objConstants);
		cmdList->SetGraphicsRootDescriptorTable(2, m_opacityMaskSrv);
	}
	else if (pass == RenderPass::Shadowmap)
	{
		cmdList->SetGraphicsRootConstantBufferView(0, shadowConstants);
		cmdList->SetGraphicsRootConstantBufferView(1, objConstants);
		cmdList->SetGraphicsRootDescriptorTable(2, m_opacityMaskSrv);
	}
}

size_t DefaultMaskedMaterial::GetHash(RenderPass::Id renderPass, VertexFormat::Type vertexFormat) const
{
	return	m_hash[renderPass] ^ (renderPass << 3) ^ (static_cast<int>(vertexFormat) << 4);
}

UntexturedMaterial::UntexturedMaterial(std::string& name, Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer) :
	Material{ name },
	m_constantBuffer{ constantBuffer }
{
	m_hash[RenderPass::Raytrace] =
		(std::hash<std::string>{}(k_rgs)) ^
		(std::hash<std::string>{}(k_chs) << 1) ^
		(std::hash<std::string>{}(k_ms) << 2);
}

void UntexturedMaterial::BindPipeline(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, RenderPass::Id pass)
{
	auto& pipeline = m_pipelines[pass];

	// Create a new pipeline and cache it if it has not been created
	if (!pipeline)
	{
		if (pass == RenderPass::Raytrace)
		{
			rootSig = CreateSharedRootSignature();
			pipeline = std::make_unique<MaterialPipeline>(device, pass, k_rgs, k_chs, k_ms, rootSig);
		}
	}

	pipeline->Bind(cmdList);
}

void UntexturedMaterial::BindConstants(RenderPass::Id pass, ID3D12GraphicsCommandList4* cmdList, D3D12_GPU_VIRTUAL_ADDRESS objConstants, D3D12_GPU_VIRTUAL_ADDRESS viewConstants, D3D12_GPU_VIRTUAL_ADDRESS lightConstants, D3D12_GPU_VIRTUAL_ADDRESS shadowConstants, D3D12_GPU_DESCRIPTOR_HANDLE renderSurfaceSrvBegin) const
{
	if (pass == RenderPass::Geometry)
	{
		cmdList->SetGraphicsRootConstantBufferView(0, viewConstants);
		cmdList->SetGraphicsRootConstantBufferView(1, objConstants);
		cmdList->SetGraphicsRootConstantBufferView(2, lightConstants);
		cmdList->SetGraphicsRootConstantBufferView(3, shadowConstants);
		cmdList->SetGraphicsRootConstantBufferView(4, m_constantBuffer->GetGPUVirtualAddress());
		cmdList->SetGraphicsRootDescriptorTable(5, renderSurfaceSrvBegin);
	}
	else if (pass == RenderPass::DepthOnly)
	{
		cmdList->SetGraphicsRootConstantBufferView(0, viewConstants);
		cmdList->SetGraphicsRootConstantBufferView(1, objConstants);
	}
	else if (pass == RenderPass::Shadowmap)
	{
		cmdList->SetGraphicsRootConstantBufferView(0, shadowConstants);
		cmdList->SetGraphicsRootConstantBufferView(1, objConstants);
	}
}

size_t UntexturedMaterial::GetHash(RenderPass::Id renderPass, VertexFormat::Type vertexFormat) const
{
	return	m_hash[renderPass] ^ (renderPass << 3) ^ (static_cast<int>(vertexFormat) << 4);
}