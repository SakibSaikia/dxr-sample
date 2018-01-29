#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <ResourceUploadBatch.h>

class Texture
{
public:
	void Init(ID3D12Device* device, DirectX::ResourceUploadBatch& resourceUpload, const std::wstring& name);
private:
	std::wstring m_name;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;
};