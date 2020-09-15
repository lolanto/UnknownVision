#pragma once
#include "../UVConfig.h"
#include "../UVType.h"
#include <string>
#include <unordered_set>
#include <vector>

BEG_NAME_SPACE

class RenderGraph;

struct TextureDescriptions {
	ResourceUsages usage;
	ElementFormatType format;
	SizeClass size;
	size_t sizeX, sizeY, sizeZ;
	TextureDescriptions() = default;
};

struct BufferDescriptions {
	ResourceUsages usage;
	ElementFormatType format;
	size_t size;
	BufferDescriptions() = default;
};

class ResourceInfo {
	friend class RenderGraph;
public:
	ResourceInfo(const std::string& name_) : m_name(name_), m_usages(RESOURCE_USAGE_INVALID), m_transient(true) {}
	virtual ~ResourceInfo() = default;
public:
	bool IsPermanent() const { return !m_transient; }
	GPUResource* GetResource() { return m_resource; }
protected:
	void WriteInPass(IRenderPass* pass) { m_writeInPasses.insert(pass); }
	void ReadInPass(IRenderPass* pass) { m_readInPasses.insert(pass); }
	const std::unordered_set<IRenderPass*> GetWriteInPasses() const { return m_writeInPasses; }
	const std::unordered_set<IRenderPass*> GetReadInPasses() const { return m_readInPasses; }
	void AddUsage(ResourceUsages usage) { m_usages |= usage; }
	void SetPermanent() { m_transient = false; }
protected:
	std::unordered_set<IRenderPass*> m_writeInPasses;
	std::unordered_set<IRenderPass*> m_readInPasses;
	std::string m_name;
	ResourceUsages m_usages;
	GPUResource* m_resource;
	bool m_transient;
};

class TextureResourceInfo : public ResourceInfo {
	friend class RenderGraph;
public:
	TextureResourceInfo CreateTexture2D(size_t width, size_t height, ElementFormatType format, const std::string& name) {
		TextureResourceInfo info;
		info.m_name = name;
		info.m_format = format;
		info.m_sizeX = width;
		info.m_sizeY = height;
		return info;
	}
public:
	TextureResourceInfo() : ResourceInfo("") {}
private:
	ElementFormatType m_format;
	size_t m_sizeX, m_sizeY, m_sizeZ;
};

class BufferResourceInfo : public ResourceInfo {
	friend class RenderGraph;
public:
	BufferResourceInfo() : ResourceInfo("") {}
	BufferResourceInfo(size_t capcity, ElementFormatType format, const std::string& name)
		: m_capacity(capcity), m_format(format), ResourceInfo(name) {}
private:
	ElementFormatType m_format;
	size_t m_capacity;
};

class IRenderPass {
public:
	struct BindingInfo {
		size_t slot;
		ShaderParameterType paraType;
		ShaderType shaderType;
	};
public:
	static const size_t INVALID_INDEX = SIZE_MAX;
public:
	IRenderPass(const std::string& name) : m_passName(name), m_index(INVALID_INDEX) {}
	virtual ~IRenderPass() = default;
	const std::string Name() const { return m_passName; }
	const size_t& Index() const { return m_index; }
	size_t& Index() { return m_index; }
public:
	virtual void Setup(RenderGraph*) = 0;
	virtual void Exec(RenderGraph*) = 0;
	virtual const std::vector<TextureResourceInfo*>* GetColorOutputs() const { return nullptr; }
	virtual TextureResourceInfo* GetDepthStencilOutput() const { return nullptr; }
public:
	virtual ProgramType Type() const = 0;
	virtual std::string VSName() const { return ""; }
	virtual std::string PSName() const { return ""; }
	virtual std::string GSName() const { return ""; }
	virtual std::string CSName() const { return ""; }
	const std::vector<BindingInfo>& GetInfos() const { return m_infos; }
protected:
	std::string m_passName;
	size_t m_index;
	std::vector<BindingInfo> m_infos;
};



END_NAME_SPACE
