#ifndef SHADER_DESCRIPTOR_H
#define SHADER_DESCRIPTOR_H

#include <cstdint>
#include <string>
#include <map>

namespace UnknownVision {
	/** 通用的shader资源描述结构体
	 * @remark 当isArray为false时，成员arraySize
	 * 没有意义。其次，arraySize设置为UNBOUNDED_ARRAY
	 * 表示该数组类型的资源边界由外部决定 */
	struct ResourceDescriptor {
		const uint8_t UNBOUNDED_ARRAY = UINT8_MAX; /**< 表示该数组边界由外部决定 */
		enum RegisterType : uint8_t {
			REGISTER_TYPE_CONSTANT_BUFFER = 0,
			REGISTER_TYPE_SHADER_RESOURCE,
			REGISTER_TYPE_SAMPLER,
			REGISTER_TYPE_UNORDER_ACCESS
		};
		std::string name; /**< 资源名称 */
		RegisterType type; /**< 资源绑定的寄存器类型 */
		uint8_t slot; /**< 资源绑定的最小寄存器编号 */
		bool isArray; /**< 是否以数组的形式进行绑定 */
		uint8_t arraySize; /**< 数组的大小 */
		uint8_t space; /**< 该资源的逻辑空间 */
	};

	/** 存储shader描述信息的结构体 */
	struct ShaderDescription {
		using ResourceDescriptorMap = std::map<std::string, ResourceDescriptor>;
		ResourceDescriptorMap constantBuffers; /**< 存储该shader的所有constant buffer描述 */
		ResourceDescriptorMap shaderResources; /**< 存储该shader的所有shader resource描述 */
		ResourceDescriptorMap samplers; /**< 存储该shader的所有sampler描述 */
		ResourceDescriptorMap unorderAccessBuffers; /**< 存储该shader的所有unorder access buffer描述 */
		/** 辅助函数 */
		inline uint32_t NumberOfConstantBuffers() const { return static_cast<uint32_t>(constantBuffers.size()); }
		inline uint32_t NumberOfShaderResources() const { return static_cast<uint32_t>(shaderResources.size()); }
		inline uint32_t NumberOfSamplers() const { return static_cast<uint32_t>(samplers.size()); }
		inline uint32_t NumberOfUnoderAccessBuffers() const { return static_cast<uint32_t>(unorderAccessBuffers.size()); }
	};

	/** 将shader描述信息格式化打印到控制台
	 * @param desc 需要打印的shader描述信息结构体引用 */
	void PrintShaderDescriptionToConsole(const ShaderDescription& desc);
}
#endif // SHADER_DESCRIPTOR_H
