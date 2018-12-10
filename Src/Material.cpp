#include "stdafx.h"
#include "Material.h"

Microsoft::WRL::ComPtr<ID3DBlob> LoadBlob(const std::string& filename)
{
	std::string filepath = R"(CompiledShaders\)" + filename + R"(.cso)";
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

RaytraceMaterialPipeline::RaytraceMaterialPipeline(ID3D12Device5* device, RenderPass::Id renderPass, const std::string rgs, const std::string chs, const std::string ms, const D3D12_ROOT_SIGNATURE_DESC& rootSigDesc)
{
	// -----------------------------------------------------------------------------
	// Root Signature
	// -----------------------------------------------------------------------------
	m_raytraceRootSignature = CreateRootSignature(device, rootSigDesc);
	m_raytraceRootSignatureSize = GetRootSignatureSizeInBytes(device, rootSigDesc);


	// -----------------------------------------------------------------------------
	// PSO
	// -----------------------------------------------------------------------------
	std::vector<D3D12_STATE_SUBOBJECT> rtSubObjects;
	rtSubObjects.reserve(10);

	// -------------------- Ray Generation Shader -------------------------------
	Microsoft::WRL::ComPtr<ID3DBlob> rgsByteCode = LoadBlob(rgs);
	std::wstring rgsExportName(rgs.begin(), rgs.end());

	D3D12_EXPORT_DESC rgsExportDesc{};
	rgsExportDesc.Name = rgsExportName.c_str();
	rgsExportDesc.ExportToRename = L"RayGen";
	

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
	std::wstring chsExportName(chs.begin(), chs.end());

	D3D12_EXPORT_DESC chsExportDesc{};
	chsExportDesc.Name = chsExportName.c_str();
	chsExportDesc.ExportToRename = L"ClosestHit";

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
	std::wstring msExportName(ms.begin(), ms.end());

	D3D12_EXPORT_DESC msExportDesc{};
	msExportDesc.Name = msExportName.c_str();
	msExportDesc.ExportToRename = L"Miss";

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
	hitGroupDesc.ClosestHitShaderImport = chsExportName.c_str();
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

size_t RaytraceMaterialPipeline::GetRootSignatureSize() const
{
	return m_raytraceRootSignatureSize;
}

void* RaytraceMaterialPipeline::GetShaderIdentifier(const std::string& exportName) const
{
	std::wstring exportNameW(exportName.begin(), exportName.end());
	return m_psoProperties->GetShaderIdentifier(exportNameW.c_str());
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
		auto rootSigDesc = BuildRaytraceRootSignature(device, pass);
		pipeline = std::make_unique<RaytraceMaterialPipeline>(device, pass, k_rgs, k_chs, k_ms, rootSigDesc);
	}

	pipeline->Bind(cmdList);
}

D3D12_ROOT_SIGNATURE_DESC DefaultOpaqueMaterial::BuildRaytraceRootSignature(ID3D12Device5* device, RenderPass::Id pass)
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
		rootDesc.Flags = rootParameters.size();
		rootDesc.pParameters = rootParameters.data();
		rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
	}

	return rootDesc;
}

size_t DefaultOpaqueMaterial::GetSBTEntrySize(RenderPass::Id pass) const
{
	size_t sbtEntrySize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + m_raytracePipelines[pass]->GetRootSignatureSize();
	sbtEntrySize = (sbtEntrySize + D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT - 1) & ~D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT;

	return sbtEntrySize;
}

void DefaultOpaqueMaterial::CreateShaderBindingTable(ID3D12Device5* device, RenderPass::Id pass)
{
	const size_t sbtSize = GetSBTEntrySize(pass) * 3; // For RGS, CHS and MS

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; 
	resDesc.Width = sbtSize * k_gfxBufferCount; // n copies where n = buffer count
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.Format = DXGI_FORMAT_UNKNOWN;
	resDesc.SampleDesc.Count = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	D3D12_HEAP_PROPERTIES heapDesc = {};
	heapDesc.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapDesc.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	CHECK(device->CreateCommittedResource(
		&heapDesc,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_shaderBindingTable[pass].GetAddressOf())
	));

	D3D12_RANGE readRange = { 0 };
	CHECK(shaderBindingTable[pass]->Map(0, &readRange, reinterpret_cast<void**)&sbtPtr);
}

