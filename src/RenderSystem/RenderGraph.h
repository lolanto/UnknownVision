#ifndef RENDER_GRAPH_H
#define RENDER_GRAPH_H

#include "../UVConfig.h"
#include "../Utility/CommandBuffer/CommandBuffer.hpp"
#include <cstdint>
#include <vector>
#include <map>
#include <string>
#include <memory>

using ResourceName = std::string;

namespace UnknownVision {
	class PipelineState;
	class RenderGraph;

	/** 资源可以处于的状态(当前用途) */
	enum ResourceState : uint8_t {
		RESOURCE_STATE_UNKNOWN, /**< 未知的资源状态，用于标记获取该资源状态需要依赖其它方法 */
		RESOURCE_STATE_SHADER,
		RESOURCE_STATE_VERTEX_BUFFER,
		RESOURCE_STATE_INDEX_BUFFER,
		RESOURCE_STATE_DEPTH_STENCIL_TARGET,
		RESOURCE_STATE_RENDER_TARGET,
		RESOURCE_STATE_SHADER_RESOURCE,
		RESOURCE_STATE_CONSTANT_BUFFER,
		RESOURCE_STATE_UNORDER_ACCESS
	};
	/** 描述pass对资源的操作，可使用or进行连接 */
	enum ResourceOperation : uint8_t {
		RESOURCE_OPERATION_UNKNOWN = 0, /**<  */
		RESOURCE_OPERATION_READ = 0x01U, /**< 需要读取资源 */
		RESOURCE_OPERATION_WRITE = 0x02U, /**< 需要写入资源 */
	};

	/** 描述某个资源以某种方式使用时所处的状态 */
	struct ResourceStateAndOperation {
		ResourceState state; /**< 资源的状态(用途) */
		ResourceOperation operation; /**< 对资源的操作方式，可以用 | 进行连接 */
		bool operator==(const ResourceStateAndOperation& rhs) {
			return (state == rhs.state && operation == rhs.operation);
		}
	};

	/** 该类提供追踪某个资源当前状态的方法 */
	class ResourceStateTracker {
	public:
		ResourceStateAndOperation ResourceState(const ResourceName& name);
	};

	/** 存储pass对某一资源的操作方式，仅
	 * 描述对资源的基本操作(读写创建)
	 * @remark 
	 * 对于一维资源，width代表实际的资源大小，height,slides均设为0
	 * 对于二维资源，width，height代表实际资源的长宽，slides设为0
	 * 对于三维资源，在二维资源基础上，需要设置slides。*/
	struct PassResourceEntry {
		DataFormatType type; /**< 资源中一个元素的格式 */
		ResourceStateAndOperation stateAndOpt; /**< 资源的使用状态和使用方式 */
		uint8_t slides; /**< 对三位资源而言表示层数 */
		ResourceName name; /**< 需要使用的资源的名称 */
		uint32_t width; /**< 缓冲的宽度，单位字节 */
		uint32_t height; /**< 缓冲的高度，单位字节 */
	};

	/** 描述图中的每个pass的类 */
	class GraphPass {
		friend class RenderGraph;
	public:
		GraphPass() = default;
	public:
		/** 向GrpahPass中插入一条资源使用记录 */
		bool InsertUseEntry(const PassResourceEntry& entry);
		/** 设置该pass的pipelineState
		 * @param index 管线状态描述对象的索引 */
		void SetPipelineState(PipelineStateIdx index);
	private:
		std::vector<PassResourceEntry> m_entries; /**< 记录了pass对资源的所有操作 */
		PipelineStateIdx m_pipelineState; /**< 这个pass需要使用的pipeline state  */
		DescriptorLayoutIdx m_descriptorLayout; /**< 这个pass使用的descriptor layout */
		GraphPassIdx m_index; /**< 该pass在graph中的索引值，由Graph使用 */
		CommandList m_commandList; /**< 当前pass需要执行的实际指令 */
	};

	/** 描述整一帧的Graph */
	class RenderGraph {
	public:
		/** 描述某个pass和其它pass的依赖关系 */
		struct PassDependenceDescriptor {
			std::vector<GraphPassIdx> lastPasses; /**< 前置pass */
			std::vector<GraphPassIdx> nextPasses; /**< 后置pass */
		};
	private:
		/** 描述所有pass分析过程中，资源的状态变化情况 */
		struct ResourceStateChanging {
			ResourceStateAndOperation stateAndOpt; /**< 当前资源的状态和使用状况 */
			/** @remark 假如index=-1表示该资源没有被其它pass使用 */
			GraphPassIdx lastUseIdx; /**< 当前最后一个使用该资源的pass，可以是读/写操作 */
			GraphPassIdx lastWriteIdx; /**< 当前最后一个写该资源的pass */
		};
	public:
		/** 向Graph中添加一个pass,重复添加的pass应该保证
		 * 只添加一次
		 * @remark GraphPass添加的顺序应该与要执行的顺序
		 * 基本一致，RenderGraph负责的是将输入的一维顺序
		 * 更改为可并行的执行顺序，并生成相应的代码
		 * @param pass 需要添加的pass的引用 */
		void AddGraphPass(const GraphPass& pass);
		/// 编译整个图，安排需要执行的指令以及执行顺序
		/** 该函数需要推导各个pass的依赖关系，从而生成
		 * 相应的执行指令和执行顺序。其中，指令还包括
		 * barrier的设置，内存管理，同步。
		 * @return 编译成功返回true，编译失败返回false */
		bool Compile();
		/** 执行一帧的渲染，未编译或编译失败时，该函数不进行任何操作 */
		void Execute() const;
		/** 返回当前RenderGraph中的总pass数量 */
		size_t NumPasses() const { return m_passes.size(); }
	private:
		bool m_hasCompiled = false; /**< 标志该graph是否已经编译，只有编译后的graph才能被执行 */
		std::vector<GraphPass> m_passes; /**< 存储该graph的所有pass，这些pass有自己基本的顺序 */

		/** 一个graph可能可以同时开启多个pass，所以入口的pass可以有多个 */
		std::vector<GraphPassIdx> m_entryPassIndices;
		std::map<GraphPassIdx, PassDependenceDescriptor> m_passDependencies; /**< 记录了所有pass的依赖关系 */

		/** 用于存储各个pass的资源状况，Compile时使用。Graph初始化时需要添加边界pass依赖的资源 */
		std::map<ResourceName, ResourceStateChanging> m_ResourceNameAndStates;
	};


}

#endif // RENDER_GRAPH_H
