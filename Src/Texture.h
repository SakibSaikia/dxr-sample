#pragma once

class Texture
{
public:
	void Init(ID3D12Device* device, DirectX::ResourceUploadBatch& resourceUpload, const std::string& name);
private:
	std::string m_name;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;
};