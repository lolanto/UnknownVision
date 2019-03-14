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
		const ResourceDescriptor& operator=(const ResourceDescriptor& rhs) {
			type = rhs.type;
			slot = rhs.slot;
			isArray = rhs.isArray;
			arraySize = rhs.arraySize;
			space = rhs.space;
			name = rhs.name;
			return *this;
		}
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

	/** 存储shader描述信息的对象 */
	class ShaderDescription {
	public:
		ShaderDescription() : m_numOfConstantBuffers(0), m_numOfSamplers(0),
			m_numOfShaderResources(0), m_numOfUnoderAccessBuffers(0) {}
	public:
		/** 将shader描述信息格式化打印到控制台
		 * @param desc 需要打印的shader描述信息结构体引用 */
		void PrintShaderDescriptionToConsole();
		/** 辅助函数 */
		inline uint8_t NumberOfConstantBuffers() const { return m_numOfConstantBuffers; }
		inline uint8_t NumberOfShaderResources() const { return m_numOfShaderResources; }
		inline uint8_t NumberOfSamplers() const { return m_numOfSamplers; }
		inline uint8_t NumberOfUnoderAccessBuffers() const { return m_numOfUnoderAccessBuffers; }
		/** 向描述体中存储新的资源描述信息
		 * @param desc 需要增加的资源描述信息
		 * @remark 重复增加相同名称的描述信息会覆盖原有信息*/
		void InsertResourceDescription(const ResourceDescriptor& desc);
		/** 获取指定资源的描述信息
		 * @param name 需要获取的资源的名称
		 * @return 获取成功返回对应资源的描述信息引用，获取失败抛出std::out_of_range*/
		const ResourceDescriptor& GetResourceDescription(const std::string& name);
	private:
		using ResourceDescriptorMap = std::map<std::string, ResourceDescriptor>;
		ResourceDescriptorMap m_resourceDescriptiors; /** 以资源名称为索引，存储各个资源的描述信息 */
		uint8_t m_numOfConstantBuffers;
		uint8_t m_numOfShaderResources;
		uint8_t m_numOfSamplers;
		uint8_t m_numOfUnoderAccessBuffers;
	};
}
#endif // SHADER_DESCRIPTOR_H
