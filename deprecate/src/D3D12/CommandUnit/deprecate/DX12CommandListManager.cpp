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

#include "DX12CommandListManager.h"
#include "../DX12RenderDevice.h"
#include <cassert>

BEG_NAME_SPACE

DX12CommandQueue::DX12CommandQueue(D3D12_COMMAND_LIST_TYPE Type) :
    m_Type(Type),
    m_CommandQueue(nullptr),
    m_pFence(nullptr),
    m_NextFenceValue((uint64_t)Type << 56 | 1),
    m_LastCompletedFenceValue((uint64_t)Type << 56),
    m_AllocatorPool(Type)
{
}

DX12CommandQueue::~DX12CommandQueue()
{
    Shutdown();
}

void DX12CommandQueue::Shutdown()
{
    if (m_CommandQueue == nullptr)
        return;

    m_AllocatorPool.Shutdown();
	m_CommandQueue.Reset();
	m_pFence.Reset();
    CloseHandle(m_FenceEventHandle);
}

DX12CommandListManager::DX12CommandListManager() :
    m_pDevice(nullptr),
    m_GraphicsQueue(D3D12_COMMAND_LIST_TYPE_DIRECT),
    m_ComputeQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE),
    m_CopyQueue(D3D12_COMMAND_LIST_TYPE_COPY)
{
}

DX12CommandListManager::~DX12CommandListManager()
{
    Shutdown();
}

void DX12CommandListManager::Shutdown()
{
    m_GraphicsQueue.Shutdown();
    m_ComputeQueue.Shutdown();
    m_CopyQueue.Shutdown();
}

void DX12CommandQueue::Create(DX12CommandListManager* pManager, SmartPTR<ID3D12CommandQueue> managedQueue)
{
	/** 创建Queue时必须要求整个Queue都是“空”的 */
    assert(pManager != nullptr);
    assert(!IsReady());
    assert(m_AllocatorPool.Size() == 0);

	m_pManager = pManager;
	ID3D12Device* pDevice = pManager->GetDevice()->GetDevice();

	if (managedQueue == nullptr) {
		D3D12_COMMAND_QUEUE_DESC QueueDesc = {};
		QueueDesc.Type = m_Type;
		QueueDesc.NodeMask = 1;
		pDevice->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(&m_CommandQueue));
	}
	else {
		m_CommandQueue = managedQueue;
	}
	m_CommandQueue->SetName(L"DX12CommandListManager::m_CommandQueue");

    assert(SUCCEEDED(pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence))));
    m_pFence->SetName(L"DX12CommandListManager::m_pFence");
    m_pFence->Signal((uint64_t)m_Type << 56);

    m_FenceEventHandle = CreateEvent(nullptr, false, false, nullptr);
    assert(m_FenceEventHandle != INVALID_HANDLE_VALUE);

    m_AllocatorPool.Create(pDevice);

    assert(IsReady());
}

void DX12CommandListManager::Create(DX12RenderDevice* pDevice,
	SmartPTR<ID3D12CommandQueue> managedGraphicsQueue,
	SmartPTR<ID3D12CommandQueue> managedComputeQueue,
	SmartPTR<ID3D12CommandQueue> managedCopyQueue)
{
    assert(pDevice != nullptr);

    m_pDevice = pDevice;

    m_GraphicsQueue.Create(this, managedGraphicsQueue);
    m_ComputeQueue.Create(this, managedComputeQueue);
    m_CopyQueue.Create(this, managedCopyQueue);
}

void DX12CommandListManager::CreateNewCommandList(
	D3D12_COMMAND_LIST_TYPE Type,
	SmartPTR<ID3D12GraphicsCommandList>& list,
	ID3D12CommandAllocator** Allocator )
{
	assert(Type != D3D12_COMMAND_LIST_TYPE_BUNDLE); /**< Bundles are not yet supported */
    switch (Type)
    {
    case D3D12_COMMAND_LIST_TYPE_DIRECT: *Allocator = m_GraphicsQueue.RequestAllocator(); break;
    case D3D12_COMMAND_LIST_TYPE_BUNDLE: break;
    case D3D12_COMMAND_LIST_TYPE_COMPUTE: *Allocator = m_ComputeQueue.RequestAllocator(); break;
    case D3D12_COMMAND_LIST_TYPE_COPY: *Allocator = m_CopyQueue.RequestAllocator(); break;
    }
    
    assert(SUCCEEDED( m_pDevice->GetDevice()->CreateCommandList(1, Type, *Allocator, nullptr, IID_PPV_ARGS(&list)) ));
    list->SetName(L"CommandList");
}

