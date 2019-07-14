#include "DXCompilerHelper.h"
#include "../FileContainer/FileContainer.h"

#include <d3dcompiler.h>
#include <iostream>
#include <cassert>
#include <strsafe.h>
#include <tchar.h>

#define SmartPtr Microsoft::WRL::ComPtr

#define DXIL_FOURCC(ch0, ch1, ch2, ch3) (                            \
  (uint32_t)(uint8_t)(ch0)        | (uint32_t)(uint8_t)(ch1) << 8  | \
  (uint32_t)(uint8_t)(ch2) << 16  | (uint32_t)(uint8_t)(ch3) << 24   \
  )

/** 字节码文件的文件头 */
struct ByteCodeFileHeader {
	const size_t headerSize = sizeof(ByteCodeFileHeader);
	size_t byteCodeSize;
	char profile[7]; /**< 字节码编译时使用的profile e.g. vs_6_0 */
	ByteCodeFileHeader(size_t byteCodeSize, const char* pf)
		: byteCodeSize(byteCodeSize) {
		memcpy(profile, pf, 7);
	}
	ByteCodeFileHeader() = default;
};

class SafeHandle {
public:
	inline HANDLE get() const { return m_handle; }
	SafeHandle(HANDLE handle) : m_handle(handle) {}
	~SafeHandle() { if (m_handle != INVALID_HANDLE_VALUE) CloseHandle(m_handle); }
	/** 不允许拷贝，修改等操作 */
	SafeHandle(const SafeHandle&) = delete;
	SafeHandle operator=(const SafeHandle&) = delete;
	bool operator==(const HANDLE& handle) const { return m_handle == handle; }
	bool operator==(const SafeHandle& handle) const { return m_handle == handle.m_handle; }
	bool operator!=(const HANDLE& handle) const { return m_handle != handle; }
	bool operator!=(const SafeHandle& handle) const { return m_handle != handle.m_handle; }
	SafeHandle(SafeHandle&& ref) {
		HANDLE temp = ref.m_handle;
		ref.m_handle = m_handle;
		m_handle = temp;
	}
	SafeHandle operator=(SafeHandle&& ref) {
		HANDLE temp = ref.m_handle;
		ref.m_handle = m_handle;
		m_handle = temp;
	}
private:
	HANDLE m_handle;
};


template<typename T, int n>
inline int ArraySize(T(&)[n]) { return n; }

std::wstring fromUTF8ToWideChar(const char* u8) {
	std::wstring wstr(MultiByteToWideChar(CP_UTF8, 0, u8, -1, nullptr, 0), 0);
	MultiByteToWideChar(CP_UTF8, 0, u8, -1, wstr.data(), static_cast<int>(wstr.size()) + 1);
	return wstr;
}

std::string fromWideCharToUTF8(const wchar_t* wc) {
	CHAR tc = '0'; BOOL tb = false;
	std::string str(WideCharToMultiByte(CP_UTF8, 0, wc, -1, nullptr, 0, &tc, &tb), 0);
	WideCharToMultiByte(CP_UTF8, 0, wc, -1, str.data(), static_cast<int>(str.size()) + 1, &tc, &tb);
	return str;
}

DXCompilerHelper::DXCompilerHelper() {
	if (FAILED(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&m_library)))) {
		m_err = "initialize library failed!";
		return;
	}
	if (FAILED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_compiler)))) {
		m_err = "initialize compiler failed!";
		return;
	}
}

void DisplayError(LPTSTR lpszFunction);

