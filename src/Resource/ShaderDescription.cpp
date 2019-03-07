#include "ShaderDescription.h"
#include <iostream>

namespace UnknownVision {
	void PrintShaderDescriptionToConsole(const ShaderDescription & desc) {
		std::cout << "Shader Info: \n"
			<< "\tConstant Buffers(" << desc.NumberOfConstantBuffers() << "): \n";
		for (const auto& cb : desc.constantBuffers) {
			std::cout << "\t  name: " << cb.first << '\n'
				<< "\t\tslot: " << (uint32_t)cb.second.slot << '\n'
				<< "\t\tspace: " << (uint32_t)cb.second.space << '\n'
				<< "\t\tarray size: " << (uint32_t)cb.second.arraySize << '\n';
		}
		std::cout << "\tShader Resources(" << desc.NumberOfShaderResources() << "): \n";
		for (const auto& sr : desc.shaderResources) {
			std::cout << "\t  name: " << sr.first << '\n'
				<< "\t\tslot: " << (uint32_t)sr.second.slot << '\n'
				<< "\t\tspace: " << (uint32_t)sr.second.space << '\n'
				<< "\t\tarray size: " << (uint32_t)sr.second.arraySize << '\n';
		}
		std::cout << "\tSamplers(" << desc.NumberOfSamplers() << "): \n";
		for (const auto& sp : desc.samplers) {
			std::cout << "\t  name: " << sp.first << '\n'
				<< "\t\tslot: " << (uint32_t)sp.second.slot << '\n'
				<< "\t\tspace: " << (uint32_t)sp.second.space << '\n'
				<< "\t\tarray size: " << (uint32_t)sp.second.arraySize << '\n';
		}
		std::cout << "\tUnorder Access(" << desc.NumberOfUnoderAccessBuffers() << "): \n";
		for (const auto& ua : desc.unorderAccessBuffers) {
			std::cout << "\t  name: " << ua.first << '\n'
				<< "\t\tslot: " << (uint32_t)ua.second.slot << '\n'
				<< "\t\tspace: " << (uint32_t)ua.second.space << '\n'
				<< "\t\tarray size: " << (uint32_t)ua.second.arraySize << '\n';
		}
	}
}
