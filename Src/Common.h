#pragma once

constexpr float k_Pi = 3.1415926535f;

constexpr size_t k_gfxBufferCount = 2;
constexpr size_t k_screenWidth = 1280;
constexpr size_t k_screenHeight = 720;
constexpr size_t k_materialTextureCount = 128;
constexpr size_t k_objectCount = 512;
constexpr size_t k_uploadBufferSize = 40 * 1024 * 1024; // 40 MB
constexpr size_t k_scratchDataSize = 40 * 1024 * 1024; // 40 MB
constexpr size_t k_geometryDataSize = 100 * 1024 * 1024; // 100 MB
constexpr size_t k_materialConstantsSize = 20 * 1024 * 1024; // 20 MB
constexpr size_t k_constantBufferAlignment = 256 * 1024; // 256 K
constexpr size_t k_maxRootSignatureSize = 6 * 2 * sizeof(DWORD); // 6 descriptor tables or 6 root parameters or 12 root constants
constexpr size_t k_shaderRecordSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + k_maxRootSignatureSize;
constexpr size_t k_maxRtPipelineSubobjectCount = 64;
constexpr DXGI_FORMAT k_backBufferFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
constexpr DXGI_FORMAT k_backBufferRTVFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
constexpr DXGI_FORMAT k_depthStencilFormatRaw = DXGI_FORMAT_R24G8_TYPELESS;
constexpr DXGI_FORMAT k_depthStencilFormatDsv = DXGI_FORMAT_D24_UNORM_S8_UINT;
constexpr DXGI_FORMAT k_depthStencilFormatSrv = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

namespace RTV
{
	enum Id
	{
		SwapChain,
		Count = k_gfxBufferCount
	};
}

namespace SrvUav
{
	enum Id
	{
		// UAVs
		UAVBegin,
		DxrOutputUAV = UAVBegin,

		// Mesh SRVs (VB + IB)
		MeshdataBegin,
		MeshdataEnd = MeshdataBegin + k_objectCount * 2,

		// TLAS SRVs
		TLASBegin,
		TLASEnd = TLASBegin + k_objectCount,

		// Material SRVs
		MaterialTexturesBegin,
		MaterialTexturesEnd = MaterialTexturesBegin + k_materialTextureCount,

		Count
	};
}

namespace VertexFormat
{
	enum class Type
	{
		P3N3T3B3U2,
		P3C3,
		Count
	};

	struct P3N3T3B3U2
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT3 normal;
		DirectX::XMFLOAT3 tangent;
		DirectX::XMFLOAT3 bitangent;
		DirectX::XMFLOAT2 uv;

		static inline std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout =
		{
			{ "POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL",		0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT",	0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "BITANGENT",	0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD",	0, DXGI_FORMAT_R32G32_FLOAT,	0, 48,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		P3N3T3B3U2() = default;
		P3N3T3B3U2(DirectX::XMFLOAT3 inPos, DirectX::XMFLOAT3 inNormal, DirectX::XMFLOAT3 inTangent, DirectX::XMFLOAT3 inBitangent, DirectX::XMFLOAT2 inUV) :
			position(inPos), normal(inNormal), tangent(inTangent), bitangent(inBitangent), uv(inUV) {}
	};

	struct P3C3
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT3 color;

		static inline std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout =
		{
			{ "POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR",		0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		P3C3() = default;
		P3C3(DirectX::XMFLOAT3 inPos, DirectX::XMFLOAT3 inColor) :
			position(inPos), color(inColor) {}
	};

}