#include "DX12ResourceManager.h"
#include <algorithm>

BEG_NAME_SPACE

bool DX12ResourceManager::Initialize() {
	D3D12MA::ALLOCATOR_DESC aloctrDesc;
	aloctrDesc.Flags = D3D12MA::ALLOCATOR_FLAG_NONE;
	aloctrDesc.pDevice = m_pDevice;
	aloctrDesc.PreferredBlockSize = 0;
	aloctrDesc.pAllocationCallbacks = nullptr;
	if (FAILED(D3D12MA::CreateAllocator(&aloctrDesc, &m_pAllocator)))
		return false;
	return true;
}

auto DX12ResourceManager::RequestBuffer(size_t size, D3D12_RESOURCE_FLAGS flags,
	D3D12_HEAP_TYPE heapType, bool bForceCommitted)
->std::pair<ID3D12Resource*, D3D12_RESOURCE_STATES> {
	/** 暂不支持custom类型 */
	assert(heapType != D3D12_HEAP_TYPE_CUSTOM);

	auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(size, flags);
	auto allocInfo = m_pDevice->GetResourceAllocationInfo(0, 1, &bufferDesc);
	SmartPTR<ID3D12Resource> buffer;
	D3D12_RESOURCE_STATES state = heapType == D3D12_HEAP_TYPE_UPLOAD ?
		D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COPY_DEST;;
	DX12ResourceHeapManager::BlockInfo bInfo;

	ResourceInfo newResource(state);

	D3D12MA::ALLOCATION_DESC allocDesc;
	allocDesc.Flags = bForceCommitted ? D3D12MA::ALLOCATION_FLAG_COMMITTED : D3D12MA::ALLOCATION_FLAG_NONE;
	allocDesc.HeapType = heapType;
	if (FAILED(m_pAllocator->CreateResource(&allocDesc, &bufferDesc, state, nullptr,
		&newResource.pAllocation, IID_PPV_ARGS(&newResource.pResource)))) {
		return { nullptr, D3D12_RESOURCE_STATE_COMMON };
	}

	m_resourceRepository.insert(std::make_pair(newResource.pResource, newResource));
	return { newResource.pResource, state };
}

auto DX12ResourceManager::RequestTexture(uint32_t width, uint32_t height, uint32_t slice, DXGI_FORMAT elementFormat,
	D3D12_RESOURCE_FLAGS flags,
	uint16_t mipLevel, bool bForceCommitted)->std::pair<ID3D12Resource*, D3D12_RESOURCE_STATES> {
	CD3DX12_RESOURCE_DESC texDesc;
	if (slice)
		texDesc = CD3DX12_RESOURCE_DESC::Tex3D(elementFormat, width, height, slice, mipLevel, flags);
	else if (height)
		texDesc = CD3DX12_RESOURCE_DESC::Tex2D(elementFormat, width, height, 1, mipLevel, 1, 0, flags);
	else if (width)
		texDesc = CD3DX12_RESOURCE_DESC::Tex1D(elementFormat, width, 1, mipLevel, flags);
	else
		return { nullptr, D3D12_RESOURCE_STATE_COMMON };

	auto allocInfo = m_pDevice->GetResourceAllocationInfo(0, 1, &texDesc);
	SmartPTR<ID3D12Resource> texture;
	D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COPY_DEST;
	
	ResourceInfo newResource(state);
	D3D12MA::ALLOCATION_DESC allocDesc;
	allocDesc.Flags = bForceCommitted ? D3D12MA::ALLOCATION_FLAG_COMMITTED : D3D12MA::ALLOCATION_FLAG_NONE;
	allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

	/** 申请构造纹理资源 */
	if (FAILED(m_pAllocator->CreateResource(&allocDesc, &texDesc, state, nullptr,
		&newResource.pAllocation, IID_PPV_ARGS(&newResource.pResource)))) {
		return { nullptr, D3D12_RESOURCE_STATE_COMMON };
	}
	m_resourceRepository.insert(std::make_pair(newResource.pResource, newResource));
	return { newResource.pResource, state };
}

bool DX12ResourceManager::ReleaseResource(ID3D12Resource* res) {
	auto target = m_resourceRepository.find(res);
	if (target != m_resourceRepository.end()) {
		m_resourceRepository.erase(target);
		return true;
	}
	else {
		return false;
	}
}

inline void DX12ResourceManager::swapWithRValue(DX12ResourceManager&& rvalue) {
	m_pAllocator = rvalue.m_pAllocator; rvalue.m_pAllocator = nullptr;
	m_pDevice = rvalue.m_pDevice; rvalue.m_pDevice = nullptr;
	m_resourceRepository.swap(rvalue.m_resourceRepository);
}

END_NAME_SPACE
