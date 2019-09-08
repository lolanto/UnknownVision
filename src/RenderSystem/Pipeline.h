#pragma once
#include "../UVType.h"
#include "Shader.h"

#include <assert.h>

BEG_NAME_SPACE

class CommandUnit;

class PipelineInterface {
public:
	PipelineHandle GetHandle() const { return m_handle; }
	ARR_ShaderParameterGroup GetParameterGroups() const {
		return m_parameterGroups;
	};
protected:
	PipelineHandle m_handle;
	ARR_ShaderParameterGroup m_parameterGroups;
};

template<typename VS, typename PS>
class Pipeline : public PipelineInterface {
public:
	using VertexShader = VS;
	using PixelShader = PS;
public:
	Pipeline() {
		ARR_ShaderParameterGroup vsGroup = VertexShader::GetParameterGroups();
		ARR_ShaderParameterGroup psGroup = PixelShader::GetParameterGroups();
		for (uint8_t i = 0; i < MAX_PARAMETER_GROUP; ++i) {
			m_parameterGroups[i].swap(vsGroup[i]);
			for (auto ps : psGroup[i].parameters) {
				auto& para = m_parameterGroups[i].parameters[ps.first];
				if (para.type == SHADER_PARAMETER_TYPE_INVALID) { para = ps.second; }
				else {
					assert(memcmp(&para, &ps.second, sizeof(ShaderParameter)) == 0);
				}
			}
		}
	}
private:
	VertexShader m_vs;
	PixelShader m_ps;
};

END_NAME_SPACE
