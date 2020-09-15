#include "DX12ResourceManager.h"
#include <algorithm>
#include "../../Utility/InfoLog/InfoLog.h"

BEG_NAME_SPACE

bool DX12ResourceManager::Initialize(IDXGIAdapter* adapter) {
	D3D12MA::ALLOCATOR_DESC aloctrDesc;
	aloctrDesc.Flags = D3D12MA::ALLOCATOR_FLAG_NONE;
	aloctrDesc.pDevice = m_pDevice;
	aloctrDesc.PreferredBlockSize = 0;
	aloctrDesc.pAllocationCallbacks = nullptr;
	aloctrDesc.pAdapter = adapter;
	if (FAILED(D3D12MA::CreateAllocator(&aloctrDesc, &m_pAllocator)))
		return false;
	return true;
}

DX12ResourceManager::~DX12ResourceManager() {

	/** Note: 确保调用时所有资源的引用都已经被释放 */
	for (auto& resource : m_resourceRepository) {
		resource.first->Release();
		resource.second.pAllocation->Release();
	}
	m_pAllocator->Release();

}

auto DX12ResourceManager::RequestBuffer(size_t size, D3D12_RESOURCE_FLAGS flags,
	D3D12_HEAP_TYPE heapType, bool bForceCommitted)
->std::pair<ID3D12Resource*, D3D12_RESOURCE_STATES> {
	/** 暂不支持custom类型 */
	if (heapType == D3D12_HEAP_TYPE_CUSTOM) {
		LOG_ERROR("Doesn't support custom heap");
		abort();
	}
	
	auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(size, flags);
	SmartPTR<ID3D12Resource> buffer;
	D3D12_RESOURCE_STATES state = heapType == D3D12_HEAP_TYPE_UPLOAD ?
		D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COPY_DEST;;

	ResourceInfo newResource(state);

	D3D12MA::ALLOCATION_DESC allocDesc;
	allocDesc.Flags = bForceCommitted ? D3D12MA::ALLOCATION_FLAG_COMMITTED : D3D12MA::ALLOCATION_FLAG_NONE;
	allocDesc.HeapType = heapType;
	allocDesc.CustomPool = nullptr;
	allocDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_NONE;
	if (FAILED(m_pAllocator->CreateResource(&allocDesc, &bufferDesc, state, nullptr,
		&newResource.pAllocation, IID_PPV_ARGS(&newResource.pResource)))) {
		return { nullptr, D3D12_RESOURCE_STATE_COMMON };
	}

	m_resourceRepository.insert({ newResource.pResource, newResource });
	return { newResource.pResource, state };
}

auto DX12ResourceManager::RequestTexture(uint32_t width, uint32_t height, uint32_t slice, DXGI_FORMAT elementFormat,
	D3D12_RESOURCE_FLAGS flags, uint16_t arraySize,
	uint16_t mipLevel, bool bForceCommitted)->std::pair<ID3D12Resource*, D3D12_RESOURCE_STATES> {
	CD3DX12_RESOURCE_DESC texDesc;
	if (slice)
		texDesc = CD3DX12_RESOURCE_DESC::Tex3D(elementFormat, width, height, slice, mipLevel, flags);
	else if (height)
		texDesc = CD3DX12_RESOURCE_DESC::Tex2D(elementFormat, width, height, arraySize, mipLevel, 1, 0, flags);
	else if (width)
		texDesc = CD3DX12_RESOURCE_DESC::Tex1D(elementFormat, width, arraySize, mipLevel, flags);
	else
		return { nullptr, D3D12_RESOURCE_STATE_COMMON };

	SmartPTR<ID3D12Resource> texture;
	D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COPY_DEST;
	
	ResourceInfo newResource(state);
	D3D12MA::ALLOCATION_DESC allocDesc;
	allocDesc.Flags = bForceCommitted ? D3D12MA::ALLOCATION_FLAG_COMMITTED : D3D12MA::ALLOCATION_FLAG_NONE;
	allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
	allocDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_NONE;
	allocDesc.CustomPool = nullptr;
	D3D12_CLEAR_VALUE clearValue;
	clearValue.Format = elementFormat;
	if (elementFormat == DXGI_FORMAT_D16_UNORM || elementFormat == DXGI_FORMAT_D24_UNORM_S8_UINT
		|| elementFormat == DXGI_FORMAT_D32_FLOAT || elementFormat == DXGI_FORMAT_D32_FLOAT_S8X24_UINT) {
		clearValue.DepthStencil.Depth = 1.0f;
		clearValue.DepthStencil.Stencil = 0;
	}
	else {
		clearValue.Color[0] = 0.0f; clearValue.Color[1] = 0.0f; clearValue.Color[2] = 0.0f; clearValue.Color[3] = 0.0f;
	}
	/** 申请构造纹理资源 */
	if (flags & (D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)) {
		if (FAILED(m_pAllocator->CreateResource(&allocDesc, &texDesc, state, &clearValue,
			&newResource.pAllocation, IID_PPV_ARGS(&newResource.pResource)))) {
			return { nullptr, D3D12_RESOURCE_STATE_COMMON };
		}
	}
	else {
		if (FAILED(m_pAllocator->CreateResource(&allocDesc, &texDesc, state, nullptr,
			&newResource.pAllocation, IID_PPV_ARGS(&newResource.pResource)))) {
			return { nullptr, D3D12_RESOURCE_STATE_COMMON };
		}
	}
	m_resourceRepository.insert(std::make_pair(newResource.pResource, newResource));
	return { newResource.pResource, state };
}

bool DX12ResourceManager::ReleaseResource(ID3D12Resource* pRes) {
	auto target = m_resourceRepository.find(pRes);
	if (target != m_resourceRepository.end()) {
		target->first->Release();
		target->second.pAllocation->Release();
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
