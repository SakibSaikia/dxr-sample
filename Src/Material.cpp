#include "stdafx.h"
#include "Material.h"

namespace
{
	std::string k_rgs = "mtl_untextured.rgs";
	std::unique_ptr<RaytraceMaterialPipeline> s_raytracePipelines[RenderPass::Count];
}

ID3DBlob* LoadBlob(const std::string& filename)
{
	std::string filepath = R"(CompiledShaders\)" + filename + R"(.cso)";
	std::ifstream fileHandle(filepath, std::ios::binary);
	assert(fileHandle.good() && L"Error opening file");

	// file size
	fileHandle.seekg(0, std::ios::end);
	std::ifstream::pos_type size = fileHandle.tellg();
	fileHandle.seekg(0, std::ios::beg);

	// serialize bytecode
	ID3DBlob* blob;
	CHECK(D3DCreateBlob(size, &blob));
	fileHandle.read(static_cast<char*>(blob->GetBufferPointer()), size);

	fileHandle.close();

	return blob;
}

ID3D12RootSignature* CreateRootSignature(ID3D12Device* device, const D3D12_ROOT_SIGNATURE_DESC& desc)
{
	Microsoft::WRL::ComPtr<ID3DBlob> rootSigBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

	HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, rootSigBlob.GetAddressOf(), errorBlob.GetAddressOf());
	assert(hr == S_OK && static_cast<char*>(errorBlob->GetBufferPointer()));

	ID3D12RootSignature* rootSig;
	hr = device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSig));
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
			size += sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
			break;
		case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
			size += sizeof(DWORD);
			break;
		case D3D12_ROOT_PARAMETER_TYPE_CBV:
		case D3D12_ROOT_PARAMETER_TYPE_SRV:
		case D3D12_ROOT_PARAMETER_TYPE_UAV:
			size += sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
			break;
		}
	}

	return size;
}

D3D12_ROOT_SIGNATURE_DESC RaytraceMaterialPipeline::BuildRaygenRootSignature(RenderPass::Id pass)
{
	D3D12_ROOT_SIGNATURE_DESC rootDesc = {};

	if (pass == RenderPass::Raytrace)
	{
		std::vector<D3D12_ROOT_PARAMETER> rootParams;
		rootParams.reserve(2);

		// Constant buffers
		D3D12_ROOT_PARAMETER viewCBV{};
		viewCBV.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		viewCBV.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		viewCBV.Descriptor.ShaderRegister = 0;
		rootParams.push_back(viewCBV);

		// UAV
		D3D12_ROOT_PARAMETER outputUAV{};
		outputUAV.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
		outputUAV.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		outputUAV.Descriptor.ShaderRegister = 0;
		rootParams.push_back(outputUAV);

		// Full Root Signature
		rootDesc.NumParameters = rootParams.size();
		rootDesc.pParameters = rootParams.data();
		rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
	}

	return rootDesc;
}

