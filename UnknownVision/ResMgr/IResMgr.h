#ifndef IRESOURCE_MANAGER_H
#define IRESOURCE_MANAGER_H

#include "ResMgr_UVConfig.h"
#include <vector>

namespace UnknownVision {
	class Texture2D;
	class Buffer;
	class Shader;
	class VertexDeclaration;
	struct PipelineStateDesc;

	class ResMgr {
	public:
		ResMgr(const ManagerType& type) : Type(type) {}
		virtual ~ResMgr() {}
		const ManagerType Type;
	};

	class TextureMgr : public ResMgr {
	public:
		TextureMgr(const ManagerType& type) : ResMgr(type) {}
		virtual ~TextureMgr() {}
	};

	class Texture2DMgr : public TextureMgr {
	public:
		Texture2DMgr() : TextureMgr(MT_TEXTURE2D_MANAGER) {}
		virtual ~Texture2DMgr() {}
	public:
		/* 创建供vs, ps(fs)使用的纹理资源
		@param width, height
			设置纹理的长宽大小
		@parm flag
			设置这个纹理的特性，可选参数为枚举TextureFlag中的值(比如TF_READ_BY_GPU)，
			利用 | 可以连接多个特性。特性的种类直接决定了纹理的内存使用方案以及能否以此创建RenderTarget
		@param type
			纹理像素的类型，可选参数参考枚举值定义
		@param data
			创建纹理的初始图像数据，通常来自磁盘读取的图片数据
		@param bytePerLine
			当data非空时，需要设置此值告诉纹理管理器一行有多少字节，字节数量包含了padding需要的字节
		@ret
			创建成功，则返回创建的纹理的索引值，供下次从管理器索引纹理时使用。
			创建失败返回-1
		**/
		virtual Texture2DIdx CreateTexture(float width, float height,
			TextureFlagCombination flag, DataFormatType type, uint8_t* data, size_t bytePerLine) = 0;
		/* 从纹理出发创建渲染对象
		@param index
			需要创建RenderTarget的基础纹理的索引
		@ret
			创建成功，返回RenderTarget的索引值，供绑定RenderTarget时使用
			创建失败，返回-1
		**/
		virtual RenderTargetIdx CreateRenderTargetFromTexture(Texture2DIdx index) = 0;
		/* 从纹理为基础创建深度模板测试对象
		@param index
			需要创建深度模板测试对象的基础纹理索引
		@ret
			创建成功，返回深度模板测试对象的索引值，供绑定时使用
			创建失败，返回-1
		**/
		virtual DepthStencilIdx CreateDepthStencilFromTexture(Texture2DIdx index) = 0;
		/* 获取Texture2D，从而访问该贴图的基本属性
		@param index
			需要查询的纹理的索引
		@ret
			返回索引对应的纹理信息，可以通过该对象查询纹理的基本情况
		**/
		virtual const Texture2D& GetTexture(Texture2DIdx index) const = 0;
	};

	class BufferMgr : public ResMgr {
	public:
		BufferMgr() : ResMgr(MT_BUFFER_MANAGER) {}
		virtual ~BufferMgr() {}
	public:
		/** 创建顶点缓冲
		 * @param numVtx 缓冲中顶点的数量，设置后不能修改
		 * @param vtxSize 缓冲中一个顶点的字节大小
		 * @param data 顶点缓冲的初始数据
		 * @param flags 缓冲区的特性，详细内容见BufferFlag定义
		 * @return 创建成功，返回缓冲区的索引；创建失败返回-1 */
		virtual BufferIdx CreateVertexBuffer(size_t numVtxs, size_t vtxSize, uint8_t* data, BufferFlagCombination flags) = 0;
		/** 创建常量缓冲区 
		 * @param byteSize 常量缓冲区的大小
		 * @param data 用于初始化常量缓冲区的数据
		 * @param flags 缓冲区的额外设置
		 * @return 创建成功，返回索引；否则返回-1 */
		virtual BufferIdx CreateConstantBuffer(size_t byteSize, uint8_t* data, BufferFlagCombination flags) = 0;
		/** 创建索引缓存
		 * @param numIdxs 缓冲中索引的数量
		 * @param IdxSize 一个索引元素的大小
		 * @param data 用于初始化索引缓冲区的数据
		 * @param flags 缓冲区的额外设置
		 * @return 创建成功返回缓冲索引，否则返回-1 */
		virtual BufferIdx CreateIndexBuffer(size_t numIdxs, size_t IdxSize, uint8_t* data, BufferFlagCombination flags) = 0;
		virtual Buffer& GetBuffer(BufferIdx index) = 0;
	};

	class ShaderMgr : public ResMgr {
	public:
		ShaderMgr() : ResMgr(MT_SHADER_MANAGER) {}
		virtual ~ShaderMgr() {}
	public:
		/* 直接加载已经编译过的shader
		@param type
			需要创建的shader类型，具体类型参见ShaderType定义
		@param fileName
			shader所在的二进制文件路径
		@ret
			创建成功，返回shader在管理器中的索引值，供后续检索使用
			创建失败，返回-1
		**/
		virtual ShaderIdx CreateShaderFromBinaryFile(ShaderType type, const char* fileName) = 0;
		virtual const Shader& GetShader(ShaderIdx index) const = 0;
	};

	class VertexDeclarationMgr : public ResMgr {
	public:
		VertexDeclarationMgr() : ResMgr(MT_VERTEX_DECLARATION_MANAGER) {}
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

	class PipelineStateMgr : public ResMgr {
	public:
		PipelineStateMgr() : ResMgr(MT_PIPELINE_STATE_MANAGER) {}
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

#endif // IRESOURCE_MANAGER_H
