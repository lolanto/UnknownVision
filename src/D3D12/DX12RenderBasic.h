#pragma once

#include "../RenderSystem/RenderBasic.h"
#include "DX12Config.h"
#include "DX12Helpers.h"
#include "DX12ResourceManager.h"
#include <vector>
#include <memory>
#include <array>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <map>
#include <optional>

#define MemoryManagementStrategy NoMemMng

BEG_NAME_SPACE

class DX12RenderBackend;
class DX12RenderDevice;

/** 单个句柄对应的实体的信息 */
struct BufferInfo {
	BufferInfo(size_t size = 0, ID3D12Resource* ptr = nullptr,
		D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON)
		: ptr(ptr), state(state), size(size) {}
	ID3D12Resource* ptr = nullptr;
	D3D12_RESOURCE_STATES state;
	const size_t size;
	union
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC constBufViewDesc;
		D3D12_VERTEX_BUFFER_VIEW vtxBufView;
		D3D12_INDEX_BUFFER_VIEW idxBufView;
	};
};

struct TextureInfo {
	TextureInfo(uint32_t width = 0, uint32_t height = 0,
		ID3D12Resource* ptr = nullptr, D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON)
		: ptr(ptr), state(state), width(width), height(height), 
		/** 让RTVCode最初索引到“空闲位”，表明当前RTV尚未创建 */
		renderTargetViewCode(NUMBER_OF_DESCRIPTOR_IN_EACH_DESCRIPTOR_HEAP[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]),
		depthStencilViewCode(NUMBER_OF_DESCRIPTOR_IN_EACH_DESCRIPTOR_HEAP[D3D12_DESCRIPTOR_HEAP_TYPE_DSV]) {}
	ID3D12Resource* ptr = nullptr;
	D3D12_RESOURCE_STATES state;
	const uint32_t width = 0, height = 0;
	D3D12_SHADER_RESOURCE_VIEW_DESC shaderResViewDesc;
	uint64_t renderTargetViewCode;
	uint64_t depthStencilViewCode;
	static uint32_t DecodeRTVCodeGen(uint64_t rtvCode) {
		return (rtvCode & 0xffffffff00000000u) >> 32;
	}
	static uint32_t DecodeRTVCodeIndex(uint64_t rtvCode) {
		return rtvCode & 0x00000000ffffffffu;
	}
	static uint64_t EncodeRTVCodeGen(uint64_t& rtvCode, uint64_t gen) {
		rtvCode &= 0x00000000ffffffffu;
		rtvCode |= (gen << 32);
		return rtvCode;
	}
	static uint64_t EncodeRTVCodeIndex(uint64_t& rtvCode, uint32_t index) {
		rtvCode &= 0xffffffff00000000u;
		rtvCode |= index;
		return rtvCode;
	}

	static uint32_t DecodeDSVCodeGen(uint64_t dsvCode) {
		return (dsvCode & 0xffffffff00000000u) >> 32;
	}
	static uint32_t DecodeDSVCodeIndex(uint64_t dsvCode) {
		return dsvCode & 0x00000000ffffffffu;
	}
	static uint64_t EncodeDSVCodeGen(uint64_t& dsvCode, uint64_t gen) {
		dsvCode &= 0x00000000ffffffffu;
		dsvCode |= (gen << 32);
		return dsvCode;
	}
	static uint64_t EncodeDSVCodeIndex(uint64_t& dsvCode, uint32_t index) {
		dsvCode &= 0xffffffff00000000u;
		dsvCode |= index;
		return dsvCode;
	}
};

struct RootSignatureInfo {
	SmartPTR<ID3D12RootSignature> rootSignature;
	/** parameter列表中记录了各个descriptor / constant / table的情况
	 * 连续的table之间使用invalidParamter进行分隔 */
	std::vector<RootSignatureParameter> parameters;
	/** functions */
	/** 向rootSignature增加新的参数值
	 * @param newParameter 新的参数值，可能是单个parameter，也可能是一个group
	 * @return 返回当前parameter在parameters中的下标，或者说添加的是第几个参数*/
	uint32_t SetParameter(const std::vector<RootSignatureParameter>& newParameter);
	/** 基于当前parameters的设计创建一个rootsignature，返回创建结果 */
	bool Build();
	/** QueryAbility */
	auto QueryWithParameter(const std::vector<RootSignatureParameter>& qury)
		->std::vector<std::optional<RootSignatureQueryAnswer> >;
};

struct ProgramInfo {
	/** 资源类型的编码方式，只有4位有效 */
	enum ResourceType : uint8_t {
		RESOURCE_TYPE_SHADER_RESOURCE_VIEW = 0x00u,
		RESOURCE_TYPE_CONSTANT_BUFFER_VIEW = 0x01u,
		RESOURCE_TYPE_UNORDER_ACCESS_VIEW = 0x02u,
		RESOURCE_TYPE_SAMPLER_STATE = 0x03
	};
	const std::vector<D3D12_INPUT_ELEMENT_DESC>* inputLayoutPtr;
	std::map<std::string, uint32_t> resNameToEncodingValue; /**< 资源名称和资源type, space, register编码 */
	std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers; /**< 静态sampler state */
	SmartPTR<ID3D12PipelineState> pso;
	SmartPTR<ID3D12RootSignature> rootSignature;
	/** Helper Functions */
	static void EncodeHelper(uint32_t& idx, uint8_t offset, uint32_t mask, uint8_t value) thread_safe {
		mask = mask << offset;
		mask = ~mask;
		idx &= mask;
		mask = value;
		mask = mask << offset;
		idx |= mask;
	}
	/** 向idx编码寄存器索引value */
	static void EncodeReigsterIndex(uint32_t& idx, uint8_t value) thread_safe {
		EncodeHelper(idx, 12, 0xFFu, value);
	}
	/** 向idx编码空间索引value */
	static void EncodeSpaceIndex(uint32_t& idx, uint8_t value) thread_safe {
		EncodeHelper(idx, 20, 0xFFu, value);
	}
	/** 向idx编码类型编号 SRV: 0x00, CBV: 0x01, UAV: 0x02 */
	static void EncodeType(uint32_t& idx, ResourceType type) thread_safe {
		EncodeHelper(idx, 28, 0x0Fu, type);
	}
	static uint8_t DecodeRegisterIndex(const uint32_t& idx) thread_safe { return (idx & 0x000FF000u) >> 12; }
	static uint8_t DecodeSpaceIndex(const uint32_t& idx) thread_safe { return (idx & 0x0FF00000u) >> 20; }

