#include "stdafx.h"
#include "Texture.h"

void Texture::Init(ID3D12Device* device, DirectX::ResourceUploadBatch& resourceUpload, const std::string& name)
{
	m_name = name;

	// Convert UTF-8 string to wchar (See: http://forums.codeguru.com/showthread.php?t=231165 )
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &name[0], static_cast<int>(name.size()), nullptr, 0);
	std::wstring wideName(name.size(), 0);
	MultiByteToWideChar(CP_UTF8, 0, &name[0], static_cast<int>(name.size()), &wideName[0], size_needed);
	std::wstring filePath = L"..\\Content\\Sponza\\textures\\Compressed\\" + wideName + L".dds";

	HRESULT hr = DirectX::CreateDDSTextureFromFile(
		device,
		resourceUpload,
		filePath.c_str(),
		m_resource.ReleaseAndGetAddressOf()
	);

	//assert(hr == S_OK);
}