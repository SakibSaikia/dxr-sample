#pragma once

#include "Common.h"

class UploadBuffer
{
public:
	using Alloc = std::pair<uint8_t*, size_t>;

	~UploadBuffer();
	void Init(ID3D12Device5* device, const size_t sizeInBytes);
	Alloc GetAlloc(const size_t sizeInBytes);
	ID3D12Resource* GetResource() const;
	void Flush();

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_buffer;
	uint8_t* m_dataPtr;
	size_t m_totalSize;
	size_t m_allocatedSize;
};
