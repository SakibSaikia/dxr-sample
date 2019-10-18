#include "stdafx.h"
#include "Material.h"

namespace
{
	static const wchar_t* k_rgs = L"raygen.rgs";
}

Microsoft::WRL::ComPtr<ID3DBlob> LoadBlob(const std::wstring& filename)
{
	std::wstring filepath = L"CompiledShaders\\" + filename + L".cso";
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
	assert(hr == S_OK || static_cast<char*>(errorBlob->GetBufferPointer()));

	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSig;
	hr = device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(rootSig.GetAddressOf()));
	assert(hr == S_OK && L"Failed to create root signature!");

	return rootSig;
}

size_t GetRootSignatureSizeInBytes(const D3D12_ROOT_SIGNATURE_DESC& desc)
{
	size_t size{ 0 };
	for (auto i = 0u; i < desc.NumParameters; i++)
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

Microsoft::WRL::ComPtr<ID3D12RootSignature> RaytraceMaterialPipeline::GetRaygenRootSignature(ID3D12Device5* device)
{
	std::vector<D3D12_ROOT_PARAMETER> rootParams;
	rootParams.reserve(3);

	// Constant buffers
	D3D12_ROOT_PARAMETER viewCBV{};
	viewCBV.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	viewCBV.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	viewCBV.Descriptor.ShaderRegister = 0;
	rootParams.push_back(viewCBV);

	// TLAS SRV
	D3D12_ROOT_PARAMETER tlasSRV{};
	tlasSRV.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	tlasSRV.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	tlasSRV.Descriptor.RegisterSpace = 0;
	tlasSRV.Descriptor.ShaderRegister = 0;
	rootParams.push_back(tlasSRV);

	// UAV
	D3D12_ROOT_PARAMETER outputUAV{};
	outputUAV.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
	outputUAV.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	outputUAV.Descriptor.ShaderRegister = 0;
	rootParams.push_back(outputUAV);

	D3D12_ROOT_SIGNATURE_DESC rootDesc = {};
	rootDesc.NumParameters = static_cast<UINT>(rootParams.size());
	rootDesc.pParameters = rootParams.data();
	rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
	assert(GetRootSignatureSizeInBytes(rootDesc) <= k_maxRootSignatureSize);

	return CreateRootSignature(device, rootDesc);
}

RaytraceMaterialPipeline::RaytraceMaterialPipeline(ID3D12Device5* device)
{
	// This is important as we don't want reallocations since subobjects can refer to other subobjects by ptr
	m_pipelineSubObjects.reserve(k_maxRtPipelineSubobjectCount);

	// RayGen shader
	Microsoft::WRL::ComPtr<ID3DBlob> rgsByteCode = LoadBlob(k_rgs);

	D3D12_EXPORT_DESC rgsExportDesc{};
	rgsExportDesc.Name = L"RayGenDefaultPass";
	rgsExportDesc.ExportToRename = L"RayGen";

	D3D12_DXIL_LIBRARY_DESC rgsLibDesc{};
	rgsLibDesc.DXILLibrary.pShaderBytecode = rgsByteCode->GetBufferPointer();
	rgsLibDesc.DXILLibrary.BytecodeLength = rgsByteCode->GetBufferSize();
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
	m_raygenRootSignature = GetRaygenRootSignature(device);

	D3D12_STATE_SUBOBJECT raygenRootSigSubObject{};
	raygenRootSigSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	raygenRootSigSubObject.pDesc = m_raygenRootSignature.Get();
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

void RaytraceMaterialPipeline::BuildFromMaterial(
	ID3D12Device5* device, 
	std::wstring materialName, 
	RtMaterialPipeline material
)
{
	m_materials.push_back(material);

	// Closest Hit Shader
	D3D12_EXPORT_DESC chsExportDesc{};
	chsExportDesc.Name = (L"ClosestHit" + materialName).c_str();
	chsExportDesc.ExportToRename = L"ClosestHit";

	D3D12_DXIL_LIBRARY_DESC chsLibDesc{};
	chsLibDesc.DXILLibrary.pShaderBytecode = material.closestHitShader->GetBufferPointer();
	chsLibDesc.DXILLibrary.BytecodeLength = material.closestHitShader->GetBufferSize();
	chsLibDesc.NumExports = 1;
	chsLibDesc.pExports = &chsExportDesc;

	D3D12_STATE_SUBOBJECT chsSubObject{};
	chsSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	chsSubObject.pDesc = &chsLibDesc;
	m_pipelineSubObjects.push_back(chsSubObject);


	// Miss Shader
	D3D12_EXPORT_DESC msExportDesc{};
	msExportDesc.Name = (L"Miss" + materialName).c_str();
	msExportDesc.ExportToRename = L"Miss";

	D3D12_DXIL_LIBRARY_DESC msLibDesc{};
	msLibDesc.DXILLibrary.pShaderBytecode = material.missShader->GetBufferPointer();
	msLibDesc.DXILLibrary.BytecodeLength = material.missShader->GetBufferSize();
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
	D3D12_STATE_SUBOBJECT sharedRootSigSubObject{};
	sharedRootSigSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	sharedRootSigSubObject.pDesc = material.rootSignature.Get();
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

void RaytraceMaterialPipeline::Commit(ID3D12Device5* device)
{
	assert(m_pipelineSubObjects.size() < k_maxRtPipelineSubobjectCount);

	// Create the PSO
	D3D12_STATE_OBJECT_DESC psoDesc;
	psoDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
	psoDesc.NumSubobjects = static_cast<UINT>(m_pipelineSubObjects.size());
	psoDesc.pSubobjects = m_pipelineSubObjects.data();

	CHECK(device->CreateStateObject(
		&psoDesc,
		IID_PPV_ARGS(m_pso.GetAddressOf())
	));

	// Get the PSO properties
	CHECK(m_pso->QueryInterface(IID_PPV_ARGS(m_psoProperties.GetAddressOf())));

	// Release the pending resources
	for (auto material : m_materials)
	{
		material.closestHitShader.Reset();
		material.missShader.Reset();
		material.rootSignature.Reset();
	}

	m_pipelineSubObjects.clear();
}

void RaytraceMaterialPipeline::Bind(
	ID3D12GraphicsCommandList4* cmdList,
	uint8_t* pData, 
	D3D12_GPU_VIRTUAL_ADDRESS viewCBV,
	D3D12_GPU_DESCRIPTOR_HANDLE tlasSRV,
	D3D12_GPU_DESCRIPTOR_HANDLE outputUAV
) const
{
	// Bind PSO
	cmdList->SetPipelineState1(m_pso.Get());

	// Raygen shader 
	memcpy(pData, GetShaderIdentifier(k_rgs), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	// Raygen shader bindings
	auto* pRootDescriptors = reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	*(pRootDescriptors++) = viewCBV;
	*(pRootDescriptors++) = tlasSRV.ptr;
	*(pRootDescriptors++) = outputUAV.ptr;
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

RtMaterialPipeline DefaultOpaqueMaterial::GetRaytraceMaterialPipeline(ID3D12Device5* device)
{
	return RtMaterialPipeline{ LoadBlob(k_chs), LoadBlob(k_ms), GetRaytraceRootSignature(device) };
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> DefaultOpaqueMaterial::GetRaytraceRootSignature(ID3D12Device5* device)
{
	std::vector<D3D12_ROOT_PARAMETER> rootParams;
	rootParams.reserve(7);

	// Constant buffers
	D3D12_ROOT_PARAMETER objectCBV{};
	objectCBV.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	objectCBV.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	objectCBV.Descriptor.ShaderRegister = 0;
	rootParams.push_back(objectCBV);

	D3D12_ROOT_PARAMETER viewCBV{};
	viewCBV.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	viewCBV.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	viewCBV.Descriptor.ShaderRegister = 1;
	rootParams.push_back(viewCBV);

	D3D12_ROOT_PARAMETER lightCBV{};
	lightCBV.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	lightCBV.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	lightCBV.Descriptor.ShaderRegister = 2;
	rootParams.push_back(lightCBV);

	// Meshdata SRVs
	D3D12_DESCRIPTOR_RANGE meshSRVRange{};
	meshSRVRange.BaseShaderRegister = 0;
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

	D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
	rootSigDesc.NumParameters = static_cast<UINT>(rootParams.size());
	rootSigDesc.pParameters = rootParams.data();
	rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
	assert(GetRootSignatureSizeInBytes(rootSigDesc) <= k_maxRootSignatureSize);

	return CreateRootSignature(device, rootSigDesc);
}

void DefaultOpaqueMaterial::BindConstants(
	uint8_t* pData, 
	const RaytraceMaterialPipeline* pipeline,
	D3D12_GPU_DESCRIPTOR_HANDLE meshBuffer, 
	D3D12_GPU_VIRTUAL_ADDRESS objConstants, 
	D3D12_GPU_VIRTUAL_ADDRESS viewConstants, 
	D3D12_GPU_VIRTUAL_ADDRESS lightConstants
) const
{
	// Entry 1 - Miss Program
	{
		memcpy(pData, pipeline->GetShaderIdentifier(k_ms), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		auto* pRootDescriptors = reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		*(pRootDescriptors++) = objConstants;
		*(pRootDescriptors++) = viewConstants;
		*(pRootDescriptors++) = lightConstants;

		auto* pDescriptorTables = reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(pRootDescriptors);
		(*pDescriptorTables++) = meshBuffer;
		(*pDescriptorTables++) = m_srvBegin;
	}

	pData += k_shaderRecordSize;

	// Entry 2 - Closest Hit Program / Hit Group
	{
		memcpy(pData, pipeline->GetShaderIdentifier(L"HitGroup"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		auto* pRootDescriptors = reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		*(pRootDescriptors++) = viewConstants;
		*(pRootDescriptors++) = lightConstants;

		auto* pDescriptorTables = reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(pRootDescriptors);
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

RtMaterialPipeline DefaultMaskedMaterial::GetRaytraceMaterialPipeline(ID3D12Device5* device)
{
	return RtMaterialPipeline{ LoadBlob(k_chs), LoadBlob(k_ms), GetRaytraceRootSignature(device) };
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> DefaultMaskedMaterial::GetRaytraceRootSignature(ID3D12Device5* device)
{
	std::vector<D3D12_ROOT_PARAMETER> rootParams;
	rootParams.reserve(5);

	// Constant buffers
	D3D12_ROOT_PARAMETER objectCBV{};
	objectCBV.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	objectCBV.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	objectCBV.Descriptor.ShaderRegister = 0;
	rootParams.push_back(objectCBV);

	D3D12_ROOT_PARAMETER viewCBV{};
	viewCBV.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	viewCBV.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	viewCBV.Descriptor.ShaderRegister = 1;
	rootParams.push_back(viewCBV);

	D3D12_ROOT_PARAMETER lightCBV{};
	lightCBV.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	lightCBV.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	lightCBV.Descriptor.ShaderRegister = 2;
	rootParams.push_back(lightCBV);

	// Mesh SRVs
	D3D12_DESCRIPTOR_RANGE meshSRVRange{};
	meshSRVRange.BaseShaderRegister = 0;
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

	D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
	rootSigDesc.NumParameters = static_cast<UINT>(rootParams.size());
	rootSigDesc.pParameters = rootParams.data();
	rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
	assert(GetRootSignatureSizeInBytes(rootSigDesc) <= k_maxRootSignatureSize);

	return CreateRootSignature(device, rootSigDesc);
}

void DefaultMaskedMaterial::BindConstants(
	uint8_t* pData, 
	const RaytraceMaterialPipeline* pipeline,
	D3D12_GPU_DESCRIPTOR_HANDLE meshBuffer,
	D3D12_GPU_VIRTUAL_ADDRESS objConstants,
	D3D12_GPU_VIRTUAL_ADDRESS viewConstants,
	D3D12_GPU_VIRTUAL_ADDRESS lightConstants
) const
{
	// Entry 1 - Miss Program
	{
		memcpy(pData, pipeline->GetShaderIdentifier(k_ms), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		auto* pRootDescriptors = reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		*(pRootDescriptors++) = objConstants;
		*(pRootDescriptors++) = viewConstants;
		*(pRootDescriptors++) = lightConstants;

		auto* pDescriptorTables = reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(pRootDescriptors);
		(*pDescriptorTables++) = meshBuffer;
		(*pDescriptorTables++) = m_srvBegin;
	}

	pData += k_shaderRecordSize;

	// Entry 2 - Closest Hit Program / Hit Group
	{
		memcpy(pData, pipeline->GetShaderIdentifier(L"HitGroup"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		auto* pRootDescriptors = reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		*(pRootDescriptors++) = viewConstants;
		*(pRootDescriptors++) = lightConstants;

		auto* pDescriptorTables = reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(pRootDescriptors);
		(*pDescriptorTables++) = meshBuffer;
		(*pDescriptorTables++) = m_srvBegin;
	}
}

UntexturedMaterial::UntexturedMaterial(std::string& name, Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer) :
	Material{ name },
	m_constantBuffer{ constantBuffer }
{
}

RtMaterialPipeline UntexturedMaterial::GetRaytraceMaterialPipeline(ID3D12Device5* device)
{
	return RtMaterialPipeline{ LoadBlob(k_chs), LoadBlob(k_ms), GetRaytraceRootSignature(device) };
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> UntexturedMaterial::GetRaytraceRootSignature(ID3D12Device5* device)
{
	std::vector<D3D12_ROOT_PARAMETER> rootParams;
	rootParams.reserve(5);

	// Constant buffers
	D3D12_ROOT_PARAMETER objectCBV{};
	objectCBV.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	objectCBV.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	objectCBV.Descriptor.ShaderRegister = 0;
	rootParams.push_back(objectCBV);

	D3D12_ROOT_PARAMETER viewCBV{};
	viewCBV.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	viewCBV.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	viewCBV.Descriptor.ShaderRegister = 1;
	rootParams.push_back(viewCBV);

	D3D12_ROOT_PARAMETER lightCBV{};
	lightCBV.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	lightCBV.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	lightCBV.Descriptor.ShaderRegister = 2;
	rootParams.push_back(lightCBV);

	D3D12_ROOT_PARAMETER materialCBV{};
	materialCBV.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	materialCBV.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	materialCBV.Descriptor.ShaderRegister = 3;
	rootParams.push_back(materialCBV);

	// Mesh SRVs
	D3D12_DESCRIPTOR_RANGE meshSRVRange{};
	meshSRVRange.BaseShaderRegister = 0;
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

	D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
	rootSigDesc.NumParameters = static_cast<UINT>(rootParams.size());
	rootSigDesc.pParameters = rootParams.data();
	rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
	assert(GetRootSignatureSizeInBytes(rootSigDesc) <= k_maxRootSignatureSize);

	return CreateRootSignature(device, rootSigDesc);
}

void UntexturedMaterial::BindConstants(
	uint8_t* pData,
	const RaytraceMaterialPipeline* pipeline,
	D3D12_GPU_DESCRIPTOR_HANDLE meshBuffer,
	D3D12_GPU_VIRTUAL_ADDRESS objConstants,
	D3D12_GPU_VIRTUAL_ADDRESS viewConstants,
	D3D12_GPU_VIRTUAL_ADDRESS lightConstants
) const
{
	// Entry 1 - Miss Program
	{
		memcpy(pData, pipeline->GetShaderIdentifier(k_ms), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		auto* pRootDescriptors = reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		*(pRootDescriptors++) = objConstants;
		*(pRootDescriptors++) = viewConstants;
		*(pRootDescriptors++) = lightConstants;
		*(pRootDescriptors++) = m_constantBuffer->GetGPUVirtualAddress();

		auto* pDescriptorTables = reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(pRootDescriptors);
		(*pDescriptorTables++) = meshBuffer;
	}

	pData += k_shaderRecordSize;

	// Entry 2 - Closest Hit Program / Hit Group
	{
		memcpy(pData, pipeline->GetShaderIdentifier(L"HitGroup"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		auto* pRootDescriptors = reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		*(pRootDescriptors++) = viewConstants;
		*(pRootDescriptors++) = lightConstants;
		*(pRootDescriptors++) = m_constantBuffer->GetGPUVirtualAddress();

		auto* pDescriptorTables = reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(pRootDescriptors);
		(*pDescriptorTables++) = meshBuffer;
	}
}