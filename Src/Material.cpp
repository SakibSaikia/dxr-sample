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

MaterialPipeline::MaterialPipeline(ID3D12Device* device, RenderPass::Id renderPass, VertexFormat::Type vertexFormat, const std::string vs, const std::string ps, const std::string rootSig)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};

	switch (vertexFormat)
	{
	case VertexFormat::Type::P3N3T3B3U2:
		desc.InputLayout.pInputElementDescs = VertexFormat::P3N3T3B3U2::inputLayout.data();
		desc.InputLayout.NumElements = VertexFormat::P3N3T3B3U2::inputLayout.size();
		break;
	case VertexFormat::Type::P3C3:
		desc.InputLayout.pInputElementDescs = VertexFormat::P3C3::inputLayout.data();
		desc.InputLayout.NumElements = VertexFormat::P3C3::inputLayout.size();
		break;
	default:
		assert(false);
	}

	// rasterizer state
	switch (renderPass)
	{
	case RenderPass::DepthOnly:
	case RenderPass::Geometry:
	case RenderPass::DebugDraw:
		desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		desc.RasterizerState.FrontCounterClockwise = FALSE;
		desc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		desc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		desc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		desc.RasterizerState.DepthClipEnable = TRUE;
		desc.RasterizerState.MultisampleEnable = FALSE;
		desc.RasterizerState.AntialiasedLineEnable = FALSE;
		desc.RasterizerState.ForcedSampleCount = 0;
		desc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		break;
	case RenderPass::Shadowmap:
		desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		desc.RasterizerState.FrontCounterClockwise = FALSE;
		desc.RasterizerState.DepthBias = 100000;
		desc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		desc.RasterizerState.SlopeScaledDepthBias = 1.f;
		desc.RasterizerState.DepthClipEnable = TRUE;
		desc.RasterizerState.MultisampleEnable = FALSE;
		desc.RasterizerState.AntialiasedLineEnable = FALSE;
		desc.RasterizerState.ForcedSampleCount = 0;
		desc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		break;
	default:
		assert(false);
	}

	// blend state
	switch (renderPass)
	{
	case RenderPass::DepthOnly:
		desc.BlendState.AlphaToCoverageEnable = FALSE;
		desc.BlendState.IndependentBlendEnable = FALSE;
		for (auto& rt : desc.BlendState.RenderTarget)
		{
			rt.BlendEnable = FALSE;
			rt.LogicOpEnable = FALSE;
			rt.RenderTargetWriteMask = 0;
		}
		break;
	case RenderPass::Geometry:
	case RenderPass::Shadowmap:
	case RenderPass::DebugDraw:
		desc.BlendState.AlphaToCoverageEnable = FALSE;
		desc.BlendState.IndependentBlendEnable = FALSE;
		for (auto& rt : desc.BlendState.RenderTarget)
		{
			rt.BlendEnable = FALSE;
			rt.LogicOpEnable = FALSE;
			rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		}
		break;
	default:
		assert(false);
	}

	// depth stencil state
	switch (renderPass)
	{
	case RenderPass::Geometry:
		desc.DepthStencilState.DepthEnable = TRUE;
		desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
		desc.DepthStencilState.StencilEnable = FALSE;
		break;
	case RenderPass::DepthOnly:
	case RenderPass::Shadowmap:
	case RenderPass::DebugDraw:
		desc.DepthStencilState.DepthEnable = TRUE;
		desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		desc.DepthStencilState.StencilEnable = FALSE;
		break;
	default:
		assert(false);
	}

	// RTV
	switch (renderPass)
	{
	case RenderPass::Geometry:
	case RenderPass::DebugDraw:
		desc.NumRenderTargets = 1;
		desc.RTVFormats[0] = k_backBufferRTVFormat;
		desc.DSVFormat = k_depthStencilFormatDsv;
		break;
	case RenderPass::DepthOnly:
	case RenderPass::Shadowmap:
		// turn off color writes
		desc.NumRenderTargets = 0;
		desc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
		desc.DSVFormat = k_depthStencilFormatDsv;
		break;
	default:
		assert(false);
	}

	// misc
	switch (renderPass)
	{
	case RenderPass::DepthOnly:
	case RenderPass::Geometry:
	case RenderPass::Shadowmap:
	case RenderPass::DebugDraw:
		desc.SampleMask = UINT_MAX;
		desc.SampleDesc.Count = 1;
		break;
	default:
		assert(false);
	}

	// Topology
	switch (renderPass)
	{
	case RenderPass::DepthOnly:
	case RenderPass::Geometry:
	case RenderPass::Shadowmap:
		desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		break;
	case RenderPass::DebugDraw:
		desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		break;
	default:
		assert(false);
	}

	// Shaders
	Microsoft::WRL::ComPtr<ID3DBlob> vsByteCode = LoadBlob(vs);
	Microsoft::WRL::ComPtr<ID3DBlob> psByteCode = LoadBlob(ps);

	// Root signature
	Microsoft::WRL::ComPtr<ID3DBlob> rsBytecode = LoadBlob(rootSig);
	CHECK(device->CreateRootSignature(
		0,
		rsBytecode->GetBufferPointer(),
		rsBytecode->GetBufferSize(),
		IID_PPV_ARGS(m_rootSignature.GetAddressOf())
	));

	// root sig specified in shader
	desc.pRootSignature = nullptr;

	// shaders
	desc.VS.pShaderBytecode = vsByteCode.Get()->GetBufferPointer();
	desc.VS.BytecodeLength = vsByteCode.Get()->GetBufferSize();
	desc.PS.pShaderBytecode = psByteCode.Get()->GetBufferPointer();
	desc.PS.BytecodeLength = psByteCode.Get()->GetBufferSize();

	CHECK(device->CreateGraphicsPipelineState(
		&desc,
		IID_PPV_ARGS(m_pso.GetAddressOf())
	));
}

