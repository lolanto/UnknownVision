#pragma once
#include <GraphicsInterface/Shader.h>
#include <GraphicsInterface/Pipeline.h>

namespace PlainModel {

    static const char* vertexShader = "\
        cbuffer MVP : register(b0) {\
        float4x4 model;\
        float4x4 view;\
        float4x4 projection;\
        };\
        \
        float4 main(float3 position : POSITION) : SV_POSITION{\
        return mul(projection,\
            mul(view, \
                mul(model, float4(position, 1.0f))\
            )\
        );\
        }";
    class PlainModelVS : public UnknownVision::VertexShader {
    public:
        static std::vector<UnknownVision::VertexAttribute> GetVertexAttributes() {
            return {
                UnknownVision::VertexAttribute(UnknownVision::VERTEX_ATTRIBUTE_TYPE_POSITION, UnknownVision::ELEMENT_FORMAT_TYPE_R32G32B32_FLOAT, 0, 0, 0),
            };
        }
    public:
        PlainModelVS() : VertexShader(vertexShader) {}
        virtual ~PlainModelVS() = default;
        virtual std::vector<std::vector<UnknownVision::ShaderParameterSlotDesc>> GetShaderParameters() const override final {
            return {
                {UnknownVision::ShaderParameterSlotDesc::OnlyReadBuffer(0, 1)}
            };
        }
        virtual const char* Name() const override final { return "PlainModelVS.hlsl"; }
    };

    static const char* pixelShader = "\
        float4 main(float4 position : SV_POSITION) : SV_TARGET{\
            return float4(1.0f, 1.0f, 1.0f, 1.0f);\
        }";

    class PlainModelPS : public UnknownVision::PixelShader {
    public:
        PlainModelPS() : PixelShader(pixelShader) {}
        virtual ~PlainModelPS() = default;
        virtual const char* Name() const override final { return "PlainModelPS.hlsl"; }
    };
};
