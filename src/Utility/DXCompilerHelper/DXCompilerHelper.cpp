#include "DXCompilerHelper.h"
#include "../FileContainer/FileContainer.h"
#include "../../Resource/ShaderDescription.h"

#include <d3d12shader.h>
#include <d3dcompiler.h>
#include <iostream>

#define SmartPtr Microsoft::WRL::ComPtr

#define DXIL_FOURCC(ch0, ch1, ch2, ch3) (                            \
  (uint32_t)(uint8_t)(ch0)        | (uint32_t)(uint8_t)(ch1) << 8  | \
  (uint32_t)(uint8_t)(ch2) << 16  | (uint32_t)(uint8_t)(ch3) << 24   \
  )

using UnknownVision::ResourceDescriptor;
using UnknownVision::ShaderDescription;

template<typename T, int n>
inline void setupErrMsg(T(& msg)[n], std::vector<char>* container) {
	container->resize(n);
	memcpy(container->data(), msg, n);
}

template<typename T, int n>
inline int ArraySize(T(&)[n]) { return n; }

std::vector<wchar_t> fromUTF8ToWideChar(const char* u8) {
	std::vector<wchar_t> wstr(MultiByteToWideChar(CP_UTF8, 0, u8, -1, nullptr, 0));
	MultiByteToWideChar(CP_UTF8, 0, u8, -1, wstr.data(), static_cast<int>(wstr.size()));
	return wstr;
}

std::vector<char> fromWideCharToUTF8(const wchar_t* wc) {
	CHAR tc = '0'; BOOL tb = false;
	std::vector<char> str(WideCharToMultiByte(CP_UTF8, 0, wc, -1, nullptr, 0, &tc, &tb));
	WideCharToMultiByte(CP_UTF8, 0, wc, -1, str.data(), static_cast<int>(str.size()), &tc, &tb);
	return str;
}

DXCompilerHelper::DXCompilerHelper(std::vector<char>* err) {
	if (FAILED(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&m_library)))) {
		if (err) {
			setupErrMsg("initialize library failed!", err);
		}
		return;
	}
	if (FAILED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_compiler)))) {
		if (err) {
			setupErrMsg("initialize compiler failed!", err);
		}
		return;
	}
}

bool DXCompilerHelper::CompileToByteCode(const char* srcFilePath, const char* profile,
	SmartPtr<ID3DBlob>& outputBuffer, 
	bool outputDebugInfo, std::vector<char>* err) {
	if (m_compiler.Get() == nullptr) {
		if (err) {
			setupErrMsg("compiler is invalid!", err);
		}
		return false;
	}
	else if (m_library.Get() == nullptr) {
		if (err) {
			setupErrMsg("library is invalid!", err);
		}
		return false;
	}
	SmartPtr<IDxcBlobEncoding> shaderSrc;
	std::vector<wchar_t>&& fp = fromUTF8ToWideChar(srcFilePath);
	uint32_t pageCode = CP_UTF8;
	if (FAILED(m_library->CreateBlobFromFile(fp.data(), &pageCode, shaderSrc.ReleaseAndGetAddressOf()))) {
		if (err) {
			setupErrMsg("load shader source file failed!", err);
		}
		return false;
	}
	std::vector<wchar_t>&& pf = fromUTF8ToWideChar(profile);
	SmartPtr<IDxcOperationResult> compileOpResult;
	SmartPtr<IDxcBlob> debugInfo;
	wchar_t* suggestDebugInfoFileName;
	const wchar_t* args[] = {
		L"/od", L"/Zi", L"/Zss", L"/Fd"
	};
	/** CompileWithDebug返回值与实际的编译结果有出入
	 * 即便编译失败，该函数依然不会返回错误状态 */
	if (FAILED(m_compiler->CompileWithDebug(shaderSrc.Get(), fp.data(), L"main", pf.data(),
		&args[0], outputDebugInfo ? ArraySize(args) : 0, nullptr, 0,
		nullptr, compileOpResult.ReleaseAndGetAddressOf(),
		&suggestDebugInfoFileName, debugInfo.ReleaseAndGetAddressOf()))) {
		if (err) {
			SmartPtr<IDxcBlobEncoding> errBlob;
			compileOpResult->GetErrorBuffer(errBlob.ReleaseAndGetAddressOf());
			*err = fromWideCharToUTF8(reinterpret_cast<const wchar_t*>(errBlob->GetBufferPointer()));
		}
		return false;
	}
	/** 检查编译结果 */
	HRESULT compileStatus;
	compileOpResult->GetStatus(&compileStatus);
	if (FAILED(compileStatus)) {
		if (err) {
			SmartPtr<IDxcBlobEncoding> errBlob;
			compileOpResult->GetErrorBuffer(errBlob.ReleaseAndGetAddressOf());
			uint32_t codePage = 0;
			BOOL knownPage = false;
			errBlob->GetEncoding(&knownPage, &codePage);
			if (codePage == 65001) {
				err->resize(errBlob->GetBufferSize() + 1);
				memcpy(err->data(), errBlob->GetBufferPointer(), errBlob->GetBufferSize());
				err->back() = 0;
			}
			else {
				*err = fromWideCharToUTF8(reinterpret_cast<const wchar_t*>(errBlob->GetBufferPointer()));
			}
		}
		return false;
	}
	SmartPtr<IDxcBlob> byteCodes;
	compileOpResult->GetResult(byteCodes.ReleaseAndGetAddressOf());
	
	byteCodes.As(&outputBuffer);
	if (outputDebugInfo) {
		std::vector<char>&& df = fromWideCharToUTF8(suggestDebugInfoFileName);
		FileContainer debugInfoFile(df.data(), std::ios::out | std::ios::trunc);
		debugInfoFile.WriteFile(0, static_cast<uint32_t>(debugInfo->GetBufferSize()), reinterpret_cast<const char*>(debugInfo->GetBufferPointer()));
	}
	return true;
}

