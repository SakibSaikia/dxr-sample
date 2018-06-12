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

MaterialPipeline::MaterialPipeline(ID3D12Device* device, RenderPass renderPass, VertexFormat::Type vertexFormat, const std::string vs, const std::string ps, const std::string rootSig)
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

	switch (renderPass)
	{
	case RenderPass::Geometry:
		// rasterizer state
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
		// blend state
		desc.BlendState.AlphaToCoverageEnable = FALSE;
		desc.BlendState.IndependentBlendEnable = FALSE;
		for (auto& rt : desc.BlendState.RenderTarget)
		{
			rt.BlendEnable = FALSE;
			rt.LogicOpEnable = FALSE;
			rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		}
		// depth stencil state
		desc.DepthStencilState.DepthEnable = TRUE;
		desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		desc.DepthStencilState.StencilEnable = FALSE;
		// RTV
		desc.NumRenderTargets = 1;
		desc.RTVFormats[0] = k_backBufferRTVFormat;
		desc.DSVFormat = k_depthStencilFormatDsv;
		// misc
		desc.SampleMask = UINT_MAX;
		desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		desc.SampleDesc.Count = 1;
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
}

void DefaultOpaqueMaterial::BindPipeline(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, RenderPass renderPass, VertexFormat::Type vertexFormat)
{
	auto& pipeline = m_pipelines[static_cast<int>(renderPass)][static_cast<int>(vertexFormat)];

	// Create a new pipeline and cache it if it has not been created
	if (!pipeline)
	{
		pipeline = std::make_unique<MaterialPipeline>(device, renderPass, vertexFormat, k_vs, k_ps, k_rootSig);
	}

	pipeline->Bind(cmdList);
}

void DefaultOpaqueMaterial::BindConstants(ID3D12GraphicsCommandList* cmdList) const
{
	cmdList->SetGraphicsRootDescriptorTable(k_srvDescriptorIndex, m_srvBegin);
}

DefaultMaskedMaterial::DefaultMaskedMaterial(std::string& name, const D3D12_GPU_DESCRIPTOR_HANDLE srvHandle) :
	Material{ name },
	m_srvBegin{ srvHandle }
{
}

void DefaultMaskedMaterial::BindPipeline(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, RenderPass renderPass, VertexFormat::Type vertexFormat)
{
	auto& pipeline = m_pipelines[static_cast<int>(renderPass)][static_cast<int>(vertexFormat)];

	// Create a new pipeline and cache it if it has not been created
	if (!pipeline)
	{
		pipeline = std::make_unique<MaterialPipeline>(device, renderPass, vertexFormat, k_vs, k_ps, k_rootSig);
	}

	pipeline->Bind(cmdList);
}

void DefaultMaskedMaterial::BindConstants(ID3D12GraphicsCommandList* cmdList) const
{
	cmdList->SetGraphicsRootDescriptorTable(k_srvDescriptorIndex, m_srvBegin);
}