RaytraceMaterialPipeline::RaytraceMaterialPipeline(ID3D12Device5* device, RenderPass::Id pass)
{
	// RayGen shader
	Microsoft::WRL::ComPtr<ID3DBlob> rgsByteCode = LoadBlob(k_rgs);
	std::wstring rgsExportName(k_rgs.begin(), k_rgs.end());

	D3D12_EXPORT_DESC rgsExportDesc{};
	rgsExportDesc.Name = L"RayGenDefaultPass";
	rgsExportDesc.ExportToRename = L"RayGen";

	D3D12_DXIL_LIBRARY_DESC rgsLibDesc{};
	rgsLibDesc.DXILLibrary.pShaderBytecode = rgsByteCode.Get()->GetBufferPointer();
	rgsLibDesc.DXILLibrary.BytecodeLength = rgsByteCode.Get()->GetBufferSize();
	rgsLibDesc.NumExports = 1;
	rgsLibDesc.pExports = &rgsExportDesc;

	D3D12_STATE_SUBOBJECT rgsSubObject{};
	rgsSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	rgsSubObject.pDesc = &rgsLibDesc;
	m_pipelineSubObjects.push_back(rgsSubObject);


	// Payload
	D3D12_RAYTRACING_SHADER_CONFIG shaderConfigDesc{};
	shaderConfigDesc.MaxPayloadSizeInBytes = sizeof(DirectX::XMFLOAT4);
	shaderConfigDesc.MaxAttributeSizeInBytes = D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES;

	D3D12_STATE_SUBOBJECT shaderConfigSubObject{};
	shaderConfigSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
	shaderConfigSubObject.pDesc = &shaderConfigDesc;
	m_pipelineSubObjects.push_back(shaderConfigSubObject);

	m_pPayloadSubobject = &m_pipelineSubObjects.back();


	// RGS - Payload Association
	const wchar_t* shaderExports[] = { rgsExportDesc.Name };

	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION shaderConfigAssociationDesc{};
	shaderConfigAssociationDesc.pExports = shaderExports;
	shaderConfigAssociationDesc.NumExports = std::extent<decltype(shaderExports)>::value;
	shaderConfigAssociationDesc.pSubobjectToAssociate = m_pPayloadSubobject;

	D3D12_STATE_SUBOBJECT shaderConfigAssociationSubObject{};
	shaderConfigAssociationSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	shaderConfigAssociationSubObject.pDesc = &shaderConfigAssociationDesc;
	m_pipelineSubObjects.push_back(shaderConfigAssociationSubObject);

	// Raygen Root Signature
	D3D12_ROOT_SIGNATURE_DESC rootSigDesc = BuildRaygenRootSignature(pass);
	assert(GetRootSignatureSizeInBytes(rootSigDesc) <= k_maxRootSignatureSize);
	m_raytraceRootSignature = CreateRootSignature(device, rootSigDesc);

	D3D12_STATE_SUBOBJECT raygenRootSigSubObject{};
	raygenRootSigSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	raygenRootSigSubObject.pDesc = &rootSig;
	m_pipelineSubObjects.push_back(raygenRootSigSubObject);


	// Raygen - Root Signature Association
	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION sharedRootSignatureAssociationDesc{};
	sharedRootSignatureAssociationDesc.pExports = shaderExports;
	sharedRootSignatureAssociationDesc.NumExports = std::extent<decltype(shaderExports)>::value;
	sharedRootSignatureAssociationDesc.pSubobjectToAssociate = &m_pipelineSubObjects.back();

	D3D12_STATE_SUBOBJECT sharedRootSignatureAssociationSubObject{};
	sharedRootSignatureAssociationSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	sharedRootSignatureAssociationSubObject.pDesc = &sharedRootSignatureAssociationDesc;
	m_pipelineSubObjects.push_back(sharedRootSignatureAssociationSubObject);


	// Pipeline Config
	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfigDesc{};
	pipelineConfigDesc.MaxTraceRecursionDepth = 1;

	D3D12_STATE_SUBOBJECT pipelineConfigSubObject{};
	pipelineConfigSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
	pipelineConfigSubObject.pDesc = &pipelineConfigDesc;
	m_pipelineSubObjects.push_back(pipelineConfigSubObject);
}

