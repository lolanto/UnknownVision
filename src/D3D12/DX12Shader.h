#pragma once
#include "DX12Config.h"
#include "../RenderSystem/Shader.h"
#include "../Utility/InfoLog/InfoLog.h"
#include <map>
#include <string>
BEG_NAME_SPACE

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

struct ShaderParameterInfo {
	D3D_SHADER_INPUT_TYPE type;
	uint8_t registerIndex;
	uint8_t spaceIndex;
	/** TODO: 这个方式似乎不支持数组类型的资源 */
	ShaderParameterInfo(D3D_SHADER_INPUT_TYPE type = D3D10_SIT_CBUFFER,
		uint8_t rId = 0, uint8_t sid = 0) : type(type), registerIndex(rId), spaceIndex(sid) {}
	ShaderParameterInfo(const ShaderParameterInfo& rhs) : type(rhs.type), registerIndex(rhs.registerIndex),
		spaceIndex(rhs.spaceIndex) {}
	ShaderParameterInfo(ShaderParameterInfo&& rhs) : ShaderParameterInfo(rhs) {}
};

/** 用于存放加载的shader字节码和reflect的属性 */
struct ShaderInfo {
	SmartPTR<ID3DBlob> blob;
	uint32_t version; /**< 包含该shader的版本信息，编码规则见https://docs.microsoft.com/zh-cn/windows/win32/api/d3d12shader/ns-d3d12shader-_d3d12_shader_desc */
	uint64_t timestamp;
	union {
		/** VS_IO记录了DX12中各个顶点属性是否被使用，使用的数量，编码规则为:
		 * 0-3: TEXCOORD; 4-7: TANGENT; 8-11: PSIZE; 12-15: POSITIONT; 16-19: POSITION
		 * 20-23: NORMAL; 24-27: COLOR; 28-31: BLENDWEIGHT; 32-35: BLENDINDICES; 36-39: BINORMAL*/
		uint64_t VS_IO;
		/** PS_IO记录了DX12中各个输出slot是否被使用，使用的数量，编码规则如下
		 * 0-3: SV_DEPTH; 4-7: SV_TARGET*/
		uint64_t PS_IO;
	};
	std::map<std::string, ShaderParameterInfo> signatures;
	ShaderInfo() : timestamp(0), version(0), VS_IO(0), PS_IO(0) {}
	uint8_t MajorVersion() const { return static_cast<uint8_t>((version & 0x000000F0) >> 4); }
	uint8_t MinorVersion() const { return static_cast<uint8_t>(version & 0x0000000F); }

};



END_NAME_SPACE
