#ifndef D3D11_RESOURCE_MANAGER_H
#define D3D11_RESOURCE_MANAGER_H

#include "../../ResMgr/IResMgr.h"
#include "DX11Texture.h"
#include "DX11Shader.h"
#include "DX11Buffer.h"
#include "DX11VertexDeclaration.h"
#include <vector>
#include <map>
#include <cassert>
namespace UnknownVision {
	class DX11_Texture2DMgr : public Texture2DMgr {
	public:
		DX11_Texture2DMgr() {};
		virtual ~DX11_Texture2DMgr() {}
	public:
		Texture2DIdx CreateTexture(float width, float height,
			TextureFlagCombination flag, TextureElementType type, uint8_t* data, size_t bytePerLine); /**< 继承自Texture2DMgr */
		RenderTargetIdx CreateRenderTargetFromTexture(Texture2DIdx index);/**< 继承自Texture2DMgr */
		DepthStencilIdx CreateDepthStencilFromTexture(Texture2DIdx index);/**< 继承自Texture2DMgr */
		const Texture2D& GetTexture(Texture2DIdx index) const { /**< 继承自Texture2DMgr */
			assert(static_cast<uint32_t>(index.value()) < m_texs.size() && index >= 0);
			return m_texs[index.value()];
		}
		/** DX11 Texture2D的派生实现，获取DX11的RenderTargetView
		 * 重载一
		 * @param idx(Texture2DIdx) 需要获取RenderTargetView的纹理ID
		 * 重载二
		 * @param idx(RenderTargetIdx) RenderTargetView本身的ID
		 * @return 一旦输入的id没有对应的RenderTargetView则返回nullptr
		 */
		ID3D11RenderTargetView* GetRenderTargetFromTexture(Texture2DIdx idx); /// (1)
		ID3D11RenderTargetView* GetRenderTargetFromTexture(RenderTargetIdx idx); /// (2)

		/** DX11 Texture2D的派生实现，获取DX11的DepthStencilView
		 * 重载一
		 * @param idx(Texture2DIdx) 需要获取DepthstencilView的纹理ID
		 * 重载二
		 * @param idx(DepthStencilIdx) DepthStencilView本身的ID
		 * @return 一旦输入的id没有对应的DepthStencilView则返回nullptr
		 */
		ID3D11DepthStencilView* GetDepthStencilFromTexture(Texture2DIdx idx); /// (1)
		ID3D11DepthStencilView* GetDepthStencilFromTexture(DepthStencilIdx idx); /// (2)
	private:
		std::vector<DX11_Texture2D> m_texs; /**< 管理器管理的纹理对象的队列 */
		std::map<Texture2DIdx, RenderTargetIdx> m_texId2RTId; /**< 记录渲染对象索引和纹理对象索引之间的对应关系 */
		std::map<Texture2DIdx, DepthStencilIdx> m_texId2DSId; /**< 记录深度模板对象索引和纹理对象索引之间的对应关系 */
		std::vector<SmartPTR<ID3D11RenderTargetView>> m_rtvs; /**< DX11特有成员属性，记录渲染对象的列表 */
		std::vector<SmartPTR<ID3D11DepthStencilView>> m_dsvs; /**< DX11特有成员属性，记录深度模板对象的列表 */
	};

	class DX11_ShaderMgr : public ShaderMgr {
	public:
		DX11_ShaderMgr() {}
		virtual ~DX11_ShaderMgr() {}
	public:
		/// 从文件系统中加载已经编译完成的Shader文件
		/** 该函数负责读取二进制Shader文件并创建相应的Shader对象
		 * @param type Shader的类型，具体查看ShaderType定义
		 * @param fileName Shader文件的路径
		 * @return 加载成功，返回Shader对象在管理器中的索引编号
		 * 加载失败返回-1
		 */
		ShaderIdx CreateShaderFromBinaryFile(ShaderType type, const char* fileName);
		/** 输入Shader索引编号，从管理器中索引Shader对象 */
		const Shader& GetShader(ShaderIdx index) const {
			assert(static_cast<uint32_t>(index.value()) < m_shaders.size() && index >= 0);
			return m_shaders[index.value()];
		}
	private:
		std::vector<DX11_Shader> m_shaders; /**< 管理器管理的Shader队列 */
	};

