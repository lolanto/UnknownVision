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

#pragma once

#include "DX12CommandAllocatorPool.h"
#include "../DX12Config.h"
#include <vector>
#include <queue>
#include <mutex>
#include <stdint.h>
BEG_NAME_SPACE
class DX12RenderDevice;

class DX12CommandQueue
{
    friend class DX12CommandListManager;

public:
    DX12CommandQueue(D3D12_COMMAND_LIST_TYPE Type);
    ~DX12CommandQueue();
	void Create(DX12CommandListManager* pManager, SmartPTR<ID3D12CommandQueue> managedQueue = nullptr);
    void Shutdown();

    inline bool IsReady()
    {
        return m_CommandQueue != nullptr;
    }
	/** 纯粹增加当前Fence的值，从而获得完成当前Queue中所有提交
	 * 指令后的Fence值 */
    uint64_t IncrementFence(void);
    bool IsFenceComplete(uint64_t FenceValue);
	/** 阻塞Queue的执行，直到指定的某个已提交的List执行完成
	 * @param FenceValue List提交时对应的Fence值
	 * @remark 由于FenceValue的前8位含有CommandListType的类型编码
	 * 所以该函数事实可以等待任意Queue的任意一次List提交指令 */
    void StallForFence(uint64_t FenceValue);
	/** 阻塞Queue的执行，直到某个Queue当前已提交的所有指令执行完成
	 * @param Producer 需要被等待的Queue */
    void StallForProducer(DX12CommandQueue& Producer);
	/** 阻塞调用线程，直到指定的某个已提交的List执行完成 */
    void WaitForFence(uint64_t FenceValue);
    void WaitForIdle(void) { WaitForFence(IncrementFence()); }

    ID3D12CommandQueue* GetCommandQueue() { return m_CommandQueue.Get(); }
	/** 下一个指令序列执行的Fence值，目前正在执行的则是减一 */
    uint64_t GetNextFenceValue() { return m_NextFenceValue; }
	uint64_t GetCurFenceValue() { return m_NextFenceValue - 1; }

	uint64_t ExecuteCommandList(ID3D12CommandList* List);
	ID3D12CommandAllocator* RequestAllocator(void);
	void DiscardAllocator(uint64_t FenceValueForReset, ID3D12CommandAllocator* Allocator);

private:

	DX12CommandListManager* m_pManager;
	SmartPTR<ID3D12CommandQueue> m_CommandQueue;

    const D3D12_COMMAND_LIST_TYPE m_Type;

    CommandAllocatorPool m_AllocatorPool;
    std::mutex m_FenceMutex;
    std::mutex m_EventMutex;

    // Lifetime of these objects is managed by the descriptor cache
	SmartPTR<ID3D12Fence> m_pFence;
	uint64_t m_NextFenceValue;
	uint64_t m_LastCompletedFenceValue;
	HANDLE m_FenceEventHandle;

};

class DX12CommandListManager
{
    friend class DX12CommandUnit;

public:
    DX12CommandListManager();
    ~DX12CommandListManager();

    void Create(DX12RenderDevice* pDevice,
		SmartPTR<ID3D12CommandQueue> managedGraphicsQueue = nullptr,
		SmartPTR<ID3D12CommandQueue> managedComputeQueue = nullptr,
		SmartPTR<ID3D12CommandQueue> managedCopyQueue = nullptr);
    void Shutdown();
	DX12RenderDevice* GetDevice() const { return m_pDevice; }
    DX12CommandQueue& GetGraphicsQueue(void) { return m_GraphicsQueue; }
    DX12CommandQueue& GetComputeQueue(void) { return m_ComputeQueue; }
    DX12CommandQueue& GetCopyQueue(void) { return m_CopyQueue; }

    DX12CommandQueue& GetQueue(D3D12_COMMAND_LIST_TYPE Type = D3D12_COMMAND_LIST_TYPE_DIRECT)
    {
        switch (Type)
        {
        case D3D12_COMMAND_LIST_TYPE_COMPUTE: return m_ComputeQueue;
        case D3D12_COMMAND_LIST_TYPE_COPY: return m_CopyQueue;
        default: return m_GraphicsQueue;
        }
    }

    ID3D12CommandQueue* GetCommandQueue()
    {
        return m_GraphicsQueue.GetCommandQueue();
    }

    void CreateNewCommandList(
        D3D12_COMMAND_LIST_TYPE Type,
        SmartPTR<ID3D12GraphicsCommandList>& list,
        ID3D12CommandAllocator** Allocator);

    // Test to see if a fence has already been reached
    bool IsFenceComplete(uint64_t FenceValue)
    {
        return GetQueue(D3D12_COMMAND_LIST_TYPE(FenceValue >> 56)).IsFenceComplete(FenceValue);
    }

    // The CPU will wait for a fence to reach a specified value
    void WaitForFence(uint64_t FenceValue);

    // The CPU will wait for all command queues to empty (so that the GPU is idle)
    void IdleGPU(void)
    {
        m_GraphicsQueue.WaitForIdle();
        m_ComputeQueue.WaitForIdle();
        m_CopyQueue.WaitForIdle();
    }

private:

	DX12RenderDevice* m_pDevice;
    DX12CommandQueue m_GraphicsQueue;
    DX12CommandQueue m_ComputeQueue;
    DX12CommandQueue m_CopyQueue;
};
END_NAME_SPACE
