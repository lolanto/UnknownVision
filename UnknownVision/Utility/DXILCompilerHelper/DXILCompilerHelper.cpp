#include "DXILCompilerHelper.h"
#include "../FileContainer/FileContainer.h"

#define SmartPtr Microsoft::WRL::ComPtr

template<typename T, int n>
inline void setupErrMsg(T(& msg)[n], std::vector<char>* container) {
	container->resize(n);
	memcpy(container->data(), msg, n);
}

template<typename T, int n>
inline int ArraySize(T(&)[n]) { return n; }

std::vector<wchar_t> fromUTF8ToWideChar(const char* u8) {
	std::vector<wchar_t> wstr(MultiByteToWideChar(CP_UTF8, 0, u8, -1, nullptr, 0));
	MultiByteToWideChar(CP_UTF8, 0, u8, -1, wstr.data(), wstr.size());
	return wstr;
}

std::vector<char> fromWideCharToUTF8(const wchar_t* wc) {
	CHAR tc = '0'; BOOL tb = false;
	std::vector<char> str(WideCharToMultiByte(CP_UTF8, 0, wc, -1, nullptr, 0, &tc, &tb));
	WideCharToMultiByte(CP_UTF8, 0, wc, -1, str.data(), str.size(), &tc, &tb);
	return str;
}

DXILCompilerHelper::DXILCompilerHelper(std::vector<char>* err) {
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

bool DXILCompilerHelper::CompileToByteCode(const char* srcFilePath, const char* profile,
	std::vector<uint8_t>& outputBuffer, 
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
		L"/Zi", L"/Zss", L"/Fd"
	};
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
	SmartPtr<IDxcBlob> byteCodes;
	compileOpResult->GetResult(byteCodes.ReleaseAndGetAddressOf());
	outputBuffer.resize(byteCodes->GetBufferSize());
	memcpy(outputBuffer.data(), byteCodes->GetBufferPointer(), byteCodes->GetBufferSize());
	if (outputDebugInfo) {
		std::vector<char>&& df = fromWideCharToUTF8(suggestDebugInfoFileName);
		FileContainer debugInfoFile(df.data(), std::ios::out | std::ios::trunc);
		debugInfoFile.WriteFile(0, debugInfo->GetBufferSize(), reinterpret_cast<const char*>(debugInfo->GetBufferPointer()));
	}
	return true;
}
