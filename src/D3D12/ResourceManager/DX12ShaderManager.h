#ifndef UV_D3D12_SHADER_MANAGER_H
#define UV_D3D12_SHADER_MANAGER_H

#include "../DX12Config.h"
#include "../Resource/DX12Shader.h"
#include "../../ResourceManager/ShaderManager.h"

namespace UnknownVision {
	class DX12_ShaderMgr : public ShaderMgr {
	public:
		/** 从源文件创建Shader对象
		 * @param filePath shader源文件路径
		 * @param shaderName shader的名称，用于索引和调试，名称不能重复
		 * @param type shader的类型
		 * @return 创建成功返回索引，创建失败返回-1 */
		virtual ShaderIdx CreateShaderFromSourceFile(const char* filePath, const char* shaderName,
			ShaderType type);
		/** 根据index返回shader
		 * @param index 需要获得的shader的索引值
		 * @remark 一旦index无效，则抛出std::out_of_range异常 */
		virtual Shader& GetShaderFromIndex(ShaderIdx index);
		/** 根据index返回shader
		 * @param index 需要获得的shader的索引值
		 * @remark 一旦index无效，则抛出std::out_of_range异常 */
		virtual const Shader& GetShaderFromIndex(ShaderIdx index) const;
		/** 根据name返回shader
		 * @param name 需要获得的shader的名称
		 * @remark 一旦index无效，则抛出std::out_of_range异常 */
		virtual Shader& GetShaderFromName(const char* name);
		/** 根据name返回shader
		 * @param name 需要获得的shader的名称
		 * @remark 一旦index无效，则抛出std::out_of_range异常 */
		virtual const Shader& GetShaderFromName(const char*name) const;
	private:
		std::map<ShaderIdx, DX12_Shader> m_shaderIdxToShader;
	};
}

#endif // UV_D3D12_SHADER_MANAGER_H