void DefaultOpaqueMaterial::BindConstants(uint32_t bufferIndex, RenderPass::Id pass, ID3D12GraphicsCommandList4* cmdList, D3D12_GPU_VIRTUAL_ADDRESS tlas, D3D12_GPU_DESCRIPTOR_HANDLE meshBuffer, D3D12_GPU_VIRTUAL_ADDRESS objConstants, D3D12_GPU_VIRTUAL_ADDRESS viewConstants, D3D12_GPU_VIRTUAL_ADDRESS lightConstants, D3D12_GPU_VIRTUAL_ADDRESS shadowConstants, D3D12_GPU_DESCRIPTOR_HANDLE outputUAV) const
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
		auto rootSigDesc = BuildRaytraceRootSignature(device, pass);
		pipeline = std::make_unique<RaytraceMaterialPipeline>(device, pass, k_rgs, k_chs, k_ms, rootSigDesc);
	}

	pipeline->Bind(cmdList);
}

D3D12_ROOT_SIGNATURE_DESC DefaultMaskedMaterial::BuildRaytraceRootSignature(ID3D12Device5* device, RenderPass::Id pass)
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
		rootDesc.Flags = rootParameters.size();
		rootDesc.pParameters = rootParameters.data();
		rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
	}

	retunr rootDesc;
}

size_t DefaultMaskedMaterial::GetSBTEntrySize(RenderPass::Id pass) const
{
	size_t sbtEntrySize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + m_raytracePipelines[pass]->GetRootSignatureSize();
	sbtEntrySize = (sbtEntrySize + D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT - 1) & ~D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT;

	return sbtEntrySize;
}

void DefaultMaskedMaterial::CreateShaderBindingTable(ID3D12Device5* device, RenderPass::Id pass)
{
	const size_t sbtSize = GetSBTEntrySize(pass) * 3; // For RGS, CHS and MS

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Width = sbtSize * k_gfxBufferCount; // n copies where n = buffer count
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.Format = DXGI_FORMAT_UNKNOWN;
	resDesc.SampleDesc.Count = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	D3D12_HEAP_PROPERTIES heapDesc = {};
	heapDesc.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapDesc.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	CHECK(device->CreateCommittedResource(
		&heapDesc,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_shaderBindingTable[pass].GetAddressOf())
	));

	D3D12_RANGE readRange = { 0 };
	CHECK(shaderBindingTable[pass]->Map(0, &readRange, reinterpret_cast<void**)&sbtPtr);
}

