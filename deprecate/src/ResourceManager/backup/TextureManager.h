#ifndef UV_TEXTURE_MANAGER_H
#define UV_TEXTURE_MANAGER_H

#include "ResourceMananger.h"

namespace UnknownVision {
	class TextureMgr : public ResourceMgr {
	public:
		TextureMgr(const ManagerType& type) : ResourceMgr(type) {}
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

}

#endif UV_TEXTURE_MANAGER_H
