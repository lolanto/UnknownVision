#pragma once
#include <vector>
#include <filesystem>

#define USE_ASSIMP
#define FLIP_UV // 假如你的图形API需要

namespace MeshLoader {
	enum VertexDataType : uint8_t {
		VERTEX_DATA_TYPE_POSITION = 0x01u,
		VERTEX_DATA_TYPE_NORMAL = 0x02u,
		VERTEX_DATA_TYPE_TANGENT = 0x04u,
		VERTEX_DATA_TYPE_BITANGENT = 0x08u,
		VERTEX_DATA_TYPE_TEXCOORD0 = 0x10u
	};
	constexpr size_t POSITION_DATA_SIZE = 3 * sizeof(float);
	constexpr size_t NORMAL_DATA_SIZE = 3 * sizeof(float);
	constexpr size_t TANGENT_DATA_SIZE = 3 * sizeof(float);
	constexpr size_t BITANGENT_DATA_SIZE = 3 * sizeof(float);
	constexpr size_t TEXCOORD_DATA_SIZE = 2 * sizeof(float);
	constexpr size_t INDEX_DATA_SIZE = sizeof(uint32_t);

	/** 模型加载器的配置信息
	 * Note: 尽量不用手动创建，而是调用相关辅助函数获得 */
	struct MeshLoaderConfigurationDesc {
		/** 每个缓冲中数据组织方式 
		 * 比如 [[position, normal], [tangent, bitangent], [texcoord0]]
		 * 就表示一共有三个顶点缓冲，第0个缓冲的格式是position_A+normal_A, position_B+normal_B, ...
		 * 第1个缓冲格式是tangent_A+bitangent_A, tangent_B+bitangent_B, ... 
		 * 以此类推 */
		std::vector<std::vector<VertexDataType>> bufferSettings;
		/** 每个顶点拥有的属性概括，通过 | 逻辑运算符进行连接，必须覆盖bufferSettings中的设置 */
		VertexDataType globalSettings; /**< 整个模型包含的数据 */

		/** 配置描述生成函数 */
		/** 仅一个缓冲，包含位置信息 */
		static MeshLoaderConfigurationDesc P();
		/** 仅一个缓冲，包含位置和纹理信息 */
		static MeshLoaderConfigurationDesc PT0();
		/** 仅一个缓冲，包含位置法线纹理信息 */
		static MeshLoaderConfigurationDesc PNT0();
		/** 仅一个缓冲，包含位置法线信息 */
		static MeshLoaderConfigurationDesc PN();
		/** 仅一个缓冲，包含位置法线，切线以及纹理信息 */
		static MeshLoaderConfigurationDesc PNTgT0();
	};

	struct MeshDataContainer {
		size_t numVertices;
		size_t numIndices;
		std::vector<std::vector<uint8_t>> vtxBuffers;
		std::vector<size_t> elementStrides;
		std::vector<uint32_t> idxBuffer;
		void Clear() {
			vtxBuffers.clear();
			idxBuffer.clear();
		}
		MeshDataContainer() = default;
		MeshDataContainer(MeshDataContainer&& rhs) {
			vtxBuffers.swap(rhs.vtxBuffers);
			idxBuffer.swap(rhs.idxBuffer);
			elementStrides.swap(rhs.elementStrides);
			numVertices = rhs.numVertices;
			numIndices = rhs.numIndices;
		}
		MeshDataContainer(const MeshDataContainer& rhs) {
			vtxBuffers = rhs.vtxBuffers;
			idxBuffer = rhs.idxBuffer;
			elementStrides = rhs.elementStrides;
			numVertices = rhs.numVertices;
			numIndices = rhs.numIndices;
		}
	};

	class IMeshLoader {
	public:
		static IMeshLoader* GetLoader(const MeshLoaderConfigurationDesc& config);
	public:
		IMeshLoader(const MeshLoaderConfigurationDesc& config) : m_config(config) {}
		virtual ~IMeshLoader() = default;
		/** 设置加载器的属性
		 * @param newConfig 新的加载设置
		 * @return 之前配置的加载设置 */
		MeshLoaderConfigurationDesc SetConfig(const MeshLoaderConfigurationDesc& newConfig) {
			MeshLoaderConfigurationDesc oldConfig = m_config;
			m_config = newConfig;
			return oldConfig;
		}
		/** 根据指定的文件路径加载模型文件数据
		 * @param path 模型文件路径
		 * @param output 模型文件数据，详见类型定义 
		 * @return 返回加载状态，加载失败为false */
		virtual bool Load(std::filesystem::path path, std::vector<MeshDataContainer>& output) = 0;
	protected:
		MeshLoaderConfigurationDesc m_config;
	};
};

inline uint8_t operator&(MeshLoader::VertexDataType a, MeshLoader::VertexDataType b) { return static_cast<uint8_t>(a)& static_cast<uint8_t>(b); }
inline MeshLoader::VertexDataType operator|(MeshLoader::VertexDataType a, MeshLoader::VertexDataType b) { return  static_cast<MeshLoader::VertexDataType>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b)); }
