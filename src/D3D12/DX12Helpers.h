#pragma once
#include "DX12Config.h"
#include "../UVType.h"
#include "../Utility/InfoLog/InfoLog.h"
#include <cassert>

BEG_NAME_SPACE
/** Helping Functions */

inline DXGI_FORMAT ElementFormatToDXGIFormat(ElementFormatType format) {
	switch (format) {
	case ELEMENT_FORMAT_TYPE_R16_FLOAT:
		return DXGI_FORMAT_R16_FLOAT;
	case ELEMENT_FORMAT_TYPE_R32_FLOAT:
		return DXGI_FORMAT_R32_FLOAT;
	case ELEMENT_FORMAT_TYPE_R32G32_FLOAT:
		return DXGI_FORMAT_R32G32_FLOAT;
	case ELEMENT_FORMAT_TYPE_R32G32B32_FLOAT:
		return DXGI_FORMAT_R32G32B32_FLOAT;
	case ELEMENT_FORMAT_TYPE_R32G32B32A32_FLOAT:
		return DXGI_FORMAT_R32G32B32A32_FLOAT;
	case ELEMENT_FORMAT_TYPE_D24_UNORM_S8_UINT:
		return DXGI_FORMAT_D24_UNORM_S8_UINT;
	case ELEMENT_FORMAT_TYPE_R8G8B8A8_UNORM:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	default:
		MLOG("Invalid format conervsion!\n");
		return DXGI_FORMAT_UNKNOWN;
	}
}

inline const char* VertexAttributeTypeToString(VertexAttributeType type) {
	switch (type)
	{
	case UnknownVision::VERTEX_ATTRIBUTE_TYPE_POSITION:
		return "POSITION";
	case UnknownVision::VERTEX_ATTRIBUTE_TYPE_NORMAL:
		return "NORMAL";
	case UnknownVision::VERTEX_ATTRIBUTE_TYPE_TANGENT:
		return "TANGENT";
	case UnknownVision::VERTEX_ATTRIBUTE_TYPE_TEXTURE:
		return "TEXCOORD";
	default:
		MLOG("Invalid Vertex Attribute type!\n");
		return nullptr;
	}
}

inline D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTypeToPrimitiveTopologyType(PrimitiveType type) {
	switch (type)
	{
	case UnknownVision::PRIMITIVE_TYPE_POINT:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	case UnknownVision::PRIMITIVE_TYPE_TRIANGLE_LIST:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	default:
		FLOG("%s: Doesn't support this primitive type\n", __FUNCTION__);
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
	}
}

inline D3D12_FILL_MODE FillModeToDX12FillMode(FillMode mode) {
	switch (mode)
	{
	case UnknownVision::FILL_MODE_SOLID:
		return D3D12_FILL_MODE_SOLID;
	case UnknownVision::FILL_MODE_WIREFRAME:
		return D3D12_FILL_MODE_WIREFRAME;
	default:
		FLOG("%s: New DX Fill mode!\n", __FUNCTION__);
		assert(false);
	}
}

inline D3D12_CULL_MODE CullModeToCullMode(CullMode mode) {
	switch (mode)
	{
	case UnknownVision::CULL_MODE_BACK:
		return D3D12_CULL_MODE_BACK;
	case UnknownVision::CULL_MODE_FRONT:
		return D3D12_CULL_MODE_FRONT;
	case UnknownVision::CULL_MODE_NONE:
		return D3D12_CULL_MODE_NONE;
	default:
		FLOG("%s: New DX cull mode!\n", __FUNCTION__);
		assert(false);
	}
}

inline D3D12_HEAP_TYPE ResourceStatusToHeapType(const ResourceStatus& status) {
	if (status.isStably()) return D3D12_HEAP_TYPE_DEFAULT;
	else if (status.isReadBack()) return D3D12_HEAP_TYPE_READBACK;
	else
		return D3D12_HEAP_TYPE_UPLOAD;
}

inline D3D12_RESOURCE_FLAGS ResourceStatusToResourceFlag(const ResourceStatus& status) {
	D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE; /**< 默认flag类型 */
	if (status.canBeRenderTarget()) flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	if (status.canBeDepth()) flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	if (status.canBeUnorderAccess()) flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	/** TODO: 还有DenyShaderResource, crossAdapter, simultaneousAccess, videoDecodeReferenceOnly */
	return flags;
}

inline D3D12_FILTER FilterTypeToDX12FilterType(const FilterType& type) {
	switch (type)
	{
	case FILTER_TYPE_MIN_MAG_MIP_POINT:
		return D3D12_FILTER_MIN_MAG_MIP_POINT;
	case FILTER_TYPE_MIN_MAG_MIP_LINEAR:
		return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	case FILTER_TYPE_ANISOTROPIC:
		return D3D12_FILTER_ANISOTROPIC;
	default:
		FLOG("%s: Doesn't support this type of filter\n", __FUNCTION__);
		assert(false);
	}
}

inline D3D12_TEXTURE_ADDRESS_MODE SamplerAddressModeToDX12TextureAddressMode(const SamplerAddressMode& mode) {
	switch (mode)
	{
	case SAMPLER_ADDRESS_MODE_BORDER:
		return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	case SAMPLER_ADDRESS_MODE_CLAMP:
		return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	case SAMPLER_ADDRESS_MODE_WRAP:
		return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	default:
		FLOG("%s: Doesn't support this address mode\n", __FUNCTION__);
		assert(false);
	}
}

END_NAME_SPACE

