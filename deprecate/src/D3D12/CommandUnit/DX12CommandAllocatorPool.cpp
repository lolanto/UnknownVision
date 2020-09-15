//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard
//
// Modified By Yonghao Lu
//


#include "DX12CommandAllocatorPool.h"
#include <cassert>
#include <iostream>
BEG_NAME_SPACE

CommandAllocatorPool::CommandAllocatorPool(D3D12_COMMAND_LIST_TYPE Type) :
    m_cCommandListType(Type),
    m_Device(nullptr)
{
}

CommandAllocatorPool::~CommandAllocatorPool()
{
    Shutdown();
}

void CommandAllocatorPool::Create(ID3D12Device * pDevice)
{
    m_Device = pDevice;
}

void CommandAllocatorPool::Shutdown()
{
    m_AllocatorPool.clear();
    m_ReadyAllocators.swap(decltype(m_ReadyAllocators)());
}

ID3D12CommandAllocator * CommandAllocatorPool::RequestAllocator(uint64_t CompletedFenceValue)
{
    std::lock_guard<std::mutex> LockGuard(m_AllocatorMutex);

    ID3D12CommandAllocator* pAllocator = nullptr;

    if (!m_ReadyAllocators.empty())
    {
		/** 运行的重要前提是Discard的顺序恰好是从小到大 */
        std::pair<uint64_t, ID3D12CommandAllocator*>& AllocatorPair = m_ReadyAllocators.front();

        if (AllocatorPair.first <= CompletedFenceValue)
        {
            pAllocator = AllocatorPair.second;
            assert(SUCCEEDED((pAllocator->Reset())));
            m_ReadyAllocators.pop();
        }
    }

    // If no allocator's were ready to be reused, create a new one
    if (pAllocator == nullptr)
    {
		SmartPTR<ID3D12CommandAllocator> tmpAllocator;
        assert(SUCCEEDED(m_Device->CreateCommandAllocator(m_cCommandListType, IID_PPV_ARGS(&tmpAllocator))));
        wchar_t AllocatorName[32];
        swprintf(AllocatorName, 32, L"CommandAllocator %zu", m_AllocatorPool.size());
        tmpAllocator->SetName(AllocatorName);
		pAllocator = tmpAllocator.Get();
        m_AllocatorPool.push_back(std::move(tmpAllocator));
    }

    return pAllocator;
}

void CommandAllocatorPool::DiscardAllocator(uint64_t FenceValue, ID3D12CommandAllocator * Allocator)
{
    std::lock_guard<std::mutex> LockGuard(m_AllocatorMutex);

    // That fence value indicates we are free to reset the allocator
    m_ReadyAllocators.push(std::make_pair(FenceValue, Allocator));
}

END_NAME_SPACE
