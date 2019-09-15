#pragma once
#include "FixedStage.h"
#include "Shader.h"

#include <assert.h>

BEG_NAME_SPACE

class CommandUnit;


struct GraphicsPipeline {
	void Reset();
	uint64_t Hash();
	RasterizeOptions rasOpt;
	OutputStageOptions outputOpt;
	ShaderInterface* vs;
	ShaderInterface* ps;
};

END_NAME_SPACE