bool DXCompilerHelper::RetrieveShaderDescriptionFromByteCode(SmartPtr<ID3DBlob>& byteCodes,
	UnknownVision::ShaderDescription& outputDescription,
	std::vector<char>* err) {
	/** 加载byte code并创建用于获取shader描述信息的对象 */
	SmartPtr<IDxcBlob> dxcBlob;
	if (FAILED(byteCodes.As(&dxcBlob))) {
		if (err) {
			setupErrMsg("create blob failed!", err);
		}
		return false;
	}
	SmartPtr<IDxcContainerReflection> reflection;
	uint32_t shaderIdx;
	DxcCreateInstance(CLSID_DxcContainerReflection, IID_PPV_ARGS(&reflection));
	if (FAILED(reflection->Load(dxcBlob.Get()))) {
		if (err) {
			setupErrMsg("load blob failed!", err);
		}
		return false;
	}
	uint32_t dxil = DXIL_FOURCC('D', 'X', 'I', 'L');
	if (FAILED(reflection->FindFirstPartKind(dxil, &shaderIdx))) {
		if (err) {
			setupErrMsg("find first part of dxil failed!", err);
		}
		return false;
	}
	SmartPtr<ID3D12ShaderReflection> shrReflect;
	if (FAILED(reflection->GetPartReflection(shaderIdx, IID_PPV_ARGS(&shrReflect)))) {
		if (err) {
			setupErrMsg("Get shader reflection failed", err);
		}
		return false;
	}

	D3D12_SHADER_DESC shaderDesc;
	if (FAILED(shrReflect->GetDesc(&shaderDesc))) {
		if (err) {
			setupErrMsg("get shader description failed!", err);
		}
		return false;
	}
	/** 开始填充描述结构体 */
	{
		ShaderDescription::ResourceDescriptorMap cbMap;
		ShaderDescription::ResourceDescriptorMap srMap;
		ShaderDescription::ResourceDescriptorMap spMap;
		ShaderDescription::ResourceDescriptorMap uaMap;
		for (uint32_t resIdx = 0; resIdx < shaderDesc.BoundResources; ++resIdx) {
			ResourceDescriptor resDesc;
			D3D12_SHADER_INPUT_BIND_DESC inputDesc;
			shrReflect->GetResourceBindingDesc(resIdx, &inputDesc);

			resDesc.slot = inputDesc.BindPoint;
			resDesc.isArray = inputDesc.BindCount > 1;
			resDesc.arraySize = inputDesc.BindCount;
			resDesc.name = inputDesc.Name;
			resDesc.space = inputDesc.Space;

			switch (inputDesc.Type) {
			case D3D_SIT_CBUFFER:
				resDesc.type = ResourceDescriptor::REGISTER_TYPE_CONSTANT_BUFFER;
				cbMap.insert(std::make_pair(resDesc.name, resDesc));
				break;
			case D3D_SIT_TEXTURE:
				resDesc.type = ResourceDescriptor::REGISTER_TYPE_SHADER_RESOURCE;
				srMap.insert(std::make_pair(resDesc.name, resDesc));
				break;
			case D3D_SIT_SAMPLER:
				resDesc.type = ResourceDescriptor::REGISTER_TYPE_SAMPLER;
				spMap.insert(std::make_pair(resDesc.name, resDesc));
				break;
			case D3D_SIT_UAV_RWTYPED:
				resDesc.type = ResourceDescriptor::REGISTER_TYPE_UNORDER_ACCESS;
				uaMap.insert(std::make_pair(resDesc.name, resDesc));
				break;
			default:
				if (err) {
					setupErrMsg("discover a resource type doesn't support!", err);
				}
				return false;
			}
		}
		outputDescription.constantBuffers.swap(cbMap);
		outputDescription.samplers.swap(spMap);
		outputDescription.shaderResources.swap(srMap);
		outputDescription.unorderAccessBuffers.swap(uaMap);
	}

	return true;
}