void DefaultMaskedMaterial::BindConstants(uint32_t bufferIndex, RenderPass::Id pass, ID3D12GraphicsCommandList4* cmdList, D3D12_GPU_VIRTUAL_ADDRESS tlas, D3D12_GPU_DESCRIPTOR_HANDLE meshBuffer, D3D12_GPU_VIRTUAL_ADDRESS objConstants, D3D12_GPU_VIRTUAL_ADDRESS viewConstants, D3D12_GPU_VIRTUAL_ADDRESS lightConstants, D3D12_GPU_VIRTUAL_ADDRESS shadowConstants, D3D12_GPU_DESCRIPTOR_HANDLE outputUAV) const
{
	if (pass == RenderPass::Raytrace)
	{
		if (m_sbtPtr[pass] == nullptr)
		{
			CreateShaderBindingTable(device, pass);
		}

		const size_t sbtEntrySize = GetSBTEntrySize(pass);
		const size_t sbtSize = sbtEntrySize * 3; // For RGS, CHS and MS

		uint8_t* pData = m_sbtPtr[pass] + bufferIndex * sbtSize;

		// Entry 0 - Ray Generation Program
		{
			memcpy(pData, m_raytracePipelines[pass]->GetShaderIdentifier(k_rgs), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

			D3D12_GPU_VIRTUAL_ADDRESS* pRootDescriptors = *reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
			*(pRootDescriptors++) = viewConstants;
			*(pRootDescriptors++) = lightConstants;
			*(pRootDescriptors++) = outputUAV;
			*(pRootDescriptors++) = tlas;

			D3D12_GPU_DESCRIPTOR_HANDLE* pDescriptorTables = *reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE>(pRootDescriptors);
			(*pDescriptorTables++) = meshBuffer;
			(*pDescriptorTables++) = m_srvBegin;
		}

		pData += sbtEntrySize;

		// Entry 1 - Miss Program
		{
			memcpy(pData, m_raytracePipelines[pass]->GetShaderIdentifier(k_ms), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
			D3D12_GPU_VIRTUAL_ADDRESS* pRootDescriptors = *reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
			*(pRootDescriptors++) = viewConstants;
			*(pRootDescriptors++) = lightConstants;
			*(pRootDescriptors++) = outputUAV;
			*(pRootDescriptors++) = tlas;

			D3D12_GPU_DESCRIPTOR_HANDLE* pDescriptorTables = *reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE>(pRootDescriptors);
			(*pDescriptorTables++) = meshBuffer;
			(*pDescriptorTables++) = m_srvBegin;
		}

		pData += sbtEntrySize;

		// Entry 2 - Closest Hit Program / Hit Group
		{
			memcpy(pData, m_raytracePipelines[pass]->GetShaderIdentifier(L"HitGroup"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
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
		auto rootSigDesc = BuildRaytraceRootSignature(device, pass);
		pipeline = std::make_unique<RaytraceMaterialPipeline>(device, pass, k_rgs, k_chs, k_ms, rootSigDesc);
	}

	pipeline->Bind(cmdList);
}

D3D12_ROOT_SIGNATURE_DESC UntexturedMaterial::BuildRaytraceRootSignature(ID3D12Device5* device, RenderPass::Id pass)
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
		rootDesc.Flags = rootParameters.size();
		rootDesc.pParameters = rootParameters.data();
		rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
	}

	return rootDesc;
}

size_t UntexturedMaterial::GetSBTEntrySize(RenderPass::Id pass) const
{
	size_t sbtEntrySize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + m_raytracePipelines[pass]->GetRootSignatureSize();
	sbtEntrySize = (sbtEntrySize + D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT - 1) & ~D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT;

	return sbtEntrySize;
}

void UntexturedMaterial::CreateShaderBindingTable(ID3D12Device5* device, RenderPass::Id pass)
{
	const size_t sbtSize = GetSBTEntrySize(pass) * 3; // For RGS, CHS and MS

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Width = sbtSize * k_gfxBufferCount; // n copies where n = buffer count
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.Format = DXGI_FORMAT_UNKNOWN;
	resDesc.SampleDesc.Count = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	D3D12_HEAP_PROPERTIES heapDesc = {};
	heapDesc.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapDesc.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	CHECK(device->CreateCommittedResource(
		&heapDesc,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_shaderBindingTable[pass].GetAddressOf())
	));

	D3D12_RANGE readRange = { 0 };
	CHECK(shaderBindingTable[pass]->Map(0, &readRange, reinterpret_cast<void**)&sbtPtr);
}

void UntexturedMaterial::BindConstants(uint32_t bufferIndex, RenderPass::Id pass, ID3D12GraphicsCommandList4* cmdList, D3D12_GPU_VIRTUAL_ADDRESS tlas, D3D12_GPU_DESCRIPTOR_HANDLE meshBuffer, D3D12_GPU_VIRTUAL_ADDRESS objConstants, D3D12_GPU_VIRTUAL_ADDRESS viewConstants, D3D12_GPU_VIRTUAL_ADDRESS lightConstants, D3D12_GPU_VIRTUAL_ADDRESS shadowConstants, D3D12_GPU_DESCRIPTOR_HANDLE outputUAV) const
{
	if (pass == RenderPass::Raytrace)
	{
		uint8_t* pData = m_sbtPtr[pass];

		// Entry 0 - Raygen program and root argument data
		memcpy(pData, m_raytracePipelines[])

		cmdList->SetGraphicsRootConstantBufferView(0, viewConstants);
		cmdList->SetGraphicsRootConstantBufferView(1, objConstants);
		cmdList->SetGraphicsRootConstantBufferView(2, lightConstants);
		cmdList->SetGraphicsRootConstantBufferView(3, shadowConstants);
		cmdList->SetGraphicsRootConstantBufferView(4, m_constantBuffer->GetGPUVirtualAddress());
		cmdList->SetGraphicsRootDescriptorTable(5, renderSurfaceSrvBegin);
	}
}

size_t UntexturedMaterial::GetHash(RenderPass::Id renderPass, VertexFormat::Type vertexFormat) const
{
	return	m_hash[renderPass] ^ (renderPass << 3) ^ (static_cast<int>(vertexFormat) << 4);
}