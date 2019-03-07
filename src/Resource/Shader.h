#ifndef UV_SHADER_H
#define UV_SHADER_H

#include "Resource.h"

namespace UnknownVision {
	/** 存储shader的必要描述信息，包括类型，字节码，描述名称 */
	class Shader : public Resource {
		friend class ShaderMgr;
	public:
		Shader(const char* name, ShaderType type, uint32_t RID)
			: Type(type), Name(name), hasBeenReflected(false), Resource(RID) {}
		~Shader() = default;
		/** 返回该shader的字节码 */
		const std::vector<uint8_t>& GetByteCode() const { return m_byteCode; }
		/** 获得shader的描述信息
		 * 仅当该函数被调用时，描述结构体才会实际被创建 */
		const ShaderDescription& GetDescription() {
			if (!hasBeenReflected) {
				updateDescription();
			}
			return m_description;
		}
	public:
		const ShaderType Type; /**< Shader类型 */
		const std::string Name; /**< shader名称，用于调试和索引 */
	protected:
		/** 更新shader的字节码，该函数应该由ShaderMgr进行调用
		 * @byteCode 替换原有字节码的新字节码 */
		void updateByteCode(std::vector<uint8_t>& byteCode) {
			hasBeenReflected = false; /**< 替换后描述信息也需要更新 */
			m_byteCode.swap(byteCode);
		}
		/** 用于更新shader的描述信息，由具体的API实现完成 */
		virtual void updateDescription() = 0;
	protected:
		std::vector<uint8_t> m_byteCode; /**< 存储shader编译后的字节码 */
		bool hasBeenReflected; /**< 是否已经创建(更新)过描述信息 */
		ShaderDescription m_description; /**< 存储shader的描述 */
	};
}

#endif // UV_SHADER_H