uint64_t DX12CommandQueue::ExecuteCommandList( ID3D12CommandList* List )
{
    std::lock_guard<std::mutex> LockGuard(m_FenceMutex);

    assert(SUCCEEDED(((ID3D12GraphicsCommandList*)List)->Close()));

    // Kickoff the command list
    m_CommandQueue->ExecuteCommandLists(1, &List);

    // Signal the next fence value (with the GPU)
    m_CommandQueue->Signal(m_pFence.Get(), m_NextFenceValue);

    // And increment the fence value.  
    return m_NextFenceValue++;
}

uint64_t DX12CommandQueue::IncrementFence(void)
{
    std::lock_guard<std::mutex> LockGuard(m_FenceMutex);
    m_CommandQueue->Signal(m_pFence.Get(), m_NextFenceValue);
    return m_NextFenceValue++;
}

bool DX12CommandQueue::IsFenceComplete(uint64_t FenceValue)
{
    // Avoid querying the fence value by testing against the last one seen.
    // The max() is to protect against an unlikely race condition that could cause the last
    // completed fence value to regress.
	/** 这里没有加锁，lastCompletedFenceValue的值确实会因为竞争而出错，
	 * 但由于max每次都将GetCompletedValue纳入考虑范围，所以比较结果应该在大部分情况下
	 * 是正确的，但没办法保证写入的lastCompletedFenceValue是否正确 */
	if (FenceValue > m_LastCompletedFenceValue) {
		uint64_t k = m_pFence->GetCompletedValue();
		m_LastCompletedFenceValue = std::max(m_LastCompletedFenceValue, m_pFence->GetCompletedValue());
	}

    return FenceValue <= m_LastCompletedFenceValue;
}

void DX12CommandQueue::StallForFence(uint64_t FenceValue)
{
    //DX12CommandQueue& Producer = Graphics::g_CommandManager.GetQueue((D3D12_COMMAND_LIST_TYPE)(FenceValue >> 56));
	DX12CommandQueue& producer = m_pManager->GetQueue((D3D12_COMMAND_LIST_TYPE)(FenceValue >> 56));
	m_CommandQueue->Wait(producer.m_pFence.Get(), FenceValue);
}

void DX12CommandQueue::StallForProducer(DX12CommandQueue& Producer)
{
    assert(Producer.m_NextFenceValue > 0);
    m_CommandQueue->Wait(Producer.m_pFence.Get(), Producer.m_NextFenceValue - 1);
}

void DX12CommandQueue::WaitForFence(uint64_t FenceValue)
{
    // TODO:  Think about how this might affect a multi-threaded situation.  Suppose thread A
   // wants to wait for fence 100, then thread B comes along and wants to wait for 99.  If
   // the fence can only have one event set on completion, then thread B has to wait for 
   // 100 before it knows 99 is ready.  Maybe insert sequential events?
    std::lock_guard<std::mutex> LockGuard(m_EventMutex);
    if (IsFenceComplete(FenceValue))
        return;

	m_pFence->SetEventOnCompletion(FenceValue, m_FenceEventHandle);
	WaitForSingleObject(m_FenceEventHandle, INFINITE);
	m_LastCompletedFenceValue = FenceValue;
}

void DX12CommandListManager::WaitForFence(uint64_t FenceValue)
{
    DX12CommandQueue& Producer = GetQueue((D3D12_COMMAND_LIST_TYPE)(FenceValue >> 56));
    Producer.WaitForFence(FenceValue);
}

ID3D12CommandAllocator* DX12CommandQueue::RequestAllocator()
{
    uint64_t CompletedFence = m_pFence->GetCompletedValue();

    return m_AllocatorPool.RequestAllocator(CompletedFence);
}

void DX12CommandQueue::DiscardAllocator(uint64_t FenceValue, ID3D12CommandAllocator* Allocator)
{
    m_AllocatorPool.DiscardAllocator(FenceValue, Allocator);
}

END_NAME_SPACE