void MaterialPipeline::Bind(ID3D12GraphicsCommandList* cmdList) const
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
	m_hash[RenderPass::DepthOnly] = std::hash<std::string>{}(k_depthonly_vs) ^
		(std::hash<std::string>{}(k_depthonly_ps) << 1) ^
		(std::hash<std::string>{}(k_depthonly_rootSig) << 2);

	m_hash[RenderPass::Geometry] = std::hash<std::string>{}(k_vs) ^ 
		(std::hash<std::string>{}(k_ps) << 1) ^ 
		(std::hash<std::string>{}(k_rootSig) << 2);

	m_hash[RenderPass::Shadowmap] = std::hash<std::string>{}(k_depthonly_vs) ^
		(std::hash<std::string>{}(k_depthonly_ps) << 1) ^
		(std::hash<std::string>{}(k_depthonly_rootSig) << 2);
}

void DefaultOpaqueMaterial::BindPipeline(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, RenderPass::Id pass, VertexFormat::Type vertexFormat)
{
	auto& pipeline = m_pipelines[pass][static_cast<int>(vertexFormat)];

	// Create a new pipeline and cache it if it has not been created
	if (!pipeline)
	{
		if (pass == RenderPass::Geometry)
		{
			pipeline = std::make_unique<MaterialPipeline>(device, pass, vertexFormat, k_vs, k_ps, k_rootSig);
		}
		else if (pass == RenderPass::DepthOnly)
		{
			pipeline = std::make_unique<MaterialPipeline>(device, pass, vertexFormat, k_depthonly_vs, k_depthonly_ps, k_depthonly_rootSig);
		}
		else if (pass == RenderPass::Shadowmap)
		{
			pipeline = std::make_unique<MaterialPipeline>(device, pass, vertexFormat, k_depthonly_vs, k_depthonly_ps, k_depthonly_rootSig);
		}
	}

	pipeline->Bind(cmdList);
}

