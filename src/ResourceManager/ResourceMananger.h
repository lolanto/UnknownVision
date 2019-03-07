#ifndef UV_RESOURCE_MANAGER_H
#define UV_RESOURCE_MANAGER_H

#include "ResourceManangerConfig.h"
#include "../Resource/Shader.h"
#include <vector>

namespace UnknownVision {
	class Texture2D;
	class Buffer;
	class VertexDeclaration;
	struct PipelineStateDesc;

	class ResourceMgr {
	public:
		ResourceMgr(const ManagerType& type) : Type(type) {}
		virtual ~ResourceMgr() {}
		const ManagerType Type;
	};

	
	
	class VertexDeclarationMgr : public ResourceMgr {
	public:
		VertexDeclarationMgr() : ResourceMgr(MT_VERTEX_DECLARATION_MANAGER) {}
		virtual ~VertexDeclarationMgr() {}
	public:
		/// 根据顶点属性描述与具体的某个顶点着色器创建顶点属性描述对象
		/** 顶点属性描述对象是连接顶点缓冲区和顶点着色器之间的桥梁
		 * @param verAttDescs 是一系列顶点属性的描述队列，与某个顶点缓冲的结构
		 * 相对应
		 * @param shaderIndex 是要与输入的顶点属性相匹配的顶点着色器的索引编号
		 * @return 创建成功，返回创建的顶点属性描述对象的索引编号，创建失败返回-1
		 */
		virtual VertexDeclarationIdx CreateVertexDeclaration(const std::vector<SubVertexAttributeDesc>& verAttDescs, ShaderIdx shaderIndex) = 0;
		/** 通过|index|从管理器中索引顶点属性描述对象 */
		virtual VertexDeclaration& GetVertexDeclaration(VertexDeclarationIdx index) = 0;
	};

	class PipelineStateMgr : public ResourceMgr {
	public:
		PipelineStateMgr() : ResourceMgr(MT_PIPELINE_STATE_MANAGER) {}
		virtual ~PipelineStateMgr() {}
	public:
		/** 利用管线状态描述结构创建管线状态对象 
		 * @param desc 管线状态描述结构的引用，具体查看PipelineStateDesc的定义
		 * @return 创建成功，返回状态对象的索引，创建失败返回 -1 */
		virtual PipelineStateIdx CreatePipelineState(const PipelineStateDesc& desc) = 0;
		/** 根据输入的管线状态索引，返回其描述结构
		 * @param index 需要查询的管线状态的索引
		 * @param output 输出索引到的描述结构
		 * @return 若index没有对应的描述对象，则返回false，否则返回true
		 * @remark 对返回结构的修改不影响已创建的对象 */
		virtual bool GetDesc(PipelineStateIdx index, PipelineStateDesc& output) = 0;
	};

}

#endif // UV_RESOURCE_MANAGER_H
