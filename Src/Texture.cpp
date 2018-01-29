#include <DDSTextureLoader.h>
#include <cassert>
#include "Texture.h"

void Texture::Init(ID3D12Device* device, DirectX::ResourceUploadBatch& resourceUpload, const std::wstring& name)
{
	m_name = name;
	std::wstring filePath = L"..\\Content\\Sponza\\textures\\Compressed\\" + name + L".dds";

	DirectX::CreateDDSTextureFromFile(
		device,
		resourceUpload,
		filePath.c_str(),
		m_resource.ReleaseAndGetAddressOf()
	);
}