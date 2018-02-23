#pragma once

class Texture
{
public:
	void Init(ID3D12Device* device, DirectX::ResourceUploadBatch& resourceUpload, const std::string& name);
	D3D12_GPU_DESCRIPTOR_HANDLE CreateDescriptor(ID3D12Device* device, ID3D12DescriptorHeap* srvHeap, size_t offsetInHeap, size_t descriptorSize) const;
	ID3D12Resource* GetResource() const;
	const std::string& GetName() const;
private:
	std::string m_name;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;
};