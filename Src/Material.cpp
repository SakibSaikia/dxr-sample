#include "stdafx.h"
#include "Material.h"
#include "StackAllocator.h"

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

RaytraceMaterialPipeline::RaytraceMaterialPipeline(
	StackAllocator& stackAlloc, 
	std::vector<D3D12_STATE_SUBOBJECT>& subObjects,
	size_t& payloadIndex,
	ID3D12Device5* device)
{
	// This is important as we don't want reallocations since subobjects can refer to other subobjects by ptr
	subObjects.reserve(k_maxRtPipelineSubobjectCount);

	// RayGen shader
	auto rgsByteCode = stackAlloc.Allocate<Microsoft::WRL::ComPtr<ID3DBlob>>();
	*rgsByteCode = LoadBlob(k_rgs);

	auto rgsExportDesc = stackAlloc.Allocate<D3D12_EXPORT_DESC>(); rgsExportDesc;
	rgsExportDesc->Name = stackAlloc.ConstructName(L"RayGenDefaultPass");
	rgsExportDesc->ExportToRename = stackAlloc.ConstructName(L"RayGen");

	auto rgsLibDesc = stackAlloc.Allocate<D3D12_DXIL_LIBRARY_DESC>();
	rgsLibDesc->DXILLibrary.pShaderBytecode = (*rgsByteCode)->GetBufferPointer();
	rgsLibDesc->DXILLibrary.BytecodeLength = (*rgsByteCode)->GetBufferSize();
	rgsLibDesc->NumExports = 1;
	rgsLibDesc->pExports = rgsExportDesc;

	D3D12_STATE_SUBOBJECT rgsSubObject{};
	rgsSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	rgsSubObject.pDesc = rgsLibDesc;
	subObjects.push_back(rgsSubObject);


	// Payload
	auto shaderConfigDesc = stackAlloc.Allocate<D3D12_RAYTRACING_SHADER_CONFIG>();
	shaderConfigDesc->MaxPayloadSizeInBytes = sizeof(DirectX::XMFLOAT4);
	shaderConfigDesc->MaxAttributeSizeInBytes = D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES;

	D3D12_STATE_SUBOBJECT shaderConfigSubObject{};
	shaderConfigSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
	shaderConfigSubObject.pDesc = shaderConfigDesc;
	subObjects.push_back(shaderConfigSubObject);

	payloadIndex = subObjects.size();


	// RGS - Payload Association
	auto shaderExports = stackAlloc.Allocate<const wchar_t*>(1);
	shaderExports[0] = { rgsExportDesc->Name };

	auto shaderConfigAssociationDesc = stackAlloc.Allocate<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION>();
	shaderConfigAssociationDesc->pExports = shaderExports;
	shaderConfigAssociationDesc->NumExports = 1;
	shaderConfigAssociationDesc->pSubobjectToAssociate = &subObjects.back();

	D3D12_STATE_SUBOBJECT shaderConfigAssociationSubObject{};
	shaderConfigAssociationSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	shaderConfigAssociationSubObject.pDesc = shaderConfigAssociationDesc;
	subObjects.push_back(shaderConfigAssociationSubObject);

	// Raygen Root Signature
	m_raygenRootSignature = GetRaygenRootSignature(device);

	D3D12_STATE_SUBOBJECT raygenRootSigSubObject{};
	raygenRootSigSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	raygenRootSigSubObject.pDesc = m_raygenRootSignature.GetAddressOf();
	subObjects.push_back(raygenRootSigSubObject);


	// Raygen - Root Signature Association
	auto sharedRootSignatureAssociationDesc = stackAlloc.Allocate<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION>();
	sharedRootSignatureAssociationDesc->pExports = shaderExports;
	sharedRootSignatureAssociationDesc->NumExports = 1;
	sharedRootSignatureAssociationDesc->pSubobjectToAssociate = &subObjects.back();

	D3D12_STATE_SUBOBJECT sharedRootSignatureAssociationSubObject{};
	sharedRootSignatureAssociationSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	sharedRootSignatureAssociationSubObject.pDesc = sharedRootSignatureAssociationDesc;
	subObjects.push_back(sharedRootSignatureAssociationSubObject);


	// Pipeline Config
	auto pipelineConfigDesc = stackAlloc.Allocate<D3D12_RAYTRACING_PIPELINE_CONFIG>();
	pipelineConfigDesc->MaxTraceRecursionDepth = 1;

	D3D12_STATE_SUBOBJECT pipelineConfigSubObject{};
	pipelineConfigSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
	pipelineConfigSubObject.pDesc = pipelineConfigDesc;
	subObjects.push_back(pipelineConfigSubObject);
}

template<class MaterialType>
RtMaterialPipeline RaytraceMaterialPipeline::GetMaterialPipeline(StackAllocator& stackAlloc, ID3D12Device5* device)
{
	RtMaterialPipeline rtPipeline = {};

	// Closest Hit Shader
	auto chsExportDesc = stackAlloc.Allocate<D3D12_EXPORT_DESC>();
	chsExportDesc->Name = stackAlloc.ConstructName((std::wstring(L"ClosestHit") + MaterialType::k_name).c_str());
	chsExportDesc->ExportToRename = stackAlloc.ConstructName(L"ClosestHit");
	rtPipeline.closestHitShader.exportDesc = chsExportDesc;
	rtPipeline.closestHitShader.shaderBlob = LoadBlob(MaterialType::k_chs);

	// Miss Shader
	auto msExportDesc = stackAlloc.Allocate<D3D12_EXPORT_DESC>();
	msExportDesc->Name = stackAlloc.ConstructName((std::wstring(L"Miss") + MaterialType::k_name).c_str());
	msExportDesc->ExportToRename = stackAlloc.ConstructName(L"Miss");
	rtPipeline.missShader.exportDesc = msExportDesc;
	rtPipeline.missShader.shaderBlob = LoadBlob(MaterialType::k_ms);

	// Hit Group
	auto hitGroup = stackAlloc.Allocate<D3D12_HIT_GROUP_DESC>();
	hitGroup->ClosestHitShaderImport = chsExportDesc->Name;
	hitGroup->HitGroupExport = stackAlloc.ConstructName((std::wstring(L"HitGroup") + MaterialType::k_name).c_str());
	rtPipeline.hitGroupDesc = hitGroup;

	// Root Signature
	rtPipeline.rootSignature = MaterialType::GetRaytraceRootSignature(device);

	return rtPipeline;
}

