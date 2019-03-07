#ifndef UV_D3D12_SHADER_H
#define UV_D3D12_SHADER_H

#include "../DX12Config.h"
#include "../../Resource/Shader.h"

namespace UnknownVision {
	class DX12_Shader : public Shader {
		friend class DX12_ShaderMgr;
	public:
		virtual ~DX12_Shader() = default;
		/** 返回该shader的字节码，返回类型由具体实现决定 */
		virtual void* GetByteCode() { return m_byteCode.Get(); }
	private:
		DX12_Shader(const char* name, ShaderType type, uint32_t RID)
			: Shader(name, type, RID) {}
		/** 用于更新shader的描述信息，由具体的API实现完成 */
		virtual void updateDescription();
	private:
		SmartPTR<ID3DBlob> m_byteCode; /**< 存储shader编译后字节码 */
	};
}

#endif // UV_D3D12_SHADER_H
