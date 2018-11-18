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

Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateRootSignature(ID3D12Device* device, const D3D12_ROOT_SIGNATURE_DESC& desc)
{
	Microsoft::WRL::ComPtr<ID3DBlob> rootSigBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

	HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, rootSigBlob.GetAddressOf(), errorBlob.GetAddressOf());
	assert(hr == S_OK && static_cast<char*>(errorBlob->GetBufferPointer()));

	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSig;
	hr = device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(rootSig.GetAddressOf()));
	assert(hr == S_OK && L"Failed to create root signature!");

	return rootSig;
}

size_t GetRootSignatureSizeInBytes(const D3D12_ROOT_SIGNATURE_DESC& desc)
{
	size_t size{ 0 };
	for (int i = 0; i < desc.NumParameters; i++)
	{
		switch (desc.pParameters[i].ParameterType)
		{
		case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
		case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
			size += 4;
			break;
		case D3D12_ROOT_PARAMETER_TYPE_CBV:
		case D3D12_ROOT_PARAMETER_TYPE_SRV:
		case D3D12_ROOT_PARAMETER_TYPE_UAV:
			size += 8;
			break;
		}
	}

	return size;
}

RaytraceMaterialPipeline::RaytraceMaterialPipeline(ID3D12Device5* device, RenderPass::Id renderPass, cconst std::string rgs, onst std::string chs, const std::string ms, const ID3D12RootSignature* rootSig)
{
	std::vector<D3D12_STATE_SUBOBJECT> rtSubObjects;
	rtSubObjects.reserve(10);

	// -------------------- Ray Generation Shader -------------------------------
	Microsoft::WRL::ComPtr<ID3DBlob> rgsByteCode = LoadBlob(k_rgs);
	std::wstring rgsName(rgs.begin(), rgs.end());
	std::wstring rgsExportName = std::wstring(L"RayGen_") + rgsName;

	D3D12_EXPORT_DESC rgsExportDesc{};
	rgsExportDesc.Name = L"RayGen";
	rgsExportDesc.ExportToRename = rgsExportName.c_str();
	

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

void RaytraceMaterialPipeline::Bind(ID3D12GraphicsCommandList4* cmdList) const
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
	auto& pipeline = m_raytracePipelines[pass];

	// Create a new pipeline and cache it if it has not been created
	if (!pipeline)
	{
		rootSig = CreateRaytraceRootSignature(device, pass);
		pipeline = std::make_unique<RaytraceMaterialPipeline>(device, pass, k_rgs, k_chs, k_ms, rootSig);
	}

	pipeline->Bind(cmdList);
}

