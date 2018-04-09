#include "../PS_INPUT.hlsli"

#ifndef LINKED_LIST
#define LINKED_LIST

#define LINKED_LIST_U0     u1
#define LINKED_LIST_U1     u2
#define LINKED_LIST_U2     u3
#define LINKED_LIST_U3     u4

#define LINKED_LIST_B0     b1

// 为每个屏幕像素创建链表
// 每个表内记录了应该被绘制在该位置的最后N个像素信息
// N为列表的长度

RWStructuredBuffer<int4>	RWStructuredCounter : register(LINKED_LIST_U0);
RWTexture2D<int>	tRWFragmentListHead		      : register(LINKED_LIST_U1);
RWTexture2D<unorm float4>	tRWFragmentColor		: register(LINKED_LIST_U2);
RWTexture2D<int2>	tRWDepthAndLink			        : register(LINKED_LIST_U3);

cbuffer LinkedListData						: register(LINKED_LIST_B0) {
	float4 ScreenWidthHeightStorageSlice;
}

// 输入节点地址，返回节点在缓冲区中的索引(uv)
int2 GetAddress(int addr);

int2 GetAddress(int addr) {
  int2 re;
  re.x = int(ScreenWidthHeightStorageSlice.x);
  re.y = addr / re.x;
  re.x = addr % re.x;
  return re;
}

void BuildLinkedList(
	float2 uv, // 屏幕的UV坐标
	float4 color, // 最终输出的颜色值
	float depth // 当前像素的摄像机空间的深度值
	) {

	// int2 ScreenSpaceAddress = int2(uv * GCameraParam.zw);
	int nNewFragmentAddress = RWStructuredCounter.IncrementCounter();
	// 假如当前超出容量
	if (nNewFragmentAddress == int(ScreenWidthHeightStorageSlice.z)) return;

	// update head buffer
  // int nOldFragmentAddress = 0;
	// int nOldFragmentAddress = tRWFragmentListHead[ScreenSpaceAddress];
 //  tRWFragmentListHead[ScreenSpaceAddress] = nNewFragmentAddress;
	// InterlockedExchange(tRWFragmentListHead[ScreenSpaceAddress],
	// 	nNewFragmentAddress, nOldFragmentAddress);

	// write the node attribute to the buffer
	int2 vAddress = GetAddress(nNewFragmentAddress);
  tRWFragmentColor[vAddress] = color;
  // tRWDepthAndLink[vAddress] = int2(int(
  //   depth * 100.0f),
  //   nOldFragmentAddress);

  return;
}

#endif