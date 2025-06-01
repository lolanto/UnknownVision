#pragma once

#include <UVMarco.h>
#include <memory>
#include <vector>
#include <functional>

BEG_NAME_SPACE
class GPUBuffer;
class CommandUnit;
class IMeshResource;
class RenderDevice;
struct VertexAttribute;

struct VertexBuffersHolder
{
	size_t VertexCount = 0; /**< 顶点数量 */
	size_t IndexCount = 0; /**< 索引数量 */
	std::vector<std::unique_ptr<GPUBuffer>> VertexBuffers;
	std::unique_ptr<GPUBuffer> IndexBuffer;
	std::function<void(VertexBuffersHolder*, CommandUnit*)> BindingFunction;
};

/**
 * 辅助类，提供一系列静态函数以支持
 * 1. 向Shader注入指定顶点格式的Shader代码，协助Vertex Shader使用统一顶点属性访问模式
 * 2. Pipeline State Object创建时，提供必要的顶点缓冲属性声明
 * 3. 对输入的顶点属性缓冲进行格式转换，保证其能够转换成与自身声明的顶点属性格式一致的格式
 */
template<typename ConcreateVertexFactory>
class TVertexFactory
{
public:
	using VFType = ConcreateVertexFactory;
public:
	static const std::vector<VertexAttribute> GetVertexAttributes() { return VFType::GetVertexAttributes(); }
	constexpr static const char* GetVertexShaderDeclarationPlacementString() { return "/* #VERTEX_DECLARATIONS# */"; }
	constexpr static const char* GetVertexShaderDeclarationCode() { return VFType::GetVertexShaderDeclarationCode(); }
	static VertexBuffersHolder CreateVertexBuffersHolder(const IMeshResource* inputMesh, CommandUnit* cmdUnit, RenderDevice* device) {
		return VFType::CreateVertexBuffersHolder(inputMesh, cmdUnit, device); 
	}
};

class VertexFactory_PNT0 : public TVertexFactory<VertexFactory_PNT0>
{
public:
	static const std::vector<VertexAttribute> GetVertexAttributes();
	constexpr static const char* GetVertexShaderDeclarationCode()
	{
		return R"(
#ifndef VERTEX_INPUT_DECLARED
#define VERTEX_INPUT_DECLARED
struct VertexInput
{
	float3 Position : POSITION;
	float2 Texcoord : TEXCOORD0;
	float3 Normal : NORMAL;
};

VertexData GetVertexData(VertexInput input) {
    VertexData output;
    output.Position = input.Position;
    output.Texcoord = input.Texcoord;
    output.Normal = input.Normal;
    return output;
}
#endif // VERTEX_INPUT_DECLARED
)";
	}
	static VertexBuffersHolder CreateVertexBuffersHolder(const IMeshResource* inputMesh, CommandUnit* cmdUnit, RenderDevice* device);
public:
	static void BindingVertexBuffers(VertexBuffersHolder* holder, CommandUnit* cmdUnit);
};

END_NAME_SPACE
