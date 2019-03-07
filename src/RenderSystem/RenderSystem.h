#ifndef RENDER_SYS_H
#define RENDER_SYS_H

#include "../ResourceManager/ResourceManangerConfig.h"
#include "RenderSystemConfig.h"
#include <functional>

class WindowBase;
namespace UnknownVision {
	/// 渲染系统的抽象基类，声明了渲染系统的接口
	/** 该类就是对管线进行抽象，所提供的接口均是
	 * 针对管线设置以及管线控制的。实际接口需要
	 * 依赖不同图形API的对RenderSys虚基类的继承与实现
	 */
	class RenderSys {
	public:
		/** RenderSys的构造函数，主要记录一些基本属性
		 * @param api 管线使用的图形API类型
		 * @param width, height 管线管理的输出窗口的大小(可以考虑去掉该信息)
		 */
		RenderSys(API_TYPE api, float width, float height) :
			m_basicHeight(height), m_basicWidth(width), API(api) {}
		virtual ~RenderSys() = default;
		/** 返回管线管理的输出窗口的宽度 */
		float Width() const { return m_basicWidth; }
		/** 返回管线管理的输出窗口的高度 */
		float Height() const { return m_basicHeight; }
		const API_TYPE API; /**< 管线使用的图形API类型 */
	public:
		/// Ultility Function
		/** 管线的初始化函数，由具体API实现
		 * @param win 用于显示渲染结果的系统窗口
		 * @return 初始化成功返回true，失败返回false */
		virtual bool Init(WindowBase* win) = 0;
		/** 重置所有之前设置的管线状态 */
		virtual void ResetAll() = 0;

		/// For Shaders
		/** 向管线上绑定某个Shader
		 * @remark 当同类Shader进行绑定时，已绑定的Shader会被取代
		 * @param index 需要绑定的Shader的索引值，从ShaderMgr中获取
		 * @return 若绑定成功，返回true；绑定失败返回false
		 * @remark 不要在DX12中使用 */
		virtual bool BindShader(ShaderIdx index) = 0;
		/** 删除管线上某个已绑定的Shader
		 * @param type 需要删除绑定的Shader的类型，具体查看ShaderType定义
		 * @return 删除成功返回true；删除失败返回false
		 * @remark 不要在DX12中使用 */
		virtual bool UnbindShader(ShaderType type) = 0;