Microsoft::WRL::ComPtr<ID3DBlob> DXCompilerHelper::LoadShader(const char * shaderName, const char * profile)
{
	/** TODO: 检查字节文件是否已经存在 */
	/** 需要使用windowsAPI，需要先将字符串相关的内容全部转成unicode(utf-16) */
	int sizeOfName = MultiByteToWideChar(CP_UTF8, 0, shaderName, -1, nullptr, 0);
	std::wstring shaderName_unic(sizeOfName - 1, 0);
	assert(MultiByteToWideChar(CP_UTF8, 0, shaderName, -1, shaderName_unic.data(), sizeOfName) != 0);
	/** @remark: wstring似乎无法像string那样直接使用+进行字符串连接
	 * 即直接连接会保留前一个字符串的null，所以上面创建shaderName的时候-1
	 * 是为了省略null，尔后的append中的空格是给null预留的位置*/
	std::wstring binFile = shaderName_unic;
	std::wstring srcFile = shaderName_unic;
	srcFile.append(L".hlsl ");
	srcFile.back() = 0;
	binFile.append(L".bin ");
	binFile.back() = 0;
	/** 尝试打开源码文件 */
	SafeHandle srcHFile = CreateFileW(srcFile.data(), GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	assert(srcHFile != INVALID_HANDLE_VALUE);
	/** 尝试打开字节码文件 */
	SafeHandle binHFile = CreateFileW(binFile.data(), GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, /**< 默认安全设置 */
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	Microsoft::WRL::ComPtr<ID3DBlob> blob;
	if (binHFile == INVALID_HANDLE_VALUE) {
		/** 字节码文件不存在需要创建并编译 */
		std::cout << "byte code file is not exist!\n";
#ifdef _DEBUG
		assert(CompileToByteCode(srcFile.data(), profile, blob, true) == true);
#else // _DEBUG
		CompileToByteCode(srcFile.data(), profile, blob, false);
#endif // _DEBUG
		/** 保存字节码文件 */
		SafeHandle newBinHFile = CreateFileW(binFile.data(), GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
			CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		assert(newBinHFile != INVALID_HANDLE_VALUE);
		DWORD wordWritten;
		ByteCodeFileHeader header(blob->GetBufferSize(), profile);
		assert(WriteFile(newBinHFile.get(), &header, sizeof(header), &wordWritten, nullptr) != false
			&& wordWritten == sizeof(header));
		assert(WriteFile(newBinHFile.get(), blob->GetBufferPointer(), blob->GetBufferSize(), &wordWritten, nullptr) != false
			&& wordWritten == blob->GetBufferSize());
	}
	else {
		uint64_t binwt, srcwt;
		{
			FILETIME bin_t, src_t; /**< 上次的创建/访问/编辑时间 */
			assert(GetFileTime(srcHFile.get(), nullptr, nullptr, &src_t) != false);
			assert(GetFileTime(binHFile.get(), nullptr, nullptr, &bin_t) != false);
			binwt = (static_cast<uint64_t>(bin_t.dwHighDateTime)) << 32;
			binwt += bin_t.dwLowDateTime;
			srcwt = (static_cast<uint64_t>(src_t.dwHighDateTime)) << 32;
			srcwt += src_t.dwLowDateTime;
		}
		if (srcwt > binwt) {
			std::cout << "byte code file need to be updated!\n";
			/** 字节码不是最新的，重新编译 */
#ifdef _DEBUG
			assert(CompileToByteCode(srcFile.data(), profile, blob, true) == true);
#else // _DEBUG
			CompileToByteCode(srcFile.data(), profile, blob, false);
#endif // _DEBUG
			DWORD wordWritten;
			ByteCodeFileHeader header(blob->GetBufferSize(), profile);
			assert(WriteFile(binHFile.get(), &header, sizeof(header), &wordWritten, nullptr) != false
				&& wordWritten == sizeof(header));
			assert(WriteFile(binHFile.get(), blob->GetBufferPointer(), blob->GetBufferSize(), &wordWritten, nullptr) != false
				&& wordWritten == blob->GetBufferSize());
		}
		else {
			std::cout << "byte code can be used\n";
			/** 直接读取字节码 */
			ByteCodeFileHeader header;
			DWORD wordRead;
			assert(ReadFile(binHFile.get(), &header, sizeof(header), &wordRead, nullptr) != false
				&& wordRead == sizeof(header));
			std::cout << header.byteCodeSize << ' ' << header.profile << '\n';
			assert(SUCCEEDED(D3DCreateBlob(header.byteCodeSize, blob.GetAddressOf())));
			assert(ReadFile(binHFile.get(), blob->GetBufferPointer(), blob->GetBufferSize(), &wordRead, nullptr) != false
				&& wordRead == blob->GetBufferSize());
		}
	}
	return blob;
}

bool DXCompilerHelper::CompileToByteCode(const wchar_t* srcFilePath, const char* profile,
	SmartPtr<ID3DBlob>& outputBuffer, 
	bool outputDebugInfo) {
	if (m_compiler.Get() == nullptr) {
		m_err = "compiler is invalid!";
		return false;
	}
	else if (m_library.Get() == nullptr) {
		m_err = "library is invalid!";
		return false;
	}
	SmartPtr<IDxcBlobEncoding> shaderSrc;
	uint32_t pageCode = CP_UTF8;
	if (FAILED(m_library->CreateBlobFromFile(srcFilePath, &pageCode, shaderSrc.ReleaseAndGetAddressOf()))) {
		m_err = "load shader source file failed!";
		return false;
	}
	std::wstring&& pf = fromUTF8ToWideChar(profile);
	SmartPtr<IDxcOperationResult> compileOpResult;
	SmartPtr<IDxcBlob> debugInfo;
	wchar_t* suggestDebugInfoFileName;
	const wchar_t* args[] = {
		L"/od", L"/Zi", L"/Zss", L"/Fd"
	};
	/** CompileWithDebug返回值与实际的编译结果有出入
	 * 即便编译失败，该函数依然不会返回错误状态 */
	if (FAILED(m_compiler->CompileWithDebug(shaderSrc.Get(), srcFilePath, L"main", pf.data(),
		&args[0], outputDebugInfo ? ArraySize(args) : 0, nullptr, 0,
		nullptr, compileOpResult.ReleaseAndGetAddressOf(),
		&suggestDebugInfoFileName, debugInfo.ReleaseAndGetAddressOf()))) {
		SmartPtr<IDxcBlobEncoding> errBlob;
		compileOpResult->GetErrorBuffer(errBlob.ReleaseAndGetAddressOf());
		m_err = fromWideCharToUTF8(reinterpret_cast<const wchar_t*>(errBlob->GetBufferPointer()));
		return false;
	}
	/** 检查编译结果 */
	HRESULT compileStatus;
	compileOpResult->GetStatus(&compileStatus);
	if (FAILED(compileStatus)) {
		SmartPtr<IDxcBlobEncoding> errBlob;
		compileOpResult->GetErrorBuffer(errBlob.ReleaseAndGetAddressOf());
		m_err = fromWideCharToUTF8(reinterpret_cast<const wchar_t*>(errBlob->GetBufferPointer()));
		return false;
	}
	SmartPtr<IDxcBlob> byteCodes;
	compileOpResult->GetResult(byteCodes.ReleaseAndGetAddressOf());
	
	byteCodes.As(&outputBuffer);
	if (outputDebugInfo) {
		std::string&& df = fromWideCharToUTF8(suggestDebugInfoFileName);
		FileContainer debugInfoFile(df.data(), std::ios::out | std::ios::trunc);
		debugInfoFile.WriteFile(0, static_cast<uint32_t>(debugInfo->GetBufferSize()), reinterpret_cast<const char*>(debugInfo->GetBufferPointer()));
	}
	return true;
}

auto DXCompilerHelper::RetrieveShaderDescriptionFromByteCode(SmartPtr<ID3DBlob>& byteCodes)
	-> Microsoft::WRL::ComPtr<ID3D12ShaderReflection> {
	SmartPtr<ID3D12ShaderReflection> shrReflect;
	/** 加载byte code并创建用于获取shader描述信息的对象 */
	SmartPtr<IDxcBlob> dxcBlob;
	if (FAILED(byteCodes.As(&dxcBlob))) {
		m_err = "create blob failed!";
		return shrReflect;
	}
	SmartPtr<IDxcContainerReflection> reflection;
	uint32_t shaderIdx;
	DxcCreateInstance(CLSID_DxcContainerReflection, IID_PPV_ARGS(&reflection));
	if (FAILED(reflection->Load(dxcBlob.Get()))) {
		m_err = "load blob failed!";
		return shrReflect;
	}
	uint32_t dxil = DXIL_FOURCC('D', 'X', 'I', 'L');
	if (FAILED(reflection->FindFirstPartKind(dxil, &shaderIdx))) {
		m_err = "find first part of dxil failed!";
		return shrReflect;
	}
	if (FAILED(reflection->GetPartReflection(shaderIdx, IID_PPV_ARGS(&shrReflect)))) {
		m_err = "Get shader reflection failed";
		return shrReflect;
	}

	return shrReflect;
}

void DisplayError(LPTSTR lpszFunction)
// Routine Description:
// Retrieve and output the system error message for the last-error code
{
	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0,
		NULL);

	lpDisplayBuf =
		(LPVOID)LocalAlloc(LMEM_ZEROINIT,
		(lstrlen((LPCTSTR)lpMsgBuf)
			+ lstrlen((LPCTSTR)lpszFunction)
			+ 40) // account for format string
			* sizeof(TCHAR));

	if (FAILED(StringCchPrintf((LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error code %d as follows:\n%s"),
		lpszFunction,
		dw,
		lpMsgBuf)))
	{
		printf("FATAL ERROR: Unable to output error code.\n");
	}

	_tprintf(TEXT("ERROR: %s\n"), (LPCTSTR)lpDisplayBuf);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
}
