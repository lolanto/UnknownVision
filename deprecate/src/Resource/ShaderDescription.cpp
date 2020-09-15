#include "ShaderDescription.h"
#include <iostream>
#include <vector>
namespace UnknownVision {
	void ShaderDescription::PrintShaderDescriptionToConsole() {
		std::vector<const ResourceDescriptor*> cbs;
		std::vector<const ResourceDescriptor*> srs;
		std::vector<const ResourceDescriptor*> sms;
		std::vector<const ResourceDescriptor*> uas;
		for (const auto& resIter : m_resourceDescriptiors) {
			switch (resIter.second.type) {
			case ResourceDescriptor::REGISTER_TYPE_CONSTANT_BUFFER:
				cbs.push_back(&resIter.second);
				break;
			case ResourceDescriptor::REGISTER_TYPE_SAMPLER:
				sms.push_back(&resIter.second);
				break;
			case ResourceDescriptor::REGISTER_TYPE_SHADER_RESOURCE:
				srs.push_back(&resIter.second);
				break;
			case ResourceDescriptor::REGISTER_TYPE_UNORDER_ACCESS:
				uas.push_back(&resIter.second);
				break;
			}
		}
		std::cout << "Shader Info: \n"
			<< "\tConstant Buffers(" << NumberOfConstantBuffers() << "): \n";
		for (const auto& cb : cbs) {
			std::cout << "\t  name: " << cb->name << '\n'
				<< "\t\tslot: " << (uint32_t)cb->slot << '\n'
				<< "\t\tspace: " << (uint32_t)cb->space << '\n'
				<< "\t\tarray size: " << (uint32_t)cb->arraySize << '\n';
		}
		std::cout << "\tShader Resources(" << NumberOfShaderResources() << "): \n";
		for (const auto& sr : srs) {
			std::cout << "\t  name: " << sr->name << '\n'
				<< "\t\tslot: " << (uint32_t)sr->slot << '\n'
				<< "\t\tspace: " << (uint32_t)sr->space << '\n'
				<< "\t\tarray size: " << (uint32_t)sr->arraySize << '\n';
		}
		std::cout << "\tSamplers(" << NumberOfSamplers() << "): \n";
		for (const auto& sp : sms) {
			std::cout << "\t  name: " << sp->name << '\n'
				<< "\t\tslot: " << (uint32_t)sp->slot << '\n'
				<< "\t\tspace: " << (uint32_t)sp->space << '\n'
				<< "\t\tarray size: " << (uint32_t)sp->arraySize << '\n';
		}
		std::cout << "\tUnorder Access(" << NumberOfUnoderAccessBuffers() << "): \n";
		for (const auto& ua : uas) {
			std::cout << "\t  name: " << ua->name << '\n'
				<< "\t\tslot: " << (uint32_t)ua->slot << '\n'
				<< "\t\tspace: " << (uint32_t)ua->space << '\n'
				<< "\t\tarray size: " << (uint32_t)ua->arraySize << '\n';
		}
	}

	void ShaderDescription::InsertResourceDescription(const ResourceDescriptor & desc)
	{
		auto& descIter = m_resourceDescriptiors.find(desc.name);
		if (descIter == m_resourceDescriptiors.end()) {
			/** 当前还没有对应资源的描述信息 */
			m_resourceDescriptiors.insert(std::make_pair(desc.name, desc));
		}
		else {
			/** 当前已经有对应的描述信息，覆盖原有信息 */
			descIter->second = desc;
		}
		switch (desc.type) {
		case ResourceDescriptor::REGISTER_TYPE_CONSTANT_BUFFER:
			++m_numOfConstantBuffers;
			break;
		case ResourceDescriptor::REGISTER_TYPE_SAMPLER:
			++m_numOfSamplers;
			break;
		case ResourceDescriptor::REGISTER_TYPE_SHADER_RESOURCE:
			++m_numOfShaderResources;
			break;
		case ResourceDescriptor::REGISTER_TYPE_UNORDER_ACCESS:
			++m_numOfUnoderAccessBuffers;
			break;
		}
	}
	const ResourceDescriptor & ShaderDescription::GetResourceDescription(const std::string & name)
	{
		const auto& descIter = m_resourceDescriptiors.find(name);
		if (descIter == m_resourceDescriptiors.end()) {
			/** 当前没有找到对应名称的资源 */
			throw(std::out_of_range("invalid resource name"));
		}
		return descIter->second;
	}
}