template <class MaterialType>
void RaytraceMaterialPipeline::BuildFromMaterial(
	StackAllocator& stackAlloc,
	std::vector<D3D12_STATE_SUBOBJECT>& subObjects,
	const size_t payloadIndex,
	ID3D12Device5* device
)
{
	RtMaterialPipeline material = GetMaterialPipeline<MaterialType>(stackAlloc, device);

	// Closest Hit Subobject
	auto chsLibDesc = stackAlloc.Allocate<D3D12_DXIL_LIBRARY_DESC>();
	chsLibDesc->DXILLibrary.pShaderBytecode = material.closestHitShader.shaderBlob->GetBufferPointer();
	chsLibDesc->DXILLibrary.BytecodeLength = material.closestHitShader.shaderBlob->GetBufferSize();
	chsLibDesc->NumExports = 1;
	chsLibDesc->pExports = material.closestHitShader.exportDesc;

	D3D12_STATE_SUBOBJECT chsSubObject{};
	chsSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	chsSubObject.pDesc = chsLibDesc;
	subObjects.push_back(chsSubObject);


	// Miss Subobject
	auto msLibDesc = stackAlloc.Allocate<D3D12_DXIL_LIBRARY_DESC>();
	msLibDesc->DXILLibrary.pShaderBytecode = material.missShader.shaderBlob->GetBufferPointer();
	msLibDesc->DXILLibrary.BytecodeLength = material.missShader.shaderBlob->GetBufferSize();
	msLibDesc->NumExports = 1;
	msLibDesc->pExports = material.missShader.exportDesc;

	D3D12_STATE_SUBOBJECT msSubObject{};
	msSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	msSubObject.pDesc = msLibDesc;
	subObjects.push_back(msSubObject);


	// Hit Group Subobject
	D3D12_STATE_SUBOBJECT hitGroupSubObject{};
	hitGroupSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
	hitGroupSubObject.pDesc = material.hitGroupDesc;
	subObjects.push_back(hitGroupSubObject);


	// Payload Association Subobject
	auto shaderExports = stackAlloc.Allocate<const wchar_t*>(2);
	shaderExports[0] = material.missShader.exportDesc->Name;
	shaderExports[1] = material.hitGroupDesc->HitGroupExport;

	auto shaderConfigAssociationDesc = stackAlloc.Allocate<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION>();
	shaderConfigAssociationDesc->pExports = shaderExports;
	shaderConfigAssociationDesc->NumExports = 2;
	shaderConfigAssociationDesc->pSubobjectToAssociate = &subObjects[payloadIndex];

	D3D12_STATE_SUBOBJECT shaderConfigAssociationSubObject{};
	shaderConfigAssociationSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	shaderConfigAssociationSubObject.pDesc = shaderConfigAssociationDesc;
	subObjects.push_back(shaderConfigAssociationSubObject);


	// Material Root Signature Subobject
	auto rootSigComPtr = stackAlloc.Allocate<Microsoft::WRL::ComPtr<ID3D12RootSignature>>();
	*rootSigComPtr = material.rootSignature;
	D3D12_STATE_SUBOBJECT sharedRootSigSubObject{};
	sharedRootSigSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	sharedRootSigSubObject.pDesc = rootSigComPtr->GetAddressOf();
	subObjects.push_back(sharedRootSigSubObject);


	// Root Signature Association Subobject
	auto sharedRootSignatureAssociationDesc = stackAlloc.Allocate<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION>();
	sharedRootSignatureAssociationDesc->pExports = shaderExports;
	sharedRootSignatureAssociationDesc->NumExports = 2;
	sharedRootSignatureAssociationDesc->pSubobjectToAssociate = &subObjects.back();

	D3D12_STATE_SUBOBJECT sharedRootSignatureAssociationSubObject{};
	sharedRootSignatureAssociationSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	sharedRootSignatureAssociationSubObject.pDesc = sharedRootSignatureAssociationDesc;
	subObjects.push_back(sharedRootSignatureAssociationSubObject);
}

void RaytraceMaterialPipeline::Commit(const StackAllocator& stackAlloc, const std::vector<D3D12_STATE_SUBOBJECT>& subObjects, ID3D12Device5* device)
{
	assert(subObjects.size() < k_maxRtPipelineSubobjectCount);

	// Create the PSO
	D3D12_STATE_OBJECT_DESC psoDesc;
	psoDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
	psoDesc.NumSubobjects = static_cast<UINT>(subObjects.size());
	psoDesc.pSubobjects = subObjects.data();

	CHECK(device->CreateStateObject(
		&psoDesc,
		IID_PPV_ARGS(m_pso.GetAddressOf())
	));

	// Get the PSO properties
	CHECK(m_pso->QueryInterface(IID_PPV_ARGS(m_psoProperties.GetAddressOf())));
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

Microsoft::WRL::ComPtr<ID3D12RootSignature> DefaultOpaqueMaterial::GetRaytraceRootSignature(ID3D12Device5* device)
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