void DefaultOpaqueMaterial::CreateRaytraceRootSignature(RenderPass::Id pass)
{
	if (pass == RenderPass::Raytrace)
	{
		std::vector<D3D12_ROOT_PARAMETER> rootParams;
		rootParams.reserve(3);

		// Constant buffers
		D3D12_ROOT_PARAMETER viewCBV{};
		viewCBV.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		viewCBV.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		viewCBV.Descriptor.ShaderRegister = 0;
		rootParams.push_back(viewCBV);

		D3D12_ROOT_PARAMETER lightCBV{};
		lightCBV.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		lightCBV.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		lightCBV.Descriptor.ShaderRegister = 1;
		rootParams.push_back(lightCBV);

		D3D12_ROOT_PARAMETER materialCBV{};
		materialCBV.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		materialCBV.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		materialCBV.Descriptor.ShaderRegister = 2;
		rootParams.push_back(materialCBV);

		// UAV
		D3D12_ROOT_PARAMETER outputUAV{};
		outputUAV.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
		outputUAV.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		outputUAV.Descriptor.ShaderRegister = 0;
		rootParams.push_back(outputUAV);

		// Geometry SRVs
		D3D12_DESCRIPTOR_RANGE geometrySRVRange{};
		geometrySRVRange.BaseShaderRegister = 0;
		geometrySRVRange.NumDescriptors = 3; // TLAS, VB, IB
		geometrySRVRange.RegisterSpace = 0;
		geometrySRVRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		geometrySRVRange.OffsetInDescriptorsFromTableStart = 0;

		D3D12_ROOT_PARAMETER geometrySRVs{};
		geometrySRVs.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		geometrySRVs.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		geometrySRVs.DescriptorTable.NumDescriptorRanges = 1;
		geometrySRVs.DescriptorTable.pDescriptorRanges = &geometrySRVRange;
		rootParams.push_back(geometrySRVs);

		// Material SRVs
		D3D12_DESCRIPTOR_RANGE materialSRVRange{};
		materialSRVRange.BaseShaderRegister = 0;
		materialSRVRange.NumDescriptors = 4;
		materialSRVRange.RegisterSpace = 1;
		materialSRVRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		materialSRVRange.OffsetInDescriptorsFromTableStart = 0;

		D3D12_ROOT_PARAMETER materialSRVs{};
		materialSRVs.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		materialSRVs.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		materialSRVs.DescriptorTable.NumDescriptorRanges = 1;
		materialSRVs.DescriptorTable.pDescriptorRanges = &materialSRVRange;
		rootParams.push_back(materialSRVs);

		D3D12_ROOT_SIGNATURE_DESC rootDesc{};
		rootDesc.Flags = rootParameters.size();
		rootDesc.pParameters = rootParameters.data();
		rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

		m_raytraceRootSignatures[pass] = CreateRootSignature(device, rootDesc);
		m_raytraceRootSignatureSizes[pass] = GetRootSignatureSizeInBytes(rootDesc);
	}
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
	auto& pipeline = m_raytracePipelines[pass];

	// Create a new pipeline and cache it if it has not been created
	if (!pipeline)
	{
		rootSig = CreateRaytraceRootSignature(device, pass);
		pipeline = std::make_unique<RaytraceMaterialPipeline>(device, pass, k_rgs, k_chs, k_ms, rootSig);
	}

	pipeline->Bind(cmdList);
}

void DefaultMaskedMaterial::CreateRaytraceRootSignature(RenderPass::Id pass)
{
	if (pass == RenderPass::Raytrace)
	{
		std::vector<D3D12_ROOT_PARAMETER> rootParams;
		rootParams.reserve(3);

		// Constant buffers
		D3D12_ROOT_PARAMETER viewCBV{};
		viewCBV.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		viewCBV.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		viewCBV.Descriptor.ShaderRegister = 0;
		rootParams.push_back(viewCBV);

		D3D12_ROOT_PARAMETER lightCBV{};
		lightCBV.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		lightCBV.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		lightCBV.Descriptor.ShaderRegister = 1;
		rootParams.push_back(lightCBV);

		D3D12_ROOT_PARAMETER materialCBV{};
		materialCBV.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		materialCBV.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		materialCBV.Descriptor.ShaderRegister = 2;
		rootParams.push_back(materialCBV);

		// UAV
		D3D12_ROOT_PARAMETER outputUAV{};
		outputUAV.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
		outputUAV.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		outputUAV.Descriptor.ShaderRegister = 0;
		rootParams.push_back(outputUAV);

		// Geometry SRVs
		D3D12_DESCRIPTOR_RANGE geometrySRVRange{};
		geometrySRVRange.BaseShaderRegister = 0;
		geometrySRVRange.NumDescriptors = 3; // TLAS, VB, IB
		geometrySRVRange.RegisterSpace = 0;
		geometrySRVRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		geometrySRVRange.OffsetInDescriptorsFromTableStart = 0;

		D3D12_ROOT_PARAMETER geometrySRVs{};
		geometrySRVs.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		geometrySRVs.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		geometrySRVs.DescriptorTable.NumDescriptorRanges = 1;
		geometrySRVs.DescriptorTable.pDescriptorRanges = &geometrySRVRange;
		rootParams.push_back(geometrySRVs);

		// Material SRVs
		D3D12_DESCRIPTOR_RANGE materialSRVRange{};
		materialSRVRange.BaseShaderRegister = 0;
		materialSRVRange.NumDescriptors = 5;
		materialSRVRange.RegisterSpace = 1;
		materialSRVRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		materialSRVRange.OffsetInDescriptorsFromTableStart = 0;

		D3D12_ROOT_PARAMETER materialSRVs{};
		materialSRVs.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		materialSRVs.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		materialSRVs.DescriptorTable.NumDescriptorRanges = 1;
		materialSRVs.DescriptorTable.pDescriptorRanges = &materialSRVRange;
		rootParams.push_back(materialSRVs);

		D3D12_ROOT_SIGNATURE_DESC rootDesc{};
		rootDesc.Flags = rootParameters.size();
		rootDesc.pParameters = rootParameters.data();
		rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

		m_raytraceRootSignatures[pass] = CreateRootSignature(device, rootDesc);
		m_raytraceRootSignatureSizes[pass] = GetRootSignatureSizeInBytes(rootDesc);
	}
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
	auto& pipeline = m_raytracePipelines[pass];

	// Create a new pipeline and cache it if it has not been created
	if (!pipeline)
	{
		rootSig = CreateRaytraceRootSignature(device, pass);
		pipeline = std::make_unique<RaytraceMaterialPipeline>(device, pass, k_rgs, k_chs, k_ms, rootSig);
	}

	pipeline->Bind(cmdList);
}