	class DX11_BufferMgr : public BufferMgr {
	public:
		DX11_BufferMgr() {}
		virtual ~DX11_BufferMgr() {}
	public:
		/// 创建DX11需要的顶点缓冲区
		/** 根据BufferMgr的要求实现创建顶点缓冲区的接口
		 * @param numVtxs 顶点缓冲区中包含的顶点数量
		 * @param vtxSize 顶点缓冲中一个顶点数据的字节大小
		 * @param data 指向记录顶点缓冲区初始数据的内存区域，在缓冲区非只读的情况下可以为空
		 * @param flags 缓冲区的特性，具体查看BufferFlag定义
		 * @return 创建成功时返回缓冲区的索引，以供从管理器中检索缓冲区；创建失败返回-1
		 */
		BufferIdx CreateVertexBuffer(size_t numVtxs, size_t vtxSize, uint8_t* data, BufferFlagCombination flags);
		/// 创建DX11需要的常量缓冲区
		/** 根据BufferMgr的要求实现创建常量缓冲区的接口
		 * @param byteSize 常量缓冲区的总大小
		 * @param data 指向记录常量缓冲区初始数据的内存区域，在缓冲区非只读情况下可以为空
		 * @param flags 缓冲区的特性，具体查看BufferFlag定义
		 * @return 创建成功时返回缓冲区的索引，以供从管理器中检索缓冲区；创建失败返回-1
		 */
		BufferIdx CreateConstantBuffer(size_t byteSize, uint8_t* data, BufferFlagCombination flags);
		/// 创建DX11需要的索引缓冲区
		/** 根据BufferMgr的要求实现创建索引缓冲区的接口
		 * @param numIdxs 索引缓冲区中索引的数量
		 * @param IdxSize 一个索引的字节大小
		 * @param data 指向记录索引缓冲区初始数据的内存区域，在缓冲区非只读的情况下可以为空
		 * @param flags 缓冲区特性，具体查看BufferFlag定义
		 * @return 创建成功时返回缓冲区的索引，以供从管理器中检索缓冲区；创建失败返回-1
		 */
		BufferIdx CreateIndexBuffer(size_t numIdxs, size_t IdxSize, uint8_t* data, BufferFlagCombination flags);
		/** 根据缓冲的索引值，返回缓冲区引用
		 * @param index 需要查询的缓冲区的索引值
		 * @return 若查询成功，返回缓冲区的引用。当前尚无删除操作，所以只要
		 * 保证索引值在范围内，均可保证查询成功。
		 */
		Buffer & GetBuffer(BufferIdx index) {
			assert(static_cast<uint32_t>(index.value()) < m_buffers.size() && index >= 0);
			return m_buffers[index.value()];
		}
	private:
		std::vector<DX11_Buffer> m_buffers; /**< 存储已经创建的缓冲区的队列 */

	};

	class DX11_VertexDeclarationMgr : public VertexDeclarationMgr {
	public:
		DX11_VertexDeclarationMgr() {}
		virtual ~DX11_VertexDeclarationMgr() {}
	public:
		/// 根据顶点属性描述和着色器创建顶点声明对象
		/** 顶点声明对象作为顶点缓存以及顶点着色器之间的桥梁，需要两者配合创建
		 * @param verAttDescs 顶点属性描述队列，当中的元素均是对其中一个顶点属性的详细描述
		 * 该顶点属性对应着某个顶点缓冲的结构
		 * @param 需要接收顶点缓冲数据的顶点着色器的索引编号。
		 * @return 创建成功返回顶点属性描述对象的索引，创建失败返回-1
		 */
		virtual VertexDeclarationIdx CreateVertexDeclaration(const std::vector<SubVertexAttributeDesc>& verAttDescs, ShaderIdx shaderIndex);
		/** 从顶点属性描述对象管理器中检索|index|对应的对象 */
		virtual VertexDeclaration& GetVertexDeclaration(VertexDeclarationIdx index) {
			assert(static_cast<uint32_t>(index.value()) < m_vertexDecls.size() && index >= 0);
			return m_vertexDecls[index.value()];
		}
	private:
		std::vector<DX11_VertexDeclaration> m_vertexDecls; /**< 记录DX11专属的顶点属性描述对象的队列 */
	};

} // namespace UnknownVision

#endif // D3D11_RESOURCE_MANAGER_H
