#pragma once
#include "RenderDescriptor.h"

BEG_NAME_SPACE

VertexAttributeDescs VertexAttribute_PosNorTex() {
	static VertexAttributeDescs descs;
	/** 防止被多次初始化 */
	if (descs.size()) return descs;
	else {
		descs.resize(3);
		auto iterator = descs.begin();
		/** 位置信息 */
		descs.emplace(iterator++, VertexAttribute(VERTEX_ATTRIBUTE_TYPE_POSITION,
			ELEMENT_FORMAT_TYPE_R32G32B32_FLOAT, 0));
		descs.emplace(iterator++, VertexAttribute(VERTEX_ATTRIBUTE_TYPE_NORMAL,
			ELEMENT_FORMAT_TYPE_R32G32B32_FLOAT, 0));
		descs.emplace(iterator++, VertexAttribute(VERTEX_ATTRIBUTE_TYPE_TEXTURE,
			ELEMENT_FORMAT_TYPE_R32G32_FLOAT, 0));
	}
}

END_NAME_SPACE