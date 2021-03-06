#ifndef UV_RESOURCE_H
#define UV_RESOURCE_H
#include "ResourceConfig.h"
#include "ShaderDescription.h"
#include <vector>

// 针对资源的抽象基类
/* 无论何种Resource对象
均不包含资源对应的图形库的相关内容
与图形库相关的内容存储在管理器中
资源对象仅包含资源的属性以供查询
*/
namespace UnknownVision {
	class Resource {
	public:
		Resource(uint32_t id) : RID(id) {}
		const uint32_t RID;
		~Resource() = default;
	};

	class VertexDeclaration : public Resource {
	public:
		VertexDeclaration(uint32_t RID)
			: Resource(RID) {}
	};

	/** 描述一个Entry的类型 */
	enum DescriptorType : uint8_t {
		DT_INLINE_CONSTANT_VALUE, /**< 该Entry为常量值 */
		DT_INLINE_DESCRIPTOR, /**< 该Entry为一个描述符 */
		DT_DESCRIPTOR_TABLE /**< 该Entry为多个描述符的集合 */
	};
	/** 一项Entry存储的所有描述符对应的寄存器类型 */
	enum RegisterType : uint8_t {
		RT_TEXTURE_REGISTER,
		RT_CONSTANT_BUFFER_REGISTER,
		RT_UNORDER_ACCESS_REGISTER
	};

	struct DescriptorLayoutEntry{
		DescriptorType descriptortype; /**< 该Entry存储描述符的方式(单个/集合) */
		RegisterType registerType; /**< 该Entry存储的描述符对应Shader的寄存器类型 */
		uint8_t basedIndex; /**< 该Entry描述符对应的寄存器的基本编号 */
		uint8_t numberOfDescriptor; /**< 当该Entry是描述符集合时，指明描述符的数量 */
	};

	/** 描述符的布局描述结构 */
	struct DescriptorLayout {
		std::vector<DescriptorLayoutEntry> entries; /**< 存储各项描述符，要注意存储的顺序 */
	};

	/** 描述管线状态的结构，具体可设置状态查看具体属性
	该结构被用于PipelineState创建相关函数*/
	struct PipelineStateDesc {
		ShaderIdx vs; /**< 顶点着色器下标 */
		ShaderIdx ps; /**< 像素着色器下标 */
		ViewPortDesc viewport; /**< 视口描述对象 */
		VertexDeclarationIdx vertexDeclaration; /**< 顶点缓冲区描述对象的下标 */
		Primitive primitiveType; /**< 图元类型 */
		DataFormatType DSVFormat; /**< 深度模板缓存的元素格式 */
		DataFormatType RTVFormat[UV_MAX_RENDER_TARGET]; /**< 各个渲染对象的元素格式 */
		/** 构造一个包含默认设置的描述对象，默认值为：
		 * @param vs = -1, 用户必须设置
		 * @param ps = -1, 用户必须设置
		 * @param viewport 查看ViewPortDesc定义
		 * @param vertexDeclaration = -1, 用户必须定义
		 * @param primitiveType = INVALID 用户必须定义
		 * @param DSVFormat = D24_UNORM_S8_UINT 常用类型
		 * @param RTVFormats = R8G8B8A8_UNORM 常用类型 */
		PipelineStateDesc() : vs(-1), ps(-1), vertexDeclaration(-1), primitiveType(PRI_INVALID),
			DSVFormat(DFT_D24_UNORM_S8_UINT), viewport(ViewPortDesc()) {
			for (DataFormatType& rtvfs : RTVFormat)
				rtvfs = DFT_R8G8B8A8_UNORM;
		}
	};

	/** 承载管线对象的虚基类 */
	class PipelineState : public Resource {
	public:
		PipelineState(uint32_t RID) : Resource(RID) {}
	};

}

#endif // UV_RESOURCE_H