	static D3D12_DESCRIPTOR_RANGE_TYPE DecodeTypeToDescriptorRangeType(const uint32_t& idx) thread_safe {
		switch ((idx & 0xF0000000u) >> 28) {
		case RESOURCE_TYPE_SHADER_RESOURCE_VIEW: return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		case RESOURCE_TYPE_CONSTANT_BUFFER_VIEW: return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		case RESOURCE_TYPE_UNORDER_ACCESS_VIEW: return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		case RESOURCE_TYPE_SAMPLER_STATE: return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
		default:
			FLOG("%s: Invalid encode method! Unable to decode the type!\n", __FUNCTION__);
			assert(false);
		}
	}
	static uint32_t DecodeIndex(const uint32_t& idx) thread_safe { return idx & 0x00000FFFu; }
};



/** 以4位为单位，对value中的某4个位进行解码
 * @param value 被解码的值
 * @param pos 需要解码的4个位的位置，比如pos = 0 代表读取低位的0~3位，1代表设置低位的4~7位 */
inline uint8_t bit4Decode(const uint64_t& value, uint8_t pos) thread_safe {
	return static_cast<uint8_t>((value >> (pos * 4)) & 0x000000000000000Fu);
}

/** 以4位为单位，对value中的某4个位进行设置
 * @param value 被设置的值
 * @param pos 需要设置的4个位的位置，比如pos = 0 代表设置低位的0~3位，1代表设置低位的4~7位
 * @param newValue 需要替换的新值
 * @remark 虽然newValue类型位uint8_t，但实际上只有低四位有效 */
inline void bit4Encode(uint64_t& value, uint8_t pos, uint8_t newValue) thread_safe {
	uint64_t t1 = 0x0Fu;
	t1 = t1 << (pos * 4);
	t1 = ~t1;
	value &= t1;
#ifdef _DEBUG
	if (newValue & 0xF0u) FLOG("%s: high 4 bits are not zero and will be ignore\n", __FUNCTION__);
#endif // _DEBUG
	t1 = newValue & 0x0Fu;
	t1 = t1 << (pos * 4);
	value |= t1;
}

#define getTEXCOORDN(VS_IO) bit4Decode(VS_IO, 0)
#define setTEXCOORDN(VS_IO, value) bit4Encode(VS_IO, 0, value)
#define getTANGENTN(VS_IO) bit4Decode(VS_IO, 1)
#define setTANGENTN(VS_IO, value) bit4Encode(VS_IO, 1, value)
#define getPSIZEN(VS_IO) bit4Decode(VS_IO, 2)
#define setPSIZEN(VS_IO, value) bit4Encode(VS_IO, 2, value)
#define getPOSITIONTN(VS_IO) bit4Decode(VS_IO, 3)
#define setPOSITIONTN(VS_IO, value) bit4Encode(VS_IO, 3, value)
#define getPOSITIONN(VS_IO) bit4Decode(VS_IO, 4)
#define setPOSITIONN(VS_IO, value) bit4Encode(VS_IO, 4, value)
#define getNORMALN(VS_IO) bit4Decode(VS_IO, 5)
#define setNORMALN(VS_IO, value) bit4Encode(VS_IO, 5, value)
#define getCOLORN(VS_IO) bit4Decode(VS_IO, 6)
#define setCOLORN(VS_IO, value) bit4Encode(VS_IO, 6, value)
#define getBLENDWEIGHTN(VS_IO) bit4Decode(VS_IO, 7)
#define setBLENDWEIGHTN(VS_IO, value) bit4Encode(VS_IO, 7, value)
#define getBLENDINDICESN(VS_IO) bit4Decode(VS_IO, 8)
#define setBLENDINDICESN(VS_IO, value) bit4Encode(VS_IO, 8, value)
#define getBINORMALN(VS_IO) bit4Decode(VS_IO, 9)
#define setBINORMALN(VS_IO, value) bit4Encode(VS_IO, 9, value)

#define getSV_DEPTH(PS_IO) bit4Decode(PS_IO, 0)
#define setSV_DEPTH(PS_IO, value) bit4Encode(PS_IO, 0, value)
#define getSV_TARGET(PS_IO) bit4Decode(PS_IO, 1)
#define setSV_TARGET(PS_IO, value) bit4Encode(PS_IO, 1, value)

/** 分析a是否“过度匹配”b */
inline bool VS_IO_ExceedMatch(uint64_t VS_IO_a, uint64_t VS_IO_b) thread_safe {
	for (int i = 0; i < 10; ++i) {
		if ((VS_IO_a & 0x0Fu) < (VS_IO_b & 0x0Fu)) return false;
		VS_IO_a = VS_IO_a >> 4;
		VS_IO_b = VS_IO_b >> 4;
	}
	return true;
}

END_NAME_SPACE
