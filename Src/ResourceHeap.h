#pragma once

#include "Common.h"

class ResourceHeap
{
public:
	using Alloc = size_t;

	void Init(ID3D12Device* device, const size_t sizeInBytes);
	Alloc GetAlloc(const size_t sizeInBytes, const size_t alignment = k_defaultResourceAlignment);
	ID3D12Heap* GetHeap() const;

private:
	Microsoft::WRL::ComPtr<ID3D12Heap> m_heap;
	size_t m_totalSize;
	size_t m_allocatedSize;
};