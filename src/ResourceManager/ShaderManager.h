#ifndef UV_SHADER_MANAGER_H
#define UV_SHADER_MANAGER_H

#include "ResourceMananger.h"

namespace UnknownVision {
	/** 负责创建/存储/管理当前需要使用的所有shader */
	class ShaderMgr : public ResourceMgr {
	public:
		ShaderMgr() : ResourceMgr(MT_SHADER_MANAGER) {}
		virtual ~ShaderMgr() {}
	public:
		virtual ShaderIdx CreateShaderFromSourceFile(const char* filePath, const char* shaderName,
			ShaderType type) = 0;
		/** 根据index返回shader
		 * @param index 需要获得的shader的索引值
		 * @remark 一旦index无效，则抛出std::out_of_range异常 */
		Shader& GetShaderFromIndex(ShaderIdx index) {
			const auto& shaderIter = m_shaderIdxToShader.find(index);
			if (shaderIter == m_shaderIdxToShader.end()) {
				throw(std::out_of_range("index is invalid"));
			}
			return shaderIter->second;
		}
		/** 根据index返回shader
		 * @param index 需要获得的shader的索引值
		 * @remark 一旦index无效，则抛出std::out_of_range异常 */
		const Shader& GetShaderFromIndex(ShaderIdx index) const {
			const auto& shaderIter = m_shaderIdxToShader.find(index);
			if (shaderIter == m_shaderIdxToShader.end()) {
				throw(std::out_of_range("index is invalid"));
			}
			return shaderIter->second;
		}
		/** 根据name返回shader
		 * @param name 需要获得的shader的名称
		 * @remark 一旦index无效，则抛出std::out_of_range异常 */
		Shader& GetShaderFromName(const char* name) {
			const auto& shaderNameIter = m_shaderNameToShaderIdx.find(name);
			if (shaderNameIter == m_shaderNameToShaderIdx.end()) {
				throw(std::out_of_range("name is invalid"));
			}
			auto& shaderIter = m_shaderIdxToShader.find(shaderNameIter->second);
			return shaderIter->second;
		}
		/** 根据name返回shader
		 * @param name 需要获得的shader的名称
		 * @remark 一旦index无效，则抛出std::out_of_range异常 */
		const Shader& GetShaderFromName(const char*name) const {
			const auto& shaderNameIter = m_shaderNameToShaderIdx.find(name);
			if (shaderNameIter == m_shaderNameToShaderIdx.end()) {
				throw(std::out_of_range("name is invalid"));
			}
			auto& shaderIter = m_shaderIdxToShader.find(shaderNameIter->second);
			return shaderIter->second;
		}
	protected:
		std::map<std::string, ShaderIdx> m_filePathToShaderIdx;
		std::map<ShaderIdx, Shader> m_shaderIdxToShader;
		std::map<std::string, ShaderIdx> m_shaderNameToShaderIdx;
	};

}

#endif // UV_SHADER_MANAGER_H
