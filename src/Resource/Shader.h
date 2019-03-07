#ifndef UV_SHADER_H
#define UV_SHADER_H

#include "Resource.h"

namespace UnknownVision {
	/** 存储shader的必要描述信息，包括类型，字节码，描述名称 */
	class Shader : public Resource {
		friend class ShaderMgr;
	public:
		~Shader() = default;
		/** 返回该shader的字节码，返回类型由具体实现决定 */
		virtual void* GetByteCode() = 0;
		/** 获得shader的描述信息
		 * 仅当该函数被调用时，描述结构体才会实际被创建 */
		const ShaderDescription& GetDescription() {
			if (!m_hasBeenReflected) {
				updateDescription();
				m_hasBeenReflected = true;
			}
			return m_description;
		}
	public:
		const ShaderType Type; /**< Shader类型 */
		const std::string Name; /**< shader名称，用于调试和索引 */
	protected:
		Shader(const char* name, ShaderType type, uint32_t RID)
			: Type(type), Name(name), m_hasBeenReflected(false), Resource(RID) {}
		/** 更新shader的字节码，该函数应该由ShaderMgr进行调用
		 * 子类需要进行重载以更新实际的字节码存储对象
		 * 并调用该函数更新描述信息状态 */
		virtual void updateByteCode(void*) {
			m_hasBeenReflected = false; /**< 替换后描述信息也需要更新 */
		}
		/** 用于更新shader的描述信息，由具体的API实现完成 */
		virtual void updateDescription() = 0;
	protected:
		bool m_hasBeenReflected; /**< 是否已经创建(更新)过描述信息 */
		ShaderDescription m_description; /**< 存储shader的描述 */
	};
}

#endif // UV_SHADER_H
