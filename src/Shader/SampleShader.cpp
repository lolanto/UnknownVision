#include "SampleShader.h"
#include "../RenderSystem/Pipeline.h"

BEG_NAME_SPACE

void test() {
	Pipeline<SampleShaderVS, SampleShaderPS> samplePipe;
	GetGroup1(decltype(samplePipe)::PixelShader) k;
	k.rtv = RenderTargetIdx::InvalidIndex();
}

END_NAME_SPACE
