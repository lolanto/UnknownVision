#pragma once
#include "imgui_impl_uv.h"
#include "../RenderSystem/Shader.h"
#include "../RenderSystem/Pipeline.h"

BEG_NAME_SPACE

static const char* vertexShader =
"cbuffer vertexBuffer : register(b0) \
            {\
              float4x4 ProjectionMatrix; \
            };\
            struct VS_INPUT\
            {\
              float2 pos : POSITION;\
              float4 col : COLOR0;\
              float2 uv  : TEXCOORD0;\
            };\
            \
            struct PS_INPUT\
            {\
              float4 pos : SV_POSITION;\
              float4 col : COLOR0;\
              float2 uv  : TEXCOORD0;\
            };\
            \
            PS_INPUT main(VS_INPUT input)\
            {\
              PS_INPUT output;\
              output.pos = mul( ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));\
              output.col = input.col;\
              output.uv  = input.uv;\
              return output;\
            }";

class IMGUI_VERTEX_SHADER : public VertexShader {
public:
	static std::vector<VertexAttribute> GetVertexAttributes() {
		return {
			VertexAttribute(VERTEX_ATTRIBUTE_TYPE_POSITION, ELEMENT_FORMAT_TYPE_R32G32_FLOAT, 0, 0, (uint8_t)IM_OFFSETOF(ImDrawVert, pos)),
			VertexAttribute(VERTEX_ATTRIBUTE_TYPE_TEXTURE, ELEMENT_FORMAT_TYPE_R32G32_FLOAT, 0, 0, (uint8_t)IM_OFFSETOF(ImDrawVert, uv)),
			VertexAttribute(VERTEX_ATTRIBUTE_TYPE_COLOR, ELEMENT_FORMAT_TYPE_R8G8B8A8_UNORM, 0, 0, (uint8_t)IM_OFFSETOF(ImDrawVert, col))
		};
	}
public:
    IMGUI_VERTEX_SHADER() : VertexShader(vertexShader) {}
	virtual std::vector<std::vector<ShaderParameterSlotDesc>> GetShaderParameters() const override final {
		return {
			{ShaderParameterSlotDesc::OnlyReadBuffer(0, 1)}
		};
	}
    virtual const char* Name() const override final { return "imgui_vs"; }
};

static const char* pixelShader =
"struct PS_INPUT\
            {\
              float4 pos : SV_POSITION;\
              float4 col : COLOR0;\
              float2 uv  : TEXCOORD0;\
            };\
            SamplerState sampler0 : register(s0);\
            Texture2D texture0 : register(t0);\
            \
            float4 main(PS_INPUT input) : SV_Target\
            {\
              float4 out_col = input.col * texture0.Sample(sampler0, input.uv); \
              return out_col; \
            }";

class IMGUI_PIXEL_SHADER : public PixelShader {
public:
    IMGUI_PIXEL_SHADER() : PixelShader(pixelShader) {}
    virtual std::vector<std::vector<ShaderParameterSlotDesc>> GetShaderParameters() const override final {
        return {
            {ShaderParameterSlotDesc::OnlyReadTexture(0, 1)},
            {ShaderParameterSlotDesc::LinearSampler(0)}
        };
    }
    virtual const char* Name() const override final { return "imgui_ps"; }
};

END_NAME_SPACE

