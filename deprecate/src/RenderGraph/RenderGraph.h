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


/** RenderGraph��ְ��:
 * 1. ������Դ���������ں�״̬����
 * 2. Э��pass��ִ��˳�� */

class RenderGraph : public Uncopyable {
public:
	RenderGraph() = default;
	~RenderGraph() = default;
public:
	/** ��Graph�����Դ����Ϣ����Щ��Դ�ᱻһ������passʹ��(��������ʱ��Դ) */
	/*void AddResource(ResourceInfo&& res) { m_accessedResources.insert({ res.name, std::move(res) }); }*/
	bool Setup(RenderDevice*);
	bool Compile();
	void Execute();
	inline void AddPass(std::unique_ptr<IRenderPass>&& pass) {
		m_passes.push_back(std::move(pass));
		m_passNameToPtr.insert({ m_passes.back()->Name(), m_passes.back().get() });
	}
public:
	/** ����ĺ�����Pass����˵������ִ��ʱ��Ҫʹ�õ���Դ�Լ�ʹ�÷�ʽ */
	/** Pass��Ҫʹ�ö��㻺��
	 * @param pptr ��Ҫָ�򶥵㻺���ָ���ַ */
	BufferResourceInfo* ReadFromVertexBuffer(IRenderPass* pass);
	/** Pass��Ҫʹ����������
	 * @param pptr ��Ҫָ�����������ָ���ַ */
	BufferResourceInfo* ReadFromIndexBuffer(IRenderPass* pass);
	/** Pass��Ҫ�����д��ĳ��RT
	 * @param pptr ��Ҫָ������λname��RT��ָ���ַ
	 * @param name RT����
	 * @ramark ע��Ԥ������ */
	TextureResourceInfo* WriteToRenderTarget(IRenderPass* pass, const std::string& name);
	/** Pass��Ҫ�����д��ĳ��DS ������
	 * @param pptr ��Ҫָ������Ϊname��DS�����ָ���ַ 
	 * @param name DS������
	 * @param */
	TextureResourceInfo* WriteToDepthStencilBuffer(IRenderPass* pass, const std::string& name);
private:
	TextureResourceInfo& accessTextureResource(const std::string& name);
	/** ����������������Դ��������GPUResource */
	bool createTextureResource(TextureResourceInfo*);
private:
	std::vector<std::unique_ptr<IRenderPass>> m_passes;
	std::unordered_map<std::string, IRenderPass*> m_passNameToPtr;
	
	// ��򵥵�ִ��˳���������ִ��
	// TODO: �ݲ�֧�ֶ�Queue����ִ��
	std::vector<IRenderPass*> m_executeOrder;
private:
	// ��Դʹ����Ϣ����ǰgraph�Ǽ�ʹ�õ���Դ����Щ��ÿ����Դ�ɱ�˭��һ��passʹ��
	std::unordered_map<std::string, TextureResourceInfo> m_textureResources;
	std::unordered_map<std::string, std::unique_ptr<PipelineObject>> m_passNameToPipelineObject;
	BufferResourceInfo m_vtxBuffer;
	BufferResourceInfo m_idxBuffer;
private:
	RenderDevice* m_device;
};

END_NAME_SPACE