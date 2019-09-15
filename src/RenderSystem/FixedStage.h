﻿#pragma once
#include "../UVType.h"

BEG_NAME_SPACE

struct RasterizeOptions {
	CullMode cullMode = CULL_MODE_BACK;
	FillMode fillMode = FILL_MODE_SOLID;
	bool counterClockWiseIsFront = false;
	PrimitiveType primitive = PRIMITIVE_TYPE_TRIANGLE_LIST;
	RasterizeOptions() : cullMode(CULL_MODE_BACK), fillMode(FILL_MODE_SOLID),
		counterClockWiseIsFront(false), primitive(PRIMITIVE_TYPE_TRIANGLE_LIST) {}
};

/** 模板测试，深度测试，混合等输出过程的操作设置 */
struct OutputStageOptions {
	bool enableDepthTest = true;
	BlendOption blending = BLEND_OPTION_NO_BLEND;
	ElementFormatType dsvFormat = ELEMENT_FORMAT_TYPE_D24_UNORM_S8_UINT; /**< 深度模板缓冲的格式 */
	ElementFormatType rtvFormats[MAX_RENDER_TARGET]; /**< 各个RenderTarget的顶点格式 */
	OutputStageOptions() : enableDepthTest(true), blending(BLEND_OPTION_NO_BLEND),
		dsvFormat(ELEMENT_FORMAT_TYPE_D24_UNORM_S8_UINT) {
		for (auto& format : rtvFormats) {
			format = ELEMENT_FORMAT_TYPE_INVALID;
		}
	}
};

END_NAME_SPACE