void UntexturedMaterial::CreateRaytraceRootSignature(RenderPass::Id pass)
{
	if (pass == RenderPass::Raytrace)
	{
		std::vector<D3D12_ROOT_PARAMETER> rootParams;
		rootParams.reserve(3);

		// Constant buffers
		D3D12_ROOT_PARAMETER viewCBV{};
		viewCBV.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		viewCBV.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		viewCBV.Descriptor.ShaderRegister = 0;
		rootParams.push_back(viewCBV);

		D3D12_ROOT_PARAMETER lightCBV{};
		lightCBV.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		lightCBV.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		lightCBV.Descriptor.ShaderRegister = 1;
		rootParams.push_back(lightCBV);

		D3D12_ROOT_PARAMETER materialCBV{};
		materialCBV.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		materialCBV.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		materialCBV.Descriptor.ShaderRegister = 2;
		rootParams.push_back(materialCBV);

		// UAV
		D3D12_ROOT_PARAMETER outputUAV{};
		outputUAV.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
		outputUAV.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		outputUAV.Descriptor.ShaderRegister = 0;
		rootParams.push_back(outputUAV);

		// Geometry SRVs
		D3D12_DESCRIPTOR_RANGE geometrySRVRange{};
		geometrySRVRange.BaseShaderRegister = 0;
		geometrySRVRange.NumDescriptors = 3; // TLAS, VB, IB
		geometrySRVRange.RegisterSpace = 0;
		geometrySRVRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		geometrySRVRange.OffsetInDescriptorsFromTableStart = 0;

		D3D12_ROOT_PARAMETER geometrySRVs{};
		geometrySRVs.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		geometrySRVs.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		geometrySRVs.DescriptorTable.NumDescriptorRanges = 1;
		geometrySRVs.DescriptorTable.pDescriptorRanges = &geometrySRVRange;
		rootParams.push_back(geometrySRVs);

		D3D12_ROOT_SIGNATURE_DESC rootDesc{};
		rootDesc.Flags = rootParameters.size();
		rootDesc.pParameters = rootParameters.data();
		rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

		m_raytraceRootSignatures[pass] = CreateRootSignature(device, rootDesc);
		m_raytraceRootSignatureSizes[pass] = GetRootSignatureSizeInBytes(rootDesc);
	}
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