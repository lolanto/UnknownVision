#ifndef RENDER_GRAPH_H
#define RENDER_GRAPH_H

#include "../UVConfig.h"
#include "../Utility/CommandBuffer/CommandBuffer.hpp"
#include <cstdint>
#include <vector>
#include <string>

using ResourceName = std::string;

namespace UnknownVision {
	class PipelineState;
	class RenderGraph;

	/** 描述pass操作的资源的用途 */
	enum PassResourceUsage {
		PRU_VERTEX_BUFFER,
		PRU_INDEX_BUFFER,
		PRU_DEPTH_STENCIL_TARGET,
		PRU_RENDER_TARGET,
		PRU_SHADER_RESOURCE,
		PRU_CONSTANT_BUFFER,
		PRU_UNORDER_ACCESS
	};
	/** 描述pass对资源的操作 */
	enum PassResourceOperation : uint8_t {
		PRO_READ = 0x01U, /**< 需要读取资源 */
		PRO_WRITE = 0x02U, /**< 需要写入资源 */
		PRO_CREATE = 0X04U /**< 需要创建资源 */
	};

	/** GraphPass中的一个操作描述结构，仅
	 * 描述对资源的基本操作(读写创建)
	 * @remark 
	 * 对于一维资源，width代表实际的资源大小，height,slides均设为0
	 * 对于二维资源，width，height代表实际资源的长宽，slides设为0
	 * 对于三维资源，在二维资源基础上，需要设置slides。*/
	struct PassResourceEntry {
		DataFormatType type; /**< 资源中一个元素的格式 */
		PassResourceOperation operation; /**< 对资源的操作方式，可以用 | 进行连接 */
		uint8_t slides; /**< 对三位资源而言表示层数 */
		PassResourceUsage usage; /**< 资源的用途 */
		ResourceName name; /**< 需要使用的资源的名称 */
		uint32_t width; /**< 缓冲的宽度，单位字节 */
		uint32_t height; /**< 缓冲的高度，单位字节 */
	};

	/** 描述图中的每个pass的类 */
	class GraphPass {
		friend class RenderGraph;
	public:
		/** 向GrpahPass中插入一条资源使用记录 */
		bool InsertUseEntry(const PassResourceEntry& entry);
		/** 设置该pass的pipelineState */
		void SetPipelineState(PipelineStateIdx index);
	private:
		std::vector<PassResourceEntry> m_entries; /**< 记录了pass对资源的所有操作 */
		PipelineStateIdx m_pipelineState; /**< 这个pass需要使用的pipeline state  */
		DescriptorLayoutIdx m_descriptorLayout; /**< 这个pass使用的descriptor layout */
		CommandList m_commandList; /**< 当前pass需要执行的实际指令 */
	};

	/** 描述整一帧的Graph */
	class RenderGraph {
	public:
		/// 编译整个图，安排需要执行的指令以及执行顺序
		/** 该函数需要推导各个pass的依赖关系，从而生成
		 * 相应的执行指令和执行顺序。其中，指令还包括
		 * barrier的设置，内存管理，同步。
		 * @return 编译成功返回true，编译失败返回false */
		bool Compile();
		/** 执行一帧的渲染，未编译或编译失败时，该函数不进行任何操作 */
		void Execute();
	private:
		bool m_hasCompiled = false;
		std::vector<GraphPass> m_passes;
	};
}

#endif // RENDER_GRAPH_H
