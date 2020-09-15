#pragma once

#include "RenderPass.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>

BEG_NAME_SPACE

class RenderDevice;
class PipelineObject;


enum SizeClass : uint8_t {
	ABSOLUTE = 0,
	SWAP_CHAIN_RELATIVE
};


/** RenderGraph的职责:
 * 1. 管理资源的生命周期和状态更改
 * 2. 协调pass的执行顺序 */

class RenderGraph : public Uncopyable {
public:
	RenderGraph() = default;
	~RenderGraph() = default;
public:
	/** 向Graph添加资源绑定信息，这些资源会被一个或多个pass使用(不包含临时资源) */
	/*void AddResource(ResourceInfo&& res) { m_accessedResources.insert({ res.name, std::move(res) }); }*/
	bool Setup(RenderDevice*);
	bool Compile();
	void Execute();
	inline void AddPass(std::unique_ptr<IRenderPass>&& pass) {
		m_passes.push_back(std::move(pass));
		m_passNameToPtr.insert({ m_passes.back()->Name(), m_passes.back().get() });
	}
public:
	/** 下面的函数是Pass用来说明自身执行时需要使用的资源以及使用方式 */
	/** Pass需要使用顶点缓冲
	 * @param pptr 需要指向顶点缓冲的指针地址 */
	BufferResourceInfo* ReadFromVertexBuffer(IRenderPass* pass);
	/** Pass需要使用索引缓冲
	 * @param pptr 需要指向索引缓冲的指针地址 */
	BufferResourceInfo* ReadFromIndexBuffer(IRenderPass* pass);
	/** Pass需要将结果写入某个RT
	 * @param pptr 需要指向名称位name的RT的指针地址
	 * @param name RT名称
	 * @ramark 注意预设名称 */
	TextureResourceInfo* WriteToRenderTarget(IRenderPass* pass, const std::string& name);
	/** Pass需要将结果写入某个DS 缓冲中
	 * @param pptr 需要指向名称为name的DS缓冲的指针地址 
	 * @param name DS的名称
	 * @param */
	TextureResourceInfo* WriteToDepthStencilBuffer(IRenderPass* pass, const std::string& name);
private:
	TextureResourceInfo& accessTextureResource(const std::string& name);
	/** 辅助函数，根据资源描述创建GPUResource */
	bool createTextureResource(TextureResourceInfo*);
private:
	std::vector<std::unique_ptr<IRenderPass>> m_passes;
	std::unordered_map<std::string, IRenderPass*> m_passNameToPtr;
	
	// 最简单的执行顺序就是线性执行
	// TODO: 暂不支持多Queue并行执行
	std::vector<IRenderPass*> m_executeOrder;
private:
	// 资源使用信息，当前graph登记使用的资源有哪些，每个资源由被谁哪一个pass使用
	std::unordered_map<std::string, TextureResourceInfo> m_textureResources;
	std::unordered_map<std::string, std::unique_ptr<PipelineObject>> m_passNameToPipelineObject;
	BufferResourceInfo m_vtxBuffer;
	BufferResourceInfo m_idxBuffer;
private:
	RenderDevice* m_device;
};

END_NAME_SPACE