		/** 激活某个输入格式
		 * @remark 激活后的输入格式直到下次激活其它设置为止一直有效
		 * @param index 需要激活的输入格式的索引，从VertexDeclarationMgr中检索
		 * @remark 不要在DX12中使用 */
		virtual void ActiveVertexDeclaration(VertexDeclarationIdx index) = 0;
		/** 激活某个视口设置
		 * @remark 激活后的设置直到下次激活其它设置为止一直有效
		 * @param desc 需要激活的视口设置描述对象的引用，
		 * 具体设置内容查看ViewPortDesc定义
		 * @remark 不要在DX12中使用 */
		virtual void ActiveViewPort(const ViewPortDesc& desc) = 0;
		/** 绑定一个顶点缓冲区，默认绑定到0号位置
		 * @remark多次调用会覆盖之前的设置
		 * @param index 需要绑定的顶点缓冲区的索引
		 * @return 绑定成功返回true，绑定失败返回false
		 * @remark 不要在DX12中使用 */
		virtual bool BindVertexBuffer(BufferIdx index) = 0;
		/** 绑定多个顶点缓冲区
		 * 按照缓冲区在数组中的顺序，从0接口开始绑定
		 * @remark 多次调用会覆盖之前的设置，加入之后设置的缓冲区没有之前
		 * 的绑定的多，则之前多出来的依然会被取消绑定
		 * @param indices 需要绑定的缓冲区的索引数组
		 * @param numBuf 数组的大小
		 * @return 绑定成功返回true，绑定失败返回false
		 * @remark 不要在DX12中使用 */
		virtual bool BindVertexBuffers(BufferIdx* indices, size_t numBuf) = 0;
		/** 向特定的管线阶段绑定常量缓冲区，并指定绑定的接口
		 * @remark 多次调用，并设置相同的管线阶段以及接口编号会覆盖之前的设置
		 * @param index 需要绑定的常量缓冲区的索引
		 * @param stage 需要绑定的管线阶段，具体查看PipelineStage的定义
		 * @param slot 需要绑定到的接口的编号
		 * @return 绑定成功返回true，绑定失败返回false
		 * @remark 不要在DX12中使用 */
		virtual bool BindConstantBuffer(BufferIdx index, PipelineStage stage, SlotIdx slot) = 0;
		/** 向管线绑定索引缓冲 
		 * @param index 索引缓冲在缓冲管理器中的索引号
		 * @return 绑定成功返回true，绑定失败返回false
		 * @remark 不要在DX12中使用 */
		virtual bool BindIndexBuffer(BufferIdx index) = 0;
		/** 取消索引缓冲的绑定
		 * @remark 不要在DX12中使用 */
		virtual void UnbindIndexBuffer() = 0;
		/** 向管线绑定深度模板测试缓存
		 * @param index 深度模板缓存的索引值，供具体的Mgr检索
		 * @return 绑定成功返回true，绑定失败返回false
		 * @remark TODO：DepthStencil可能是不同的资源创建的，后续
		 * 可能需要考虑该从哪个资源管理器上获取。暂时该资源只在Texture2DMgr
		 * 中基于Texture2D资源进行创建
		 * @remark 不要在DX12中使用 */
		virtual bool BindDepthStencilTarget(DepthStencilIdx index) = 0;
		/** 取消当前管线的深度模板缓存绑定
		* @remark 不要在DX12中使用 */
		virtual void UnbindDepthStencilTarget() = 0;
		/** 向管线绑定一个渲染对象，默认绑定到0号接口
		 * @param index 需要绑定渲染对象的索引，供具体的Mgr检索
		 * @return 绑定成功返回true，绑定失败返回false
		 * @remark TODO：RenderTarget可以基于不同的资源创建，后续
		 * 需要考虑如何设计，让其能从不同的管理器上获取。暂时该资源只在Texture2DMgr
		 * 中基于Texture2D资源进行创建
		 * @remark 不要在DX12中使用 */
		virtual bool BindRenderTarget(RenderTargetIdx index) = 0;
		/** 向管线绑定多个渲染对象
		 * @remark 绑定的顺序与渲染对象在索引数组中的顺序相同，接口从0开始
		 * 之前绑定的所有渲染对象都将被解除绑定
		 * @param indices 需要绑定的渲染对象的索引数组
		 * @param numRenderTarget 索引数组中的索引总量
		 * @remark 不要在DX12中使用 */
		virtual bool BindRenderTargets(RenderTargetIdx* indices, size_t numRenderTarget) = 0;
		/** 解除当前绑定的所有渲染对象
		 * @remark 不要在DX12中使用 */
		virtual void UnbindRenderTarget() = 0;
		/** 清空某个渲染对象，该对象不一定被绑定
		 * @param index 需要清空的渲染对象的索引
		 * @remark TODO：渲染对象可能来自不同的Mgr，后期
		 * 需要考虑如何设计接口，从不同Mgr中获得需要的渲染对象，
		 * 暂时渲染对象来自Texture2DMgr
		 * @remark 不要在DX12中使用 */
		virtual void ClearRenderTarget(RenderTargetIdx index) = 0;

		// virtual bool SetCullMode() = 0;
		/** 设置光栅过程的图元类型
		 * @param pri 图元类型，具体定义查看Primitive
		 * @remark 不要在DX12中使用 */
		virtual void SetPrimitiveType(Primitive pri) = 0;
		/// Draw Call
		/** 渲染命令，由RenderSystem的具体实现绘制的方式(逐顶点/逐索引等) */
		virtual void Draw() = 0;
		/** 将渲染的结果进行呈现
		 * 因为DX需要翻转BackBuffer，需要额外的调用，暂时
		 * 不知道如何放置该调用，故设置该接口
		 */
		virtual void Present() = 0;

	protected:
		float m_basicWidth = 0; /**< 渲染管线管理的输出宽度 */
		float m_basicHeight = 0; /**< 渲染管线管理的输出高度 */
	};

	/// 针对新API，重新设计的渲染系统基类
	/** 渲染系统仅作为DrawCall的发起器，提供的接口
	 * 包括初始化必要组建，接收渲染管线状态，启动管线。其它的设置内容由资源
	 * 自己完成 */
	class RenderSys2 {
	public:
		/** RenderSys2的构造函数，进行基础数据成员的初始化
		 * @param api 当前实现Render Sys的api类型
		 * @param width 当前渲染窗口的总宽度
		 * @param height 当前渲染窗口的总高度 */
		RenderSys2(API_TYPE api, uint32_t width, uint32_t height) :
			m_height(height), m_width(width), m_api(api) {}
		virtual ~RenderSys2() = default;
	public:
		uint32_t Width() const { return m_width; }
		uint32_t Height() const { return m_height; }
		API_TYPE API() const { return m_api; }
	public:
		/// Ultility Function
		/** 管线的初始化函数，由具体API实现
		* @param win 用于显示渲染结果的系统窗口
		* @return 初始化成功返回true，失败返回false */
		virtual bool Init(WindowBase* win) = 0;
	protected:
		/** 一下宽高与主要的backbuffer设置相关 */
		uint32_t m_width; /**< 渲染窗口的总宽度 */
		uint32_t m_height; /**< 渲染窗口的总高度 */
		API_TYPE m_api; /**< 当前渲染系统代表的API类型 */
	};
}

#endif // RENDER_SYS_H