void RaytraceMaterialPipeline::BuildFromMaterial(ID3D12Device5* device, std::wstring materialName, MaterialRtPipelineDesc pipelineDesc, std::vector<IUnknown*>& pendingResources)
{
	// Closest Hit Shader
	ID3DBlob* chsByteCode = LoadBlob(pipelineDesc.closestHitShader);
	pendingResources.push_back(chsByteCode);

	D3D12_EXPORT_DESC chsExportDesc{};
	chsExportDesc.Name = (L"ClosestHit" + materialName).c_str();
	chsExportDesc.ExportToRename = L"ClosestHit";

	D3D12_DXIL_LIBRARY_DESC chsLibDesc{};
	chsLibDesc.DXILLibrary.pShaderBytecode = chsByteCode->GetBufferPointer();
	chsLibDesc.DXILLibrary.BytecodeLength = chsByteCode->GetBufferSize();
	chsLibDesc.NumExports = 1;
	chsLibDesc.pExports = &chsExportDesc;

	D3D12_STATE_SUBOBJECT chsSubObject{};
	chsSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	chsSubObject.pDesc = &chsLibDesc;
	m_pipelineSubObjects.push_back(chsSubObject);


	// Miss Shader
	ID3DBlob* msByteCode = LoadBlob(pipelineDesc.missShader);
	pendingResources.push_back(msByteCode);

	D3D12_EXPORT_DESC msExportDesc{};
	msExportDesc.Name = (L"Miss" + materialName).c_str();
	msExportDesc.ExportToRename = L"Miss";

	D3D12_DXIL_LIBRARY_DESC msLibDesc{};
	msLibDesc.DXILLibrary.pShaderBytecode = msByteCode->GetBufferPointer();
	msLibDesc.DXILLibrary.BytecodeLength = msByteCode->GetBufferSize();
	msLibDesc.NumExports = 1;
	msLibDesc.pExports = &msExportDesc;

	D3D12_STATE_SUBOBJECT msSubObject{};
	msSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	msSubObject.pDesc = &msLibDesc;
	m_pipelineSubObjects.push_back(msSubObject);


	// Hit Group
	D3D12_HIT_GROUP_DESC hitGroupDesc{};
	hitGroupDesc.ClosestHitShaderImport = chsExportDesc.Name;
	hitGroupDesc.HitGroupExport = (L"HitGroup" + materialName).c_str();

	D3D12_STATE_SUBOBJECT hitGroupSubObject{};
	hitGroupSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
	hitGroupSubObject.pDesc = &hitGroupDesc;
	m_pipelineSubObjects.push_back(hitGroupSubObject);


	// HitGroup/Miss - Payload Association
	const wchar_t* shaderExports[] = {
		msExportDesc.Name,
		hitGroupDesc.HitGroupExport
	};

	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION shaderConfigAssociationDesc{};
	shaderConfigAssociationDesc.pExports = shaderExports;
	shaderConfigAssociationDesc.NumExports = std::extent<decltype(shaderExports)>::value;
	shaderConfigAssociationDesc.pSubobjectToAssociate = m_pPayloadSubobject;

	D3D12_STATE_SUBOBJECT shaderConfigAssociationSubObject{};
	shaderConfigAssociationSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	shaderConfigAssociationSubObject.pDesc = &shaderConfigAssociationDesc;
	m_pipelineSubObjects.push_back(shaderConfigAssociationSubObject);


	// Root Signature
	assert(GetRootSignatureSizeInBytes(device, pipelineDesc.rootsigDesc) <= k_maxRootSignatureSize);
	ID3D12RootSignature* rootSig = CreateRootSignature(device, pipelineDesc.rootsigDesc);
	pendingResources.push_back(rootSig);

	D3D12_STATE_SUBOBJECT sharedRootSigSubObject{};
	sharedRootSigSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	sharedRootSigSubObject.pDesc = &rootSig;
	m_pipelineSubObjects.push_back(sharedRootSigSubObject);


	// HitGroup/Miss - Root Signature Association
	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION sharedRootSignatureAssociationDesc{};
	sharedRootSignatureAssociationDesc.pExports = shaderExports;
	sharedRootSignatureAssociationDesc.NumExports = std::extent<decltype(shaderExports)>::value;
	sharedRootSignatureAssociationDesc.pSubobjectToAssociate = &m_pipelineSubObjects.back();

	D3D12_STATE_SUBOBJECT sharedRootSignatureAssociationSubObject{};
	sharedRootSignatureAssociationSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	sharedRootSignatureAssociationSubObject.pDesc = &sharedRootSignatureAssociationDesc;
	m_pipelineSubObjects.push_back(sharedRootSignatureAssociationSubObject);
}

void RaytraceMaterialPipeline::Commit(ID3D12Device5* device, std::vector<IUnknown*>& pendingResources)
{
	// Create the PSO
	D3D12_STATE_OBJECT_DESC psoDesc;
	psoDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
	psoDesc.NumSubobjects = m_pipelineSubObjects.size();
	psoDesc.pSubobjects = m_pipelineSubObjects.data();

	CHECK(device->CreateStateObject(
		&psoDesc,
		IID_PPV_ARGS(m_pso.GetAddressOf())
	));

	// Get the PSO properties
	CHECK(m_pso->QueryInterface(IID_PPV_ARGS(m_psoProperties.GetAddressOf())));

	// Release the pending resources
	for (auto resource : pendingResources)
	{
		resource->Release();
	}
}