void DefaultOpaqueMaterial::BindConstants(RenderPass::Id pass, ID3D12GraphicsCommandList* cmdList, D3D12_GPU_VIRTUAL_ADDRESS objConstants, D3D12_GPU_VIRTUAL_ADDRESS viewConstants, D3D12_GPU_VIRTUAL_ADDRESS lightConstants, D3D12_GPU_VIRTUAL_ADDRESS shadowConstants, D3D12_GPU_DESCRIPTOR_HANDLE renderSurfaceSrvBegin) const
{
	if (pass == RenderPass::Geometry)
	{
		cmdList->SetGraphicsRootConstantBufferView(0, viewConstants);
		cmdList->SetGraphicsRootConstantBufferView(1, objConstants);
		cmdList->SetGraphicsRootConstantBufferView(2, lightConstants);
		cmdList->SetGraphicsRootConstantBufferView(3, shadowConstants);
		cmdList->SetGraphicsRootDescriptorTable(4, renderSurfaceSrvBegin);
		cmdList->SetGraphicsRootDescriptorTable(5, m_srvBegin);
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
	m_hash[RenderPass::DepthOnly] = std::hash<std::string>{}(k_depthonly_vs) ^
		(std::hash<std::string>{}(k_depthonly_ps) << 1) ^
		(std::hash<std::string>{}(k_depthonly_rootSig) << 2);

	m_hash[RenderPass::Geometry] = std::hash<std::string>{}(k_vs) ^
		(std::hash<std::string>{}(k_ps) << 1) ^ 
		(std::hash<std::string>{}(k_rootSig) << 2);

	m_hash[RenderPass::Shadowmap] = std::hash<std::string>{}(k_depthonly_vs) ^
		(std::hash<std::string>{}(k_depthonly_ps) << 1) ^
		(std::hash<std::string>{}(k_depthonly_rootSig) << 2);
}

void DefaultMaskedMaterial::BindPipeline(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, RenderPass::Id pass, VertexFormat::Type vertexFormat)
{
	auto& pipeline = m_pipelines[pass][static_cast<int>(vertexFormat)];

	// Create a new pipeline and cache it if it has not been created
	if (!pipeline)
	{
		if (pass == RenderPass::Geometry)
		{
			pipeline = std::make_unique<MaterialPipeline>(device, pass, vertexFormat, k_vs, k_ps, k_rootSig);
		}
		else if (pass == RenderPass::DepthOnly)
		{
			pipeline = std::make_unique<MaterialPipeline>(device, pass, vertexFormat, k_depthonly_vs, k_depthonly_ps, k_depthonly_rootSig);
		}
		else if (pass == RenderPass::Shadowmap)
		{
			pipeline = std::make_unique<MaterialPipeline>(device, pass, vertexFormat, k_depthonly_vs, k_depthonly_ps, k_depthonly_rootSig);
		}
	}

	pipeline->Bind(cmdList);
}

void DefaultMaskedMaterial::BindConstants(RenderPass::Id pass, ID3D12GraphicsCommandList* cmdList, D3D12_GPU_VIRTUAL_ADDRESS objConstants, D3D12_GPU_VIRTUAL_ADDRESS viewConstants, D3D12_GPU_VIRTUAL_ADDRESS lightConstants, D3D12_GPU_VIRTUAL_ADDRESS shadowConstants, D3D12_GPU_DESCRIPTOR_HANDLE renderSurfaceSrvBegin) const
{
	if (pass == RenderPass::Geometry)
	{
		cmdList->SetGraphicsRootConstantBufferView(0, viewConstants);
		cmdList->SetGraphicsRootConstantBufferView(1, objConstants);
		cmdList->SetGraphicsRootConstantBufferView(2, lightConstants);
		cmdList->SetGraphicsRootConstantBufferView(3, shadowConstants);
		cmdList->SetGraphicsRootDescriptorTable(4, renderSurfaceSrvBegin);
		cmdList->SetGraphicsRootDescriptorTable(5, m_srvBegin);
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

DebugMaterial::DebugMaterial() :
	Material{ "debug_mtl" }
{
}

void DebugMaterial::BindPipeline(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, RenderPass::Id renderPass, VertexFormat::Type vertexFormat)
{
	auto& pipeline = m_pipelines[renderPass][static_cast<int>(vertexFormat)];

	// Create a new pipeline and cache it if it has not been created
	if (!pipeline)
	{
		pipeline = std::make_unique<MaterialPipeline>(device, renderPass, vertexFormat, k_vs, k_ps, k_rootSig);
	}

	pipeline->Bind(cmdList);
}