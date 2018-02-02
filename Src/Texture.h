#pragma once

class Texture
{
public:
	void Init(ID3D12Device* device, DirectX::ResourceUploadBatch& resourceUpload, const std::string& name);
	void SetDescriptor(D3D12_GPU_DESCRIPTOR_HANDLE desc);
	D3D12_GPU_DESCRIPTOR_HANDLE GetDescriptor() const;
	ID3D12Resource* GetResource() const;
private:
	std::string m_name;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;
	D3D12_GPU_DESCRIPTOR_HANDLE m_srv;
};