void RaytraceMaterialPipeline::Bind(ID3D12GraphicsCommandList4* cmdList, uint8_t* pData, RenderPass::Id pass, D3D12_GPU_VIRTUAL_ADDRESS viewConstants, D3D12_GPU_DESCRIPTOR_HANDLE outputUAV) const
{
	// Bind PSO
	cmdList->SetPipelineState1(m_pso.Get());

	// Raygen shader 
	memcpy(pData, GetShaderIdentifier(k_rgs), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	// Raygen shader bindings
	if (pass == RenderPass::Raytrace)
	{
		D3D12_GPU_VIRTUAL_ADDRESS* pRootDescriptors = *reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		*(pRootDescriptors++) = viewConstants;
		*(pRootDescriptors++) = outputUAV;
	}


}

size_t RaytraceMaterialPipeline::GetRootSignatureSize() const
{
	return m_raytraceRootSignatureSize;
}

void* RaytraceMaterialPipeline::GetShaderIdentifier(const wchar_t* exportName) const
{
	return m_psoProperties->GetShaderIdentifier(exportName);
}

Material::Material(const std::string& name) :
	m_name { std::move(name) }
{
}

DefaultOpaqueMaterial::DefaultOpaqueMaterial(std::string& name, const D3D12_GPU_DESCRIPTOR_HANDLE srvHandle) :
	Material{ name },
	m_srvBegin{ srvHandle }
{
}

MaterialRtPipelineDesc DefaultOpaqueMaterial::GetRaytracePipelineDesc(RenderPass::Id renderPass)
{
	auto rootSigDesc = BuildRaytraceRootSignature(renderPass);
	return MaterialRtPipelineDesc{ k_chs, k_ms, rootSigDesc };
}

D3D12_ROOT_SIGNATURE_DESC DefaultOpaqueMaterial::BuildRaytraceRootSignature(RenderPass::Id pass)
{
	D3D12_ROOT_SIGNATURE_DESC rootDesc = {};

	if (pass == RenderPass::Raytrace)
	{
		std::vector<D3D12_ROOT_PARAMETER> rootParams;
		rootParams.reserve(6);

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

		// UAV
		D3D12_ROOT_PARAMETER outputUAV{};
		outputUAV.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
		outputUAV.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		outputUAV.Descriptor.ShaderRegister = 0;
		rootParams.push_back(outputUAV);

		// TLAS SRV
		D3D12_ROOT_PARAMETER tlasSRV{};
		tlasSRV.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
		tlasSRV.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		tlasSRV.Descriptor.RegisterSpace = 0;
		tlasSRV.Descriptor.ShaderRegister = 0;
		rootParams.push_back(tlasSRV);

		// Meshdata SRVs
		D3D12_DESCRIPTOR_RANGE meshSRVRange{};
		meshSRVRange.BaseShaderRegister = 1;
		meshSRVRange.NumDescriptors = 2; // VB, IB
		meshSRVRange.RegisterSpace = 0;
		meshSRVRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		meshSRVRange.OffsetInDescriptorsFromTableStart = 0;

		D3D12_ROOT_PARAMETER meshSRVs{};
		meshSRVs.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		meshSRVs.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		meshSRVs.DescriptorTable.NumDescriptorRanges = 1;
		meshSRVs.DescriptorTable.pDescriptorRanges = &meshSRVRange;
		rootParams.push_back(meshSRVs);

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

		// Full Root Signature
		rootDesc.NumParameters = rootParams.size();
		rootDesc.pParameters = rootParams.data();
		rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
	}

	return rootDesc;
}

void DefaultOpaqueMaterial::BindConstants(
	uint8_t* pData, 
	RaytraceMaterialPipeline* pipeline, 
	D3D12_GPU_VIRTUAL_ADDRESS tlas, 
	D3D12_GPU_DESCRIPTOR_HANDLE meshBuffer, 
	D3D12_GPU_VIRTUAL_ADDRESS objConstants, 
	D3D12_GPU_VIRTUAL_ADDRESS viewConstants, 
	D3D12_GPU_VIRTUAL_ADDRESS lightConstants,
	D3D12_GPU_DESCRIPTOR_HANDLE outputUAV
) const
{
	// Entry 1 - Miss Program
	{
		memcpy(pData, pipeline->GetShaderIdentifier(k_ms), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		D3D12_GPU_VIRTUAL_ADDRESS* pRootDescriptors = reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		*(pRootDescriptors++) = viewConstants;
		*(pRootDescriptors++) = lightConstants;
		*(pRootDescriptors++) = outputUAV;
		*(pRootDescriptors++) = tlas;

		D3D12_GPU_DESCRIPTOR_HANDLE* pDescriptorTables = *reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE>(pRootDescriptors);
		(*pDescriptorTables++) = meshBuffer;
		(*pDescriptorTables++) = m_srvBegin;
	}

	pData += k_sbtEntrySize;

	// Entry 2 - Closest Hit Program / Hit Group
	{
		memcpy(pData, pipeline->GetShaderIdentifier(L"HitGroup"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		D3D12_GPU_VIRTUAL_ADDRESS* pRootDescriptors = *reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		*(pRootDescriptors++) = viewConstants;
		*(pRootDescriptors++) = lightConstants;
		*(pRootDescriptors++) = outputUAV;
		*(pRootDescriptors++) = tlas;

		D3D12_GPU_DESCRIPTOR_HANDLE* pDescriptorTables = *reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE>(pRootDescriptors);
		(*pDescriptorTables++) = meshBuffer;
		(*pDescriptorTables++) = m_srvBegin;
	}
}

DefaultMaskedMaterial::DefaultMaskedMaterial(std::string& name, const D3D12_GPU_DESCRIPTOR_HANDLE srvHandle, const D3D12_GPU_DESCRIPTOR_HANDLE opacityMaskSrvHandle) :
	Material{ name },
	m_srvBegin{ srvHandle },
	m_opacityMaskSrv{opacityMaskSrvHandle}
{
}

MaterialRtPipelineDesc DefaultMaskedMaterial::GetRaytracePipelineDesc(RenderPass::Id pass)
{
	auto rootSigDesc = BuildRaytraceRootSignature( pass);
	return MaterialRtPipelineDesc{ k_chs, k_ms, rootSigDesc };
}

D3D12_ROOT_SIGNATURE_DESC DefaultMaskedMaterial::BuildRaytraceRootSignature(RenderPass::Id pass)
{
	D3D12_ROOT_SIGNATURE_DESC rootDesc = {};

	if (pass == RenderPass::Raytrace)
	{
		std::vector<D3D12_ROOT_PARAMETER> rootParams;
		rootParams.reserve(6);

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

		// UAV
		D3D12_ROOT_PARAMETER outputUAV{};
		outputUAV.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
		outputUAV.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		outputUAV.Descriptor.ShaderRegister = 0;
		rootParams.push_back(outputUAV);

		// TLAS SRV
		D3D12_ROOT_PARAMETER tlasSRV{};
		tlasSRV.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
		tlasSRV.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		tlasSRV.Descriptor.RegisterSpace = 0;
		tlasSRV.Descriptor.ShaderRegister = 0;
		rootParams.push_back(tlasSRV);

		// Mesh SRVs
		D3D12_DESCRIPTOR_RANGE meshSRVRange{};
		meshSRVRange.BaseShaderRegister = 1;
		meshSRVRange.NumDescriptors = 2; // VB, IB
		meshSRVRange.RegisterSpace = 0;
		meshSRVRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		meshSRVRange.OffsetInDescriptorsFromTableStart = 0;

		D3D12_ROOT_PARAMETER meshSRVs{};
		meshSRVs.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		meshSRVs.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		meshSRVs.DescriptorTable.NumDescriptorRanges = 1;
		meshSRVs.DescriptorTable.pDescriptorRanges = &meshSRVRange;
		rootParams.push_back(meshSRVs);

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

		// Full Root Signature
		rootDesc.NumParameters = rootParams.size();
		rootDesc.pParameters = rootParams.data();
		rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
	}

	return rootDesc;
}

void DefaultMaskedMaterial::BindConstants(
	uint8_t* pData, 
	RaytraceMaterialPipeline* pipeline,
	D3D12_GPU_VIRTUAL_ADDRESS tlas,
	D3D12_GPU_DESCRIPTOR_HANDLE meshBuffer,
	D3D12_GPU_VIRTUAL_ADDRESS objConstants,
	D3D12_GPU_VIRTUAL_ADDRESS viewConstants,
	D3D12_GPU_VIRTUAL_ADDRESS lightConstants,
	D3D12_GPU_DESCRIPTOR_HANDLE outputUAV
) const
{
	// Entry 1 - Miss Program
	{
		memcpy(pData, pipeline->GetShaderIdentifier(k_ms), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		D3D12_GPU_VIRTUAL_ADDRESS* pRootDescriptors = reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		*(pRootDescriptors++) = viewConstants;
		*(pRootDescriptors++) = lightConstants;
		*(pRootDescriptors++) = outputUAV.ptr;
		*(pRootDescriptors++) = tlas;

		D3D12_GPU_DESCRIPTOR_HANDLE* pDescriptorTables = *reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE>(pRootDescriptors);
		(*pDescriptorTables++) = meshBuffer;
		(*pDescriptorTables++) = m_srvBegin;
	}

	pData += k_sbtEntrySize;

	// Entry 2 - Closest Hit Program / Hit Group
	{
		memcpy(pData, pipeline->GetShaderIdentifier(L"HitGroup"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		D3D12_GPU_VIRTUAL_ADDRESS* pRootDescriptors = *reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		*(pRootDescriptors++) = viewConstants;
		*(pRootDescriptors++) = lightConstants;
		*(pRootDescriptors++) = outputUAV;
		*(pRootDescriptors++) = tlas;

		D3D12_GPU_DESCRIPTOR_HANDLE* pDescriptorTables = *reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE>(pRootDescriptors);
		(*pDescriptorTables++) = meshBuffer;
		(*pDescriptorTables++) = m_srvBegin;
	}
}

UntexturedMaterial::UntexturedMaterial(std::string& name, Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer) :
	Material{ name },
	m_constantBuffer{ constantBuffer }
{
}

MaterialRtPipelineDesc UntexturedMaterial::GetRaytracePipelineDesc(RenderPass::Id pass)
{
	auto rootSigDesc = BuildRaytraceRootSignature(pass);
	return MaterialRtPipelineDesc{ k_chs, k_ms, rootSigDesc };
}

D3D12_ROOT_SIGNATURE_DESC UntexturedMaterial::BuildRaytraceRootSignature(RenderPass::Id pass)
{
	D3D12_ROOT_SIGNATURE_DESC rootDesc = {};

	if (pass == RenderPass::Raytrace)
	{
		std::vector<D3D12_ROOT_PARAMETER> rootParams;
		rootParams.reserve(7);

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

		// TLAS SRV
		D3D12_ROOT_PARAMETER tlasSRV{};
		tlasSRV.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
		tlasSRV.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		tlasSRV.Descriptor.RegisterSpace = 0;
		tlasSRV.Descriptor.ShaderRegister = 0;
		rootParams.push_back(tlasSRV);

		// Mesh SRVs
		D3D12_DESCRIPTOR_RANGE meshSRVRange{};
		meshSRVRange.BaseShaderRegister = 1;
		meshSRVRange.NumDescriptors = 2; // VB, IB
		meshSRVRange.RegisterSpace = 0;
		meshSRVRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		meshSRVRange.OffsetInDescriptorsFromTableStart = 0;

		D3D12_ROOT_PARAMETER meshSRVs{};
		meshSRVs.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		meshSRVs.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		meshSRVs.DescriptorTable.NumDescriptorRanges = 1;
		meshSRVs.DescriptorTable.pDescriptorRanges = &meshSRVRange;
		rootParams.push_back(meshSRVs);

		// Full root signature
		rootDesc.NumParameters = rootParams.size();
		rootDesc.pParameters = rootParams.data();
		rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
	}

	return rootDesc;
}

void UntexturedMaterial::BindConstants(
	uint8_t* pData,
	RaytraceMaterialPipeline* pipeline,
	D3D12_GPU_VIRTUAL_ADDRESS tlas,
	D3D12_GPU_DESCRIPTOR_HANDLE meshBuffer,
	D3D12_GPU_VIRTUAL_ADDRESS objConstants,
	D3D12_GPU_VIRTUAL_ADDRESS viewConstants,
	D3D12_GPU_VIRTUAL_ADDRESS lightConstants,
	D3D12_GPU_DESCRIPTOR_HANDLE outputUAV
) const
{
	// Entry 1 - Miss Program
	{
		memcpy(pData, pipeline->GetShaderIdentifier(k_ms), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		D3D12_GPU_VIRTUAL_ADDRESS* pRootDescriptors = reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		*(pRootDescriptors++) = viewConstants;
		*(pRootDescriptors++) = lightConstants;
		*(pRootDescriptor++) = m_constantBuffer->GetGPUVirtualAddress();
		*(pRootDescriptors++) = outputUAV;
		*(pRootDescriptors++) = tlas;

		D3D12_GPU_DESCRIPTOR_HANDLE* pDescriptorTables = *reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE>(pRootDescriptors);
		(*pDescriptorTables++) = meshBuffer;
	}

	pData += k_sbtEntrySize;

	// Entry 2 - Closest Hit Program / Hit Group
	{
		memcpy(pData, pipeline->GetShaderIdentifier(L"HitGroup"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		D3D12_GPU_VIRTUAL_ADDRESS* pRootDescriptors = *reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		*(pRootDescriptors++) = viewConstants;
		*(pRootDescriptors++) = lightConstants;
		*(pRootDescriptor++) = m_constantBuffer->GetGPUVirtualAddress();
		*(pRootDescriptors++) = outputUAV;
		*(pRootDescriptors++) = tlas;

		D3D12_GPU_DESCRIPTOR_HANDLE* pDescriptorTables = *reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE>(pRootDescriptors);
		(*pDescriptorTables++) = meshBuffer;
	}
}