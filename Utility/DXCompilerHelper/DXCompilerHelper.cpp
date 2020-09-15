#include "DXCompilerHelper.h"

#include <d3dcompiler.h> /**< 提供FXC方法 */
#include <iostream>
#include <cassert>
#include <strsafe.h>
#include <tchar.h>
#include <set>
#include <mutex>
#include <filesystem>

std::set<std::string> ProcessingShaders; /**< 存储当前正在处理的shader名称，用于防止相同的shader被重复Load */
std::mutex GlobalShaderLock;
std::condition_variable GlobalShaderCV;
/** 以RAII的方式确保相同shader的load过程可以串行进行 */
struct ShaderProcessingMutex {
	const std::string shaderName;
	ShaderProcessingMutex(const char* shaderName)
		: shaderName(shaderName) {
		std::unique_lock<std::mutex> lu(GlobalShaderLock);
		GlobalShaderCV.wait(lu, [&shaderName]() {
			return ProcessingShaders.find(shaderName) == ProcessingShaders.end();
		});
		ProcessingShaders.insert(shaderName);
	}
	~ShaderProcessingMutex() {
		std::unique_lock<std::mutex> lu(GlobalShaderLock);
		ProcessingShaders.erase(shaderName);
		lu.unlock();
		GlobalShaderCV.notify_all();
	}
};

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

///** 输入(路径)文件名称(不带类型)，返回添加bin后缀的(路径)文件名称 */
//std::wstring generateBinFileName(const char* fileName) {
//	/** 需要使用windowsAPI，需要先将字符串相关的内容全部转成unicode(utf-16) */
//	int sizeOfName = MultiByteToWideChar(CP_UTF8, 0, fileName, -1, nullptr, 0);
//	std::wstring fileName_unic(sizeOfName - 1, 0);
//	assert(MultiByteToWideChar(CP_UTF8, 0, fileName, -1,fileName_unic.data(), sizeOfName) != 0);
//	/** @remark: wstring似乎无法像string那样直接使用+进行字符串连接
//	 * 即直接连接会保留前一个字符串的null，所以上面创建shaderName的时候-1
//	 * 是为了省略null，尔后的append中的空格是给null预留的位置*/
//	std::wstring binFile = fileName_unic;
//	binFile.append(L".bin ");
//	binFile.back() = 0;
//	return binFile;
//}
///** 输入(路径)文件名称(不带类型)，返回添加hlsl后缀的(路径)文件名称 */
//std::wstring generateSrcFileName(const char* fileName) {
//	/** 需要使用windowsAPI，需要先将字符串相关的内容全部转成unicode(utf-16) */
//	int sizeOfName = MultiByteToWideChar(CP_UTF8, 0, fileName, -1, nullptr, 0);
//	std::wstring fileName_unic(sizeOfName - 1, 0);
//	assert(MultiByteToWideChar(CP_UTF8, 0, fileName, -1, fileName_unic.data(), sizeOfName) != 0);
//	/** @remark: wstring似乎无法像string那样直接使用+进行字符串连接
//	 * 即直接连接会保留前一个字符串的null，所以上面创建shaderName的时候-1
//	 * 是为了省略null，尔后的append中的空格是给null预留的位置*/
//	std::wstring srcFile = fileName_unic;
//	srcFile.append(L".hlsl ");
//	srcFile.back() = 0;
//	return srcFile;
//}

DXCompilerHelper::DXCompilerHelper() {
	if (FAILED(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&m_utils)))) {
		m_err = "initialize library failed!";
		return;
	}
	if (FAILED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_compiler)))) {
		m_err = "initialize compiler failed!";
		return;
	}
}

//auto DXCompilerHelper::TimeStampOfShaderSourceCode(const char * shaderName) -> std::pair<uint64_t, bool>
//{
//	std::wstring&& srcFile = generateSrcFileName(shaderName);
//	/** 尝试打开源码文件 */
//	SafeHandle srcHFile = CreateFileW(srcFile.data(), 0, /**< 设置为0，仅读取文件属性不访问内容 */
//		FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
//		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
//	if (srcHFile == INVALID_HANDLE_VALUE) return { 0, false }; /**< 文件不存在 */
//
//	FILETIME src_t; /**< 源码文件上一次修改时间 */
//	assert(GetFileTime(srcHFile.get(), nullptr, nullptr, &src_t) != false);
//	uint64_t timestamp = static_cast<uint64_t>(src_t.dwHighDateTime);
//	timestamp = (timestamp << 32) + static_cast<uint64_t>(src_t.dwLowDateTime);
//	return { timestamp, true };
//}

void DisplayError(LPTSTR lpszFunction);

//Microsoft::WRL::ComPtr<ID3DBlob> DXCompilerHelper::LoadShader(const char * shaderName, const char * profile)
//{
//	ShaderProcessingMutex spm(shaderName);
//	std::wstring&& binFile = generateBinFileName(shaderName);
//	std::wstring&& srcFile = generateSrcFileName(shaderName);
//
//	/** 尝试打开源码文件 */
//	SafeHandle srcHFile = CreateFileW(srcFile.data(), GENERIC_READ,
//		FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
//		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
//	assert(srcHFile != INVALID_HANDLE_VALUE);
//	/** 尝试打开字节码文件 */
//	SafeHandle binHFile = CreateFileW(binFile.data(), GENERIC_READ | GENERIC_WRITE,
//		FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, /**< 默认安全设置 */
//		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
//	Microsoft::WRL::ComPtr<ID3DBlob> blob;
//	if (binHFile == INVALID_HANDLE_VALUE) {
//		/** 字节码文件不存在需要创建并编译 */
//#ifdef _DEBUG
//		if (CompileToByteCode(srcFile.data(), profile, blob, true) == false)
//			return blob;
//#else // _DEBUG
//		CompileToByteCode(srcFile.data(), profile, blob, false);
//#endif // _DEBUG
//		/** 保存字节码文件 */
//		SafeHandle newBinHFile = CreateFileW(binFile.data(), GENERIC_READ | GENERIC_WRITE,
//			FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
//			CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
//		assert(newBinHFile != INVALID_HANDLE_VALUE);
//		DWORD wordWritten;
//		ByteCodeFileHeader header(blob->GetBufferSize(), profile);
//		assert(WriteFile(newBinHFile.get(), &header, sizeof(header), &wordWritten, nullptr) != false
//			&& wordWritten == sizeof(header));
//		assert(WriteFile(newBinHFile.get(), blob->GetBufferPointer(), blob->GetBufferSize(), &wordWritten, nullptr) != false
//			&& wordWritten == blob->GetBufferSize());
//	}
//	else {
//		uint64_t binwt, srcwt;
//		{
//			FILETIME bin_t, src_t; /**< 上次的创建/访问/编辑时间 */
//			assert(GetFileTime(srcHFile.get(), nullptr, nullptr, &src_t) != false);
//			assert(GetFileTime(binHFile.get(), nullptr, nullptr, &bin_t) != false);
//			binwt = (static_cast<uint64_t>(bin_t.dwHighDateTime)) << 32;
//			binwt += bin_t.dwLowDateTime;
//			srcwt = (static_cast<uint64_t>(src_t.dwHighDateTime)) << 32;
//			srcwt += src_t.dwLowDateTime;
//		}
//		if (srcwt > binwt) {
//			/** 字节码不是最新的，重新编译 */
//#ifdef _DEBUG
//			if (CompileToByteCode(srcFile.data(), profile, blob, true) == false)
//				return blob;
//#else // _DEBUG
//			CompileToByteCode(srcFile.data(), profile, blob, false);
//#endif // _DEBUG
//			DWORD wordWritten;
//			ByteCodeFileHeader header(blob->GetBufferSize(), profile);
//			assert(WriteFile(binHFile.get(), &header, sizeof(header), &wordWritten, nullptr) != false
//				&& wordWritten == sizeof(header));
//			assert(WriteFile(binHFile.get(), blob->GetBufferPointer(), blob->GetBufferSize(), &wordWritten, nullptr) != false
//				&& wordWritten == blob->GetBufferSize());
//		}
//		else {
//			/** 直接读取字节码 */
//			ByteCodeFileHeader header;
//			DWORD wordRead;
//			assert(ReadFile(binHFile.get(), &header, sizeof(header), &wordRead, nullptr) != false
//				&& wordRead == sizeof(header));
//			assert(SUCCEEDED(D3DCreateBlob(header.byteCodeSize, blob.GetAddressOf())));
//			assert(ReadFile(binHFile.get(), blob->GetBufferPointer(), blob->GetBufferSize(), &wordRead, nullptr) != false
//				&& wordRead == blob->GetBufferSize());
//		}
//	}
//	return blob;
//}

class MIncludeHandler : public IDxcIncludeHandler {
public:
	MIncludeHandler(IDxcUtils* utils, std::filesystem::path rootPath) : utils(utils), rootPath(rootPath) {}
	virtual ~MIncludeHandler() = default;
	HRESULT LoadSource(const wchar_t* path, IDxcBlob** pptr) override{
		std::filesystem::path fsp(path);
		if (fsp.is_relative()) {
			auto rp = rootPath;
			rp += fsp;
			fsp.swap(rp);
		}
		UINT codePage = CP_UTF8;
		return utils->LoadFile(fsp.c_str(), &codePage, reinterpret_cast<IDxcBlobEncoding**>(pptr));
	}
	HRESULT QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) { return E_FAIL; }
	ULONG STDMETHODCALLTYPE AddRef(void) { return 0; }
	ULONG STDMETHODCALLTYPE Release(void) { return 0; }
	IDxcUtils* utils;
	std::filesystem::path rootPath;
};

bool DXCompilerHelper::CompileToByteCode(const wchar_t* srcFilePath, const char* profile,
	SmartPtr<ID3DBlob>& outputBuffer, 
	bool outputDebugInfo) {
	if (m_compiler.Get() == nullptr) {
		m_err = "compiler is invalid!";
		return false;
	}
	else if (m_utils.Get() == nullptr) {
		m_err = "library is invalid!";
		return false;
	}


	bool bUseDXIL = profile[3] - '0' >= 6;
	if (bUseDXIL == false) {
		SmartPtr<ID3DBlob> errorMsg;
		/** 使用FXC */
		UINT Flag = 0;
#ifdef _DEBUG
		Flag = D3DCOMPILE_DEBUG;
#else
		Flag = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif // _DEBUG
		if (FAILED(D3DCompileFromFile(srcFilePath, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"main", profile, Flag, 0, outputBuffer.GetAddressOf(), errorMsg.GetAddressOf()))) {
			m_err = reinterpret_cast<char*>(errorMsg->GetBufferPointer());
			return false;
		}
		return true;
	}
	/**使用DXIL */
	std::wstring&& pf = fromUTF8ToWideChar(profile);
	SmartPtr<IDxcBlobEncoding> shaderSrc;
	uint32_t pageCode = CP_UTF8;
	if (FAILED(m_utils->LoadFile(srcFilePath, &pageCode, shaderSrc.ReleaseAndGetAddressOf()))) {
		m_err = "load shader source file failed!";
		return false;
	}
	SmartPtr<IDxcResult> compileOpResult;
	SmartPtr<IDxcBlob> debugInfo;
	std::filesystem::path sourceFilePath(srcFilePath);
	auto fileName = sourceFilePath.filename();
#ifdef _DEBUG
	auto pdbFileName = sourceFilePath.remove_filename();
	pdbFileName += "Debug/";
	pdbFileName += fileName;
	pdbFileName.replace_extension(".pdb");
#endif // _DEBUG
	const wchar_t* args[] = {
		fileName.c_str(),
		L"-E", L"main",
		L"-T", pf.c_str(),
#ifdef _DEBUG
		L"-D", L"_DEBUG",
		L"-Od",
		L"-Zi",
		L"-Qembed_debug", /**< 嵌入更多调式指令 */
		//L"-Fd", pdbFileName.c_str()
#else // _DEBUG
		L"-O3"
#endif // _DEBUG
	};
	MIncludeHandler mh(m_utils.Get(), sourceFilePath.remove_filename());
	DxcBuffer sourceBuffer;
	sourceBuffer.Ptr = shaderSrc->GetBufferPointer();
	sourceBuffer.Size = shaderSrc->GetBufferSize();
	sourceBuffer.Encoding = CP_UTF8;
	
	if (DXCComplie(&sourceBuffer, args, ArraySize(args), &mh, compileOpResult) == false) return false;
	SmartPtr<IDxcBlob> byteCodes;
	compileOpResult->GetResult(byteCodes.ReleaseAndGetAddressOf());
	
	byteCodes.As(&outputBuffer);
	return true;
}

bool DXCompilerHelper::CompileToByteCode(const char* srcCode, size_t srcSize, const char* profile,
	Microsoft::WRL::ComPtr<ID3DBlob>& outputBuffer, bool outputDebugInfo, const char* shaderName)
{
	if (m_compiler.Get() == nullptr) {
		m_err = "compiler is invalid!";
		return false;
	}
	else if (m_utils.Get() == nullptr) {
		m_err = "library is invalid!";
		return false;
	}


	bool bUseDXIL = profile[3] - '0' >= 6;
	if (bUseDXIL == false) {
		SmartPtr<ID3DBlob> errorMsg;
		/** 使用FXC */
		if (FAILED(D3DCompile(srcCode, srcSize, shaderName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"main", profile, D3DCOMPILE_DEBUG, 0, outputBuffer.GetAddressOf(), errorMsg.GetAddressOf()))) {
			m_err = reinterpret_cast<char*>(errorMsg->GetBufferPointer());
			return false;
		}
		return true;
	}
	/**使用DXIL */
	std::wstring&& pf = fromUTF8ToWideChar(profile);
	SmartPtr<IDxcResult> compileOpResult;
	std::wstring&& sn = fromUTF8ToWideChar(shaderName);
	const wchar_t* args[] = {
		sn.c_str(),
		L"-E", L"main",
		L"-T", pf.c_str(),
		L"-O3"
	};


	DxcBuffer sourceBuffer;
	sourceBuffer.Encoding = CP_UTF8;
	sourceBuffer.Ptr = srcCode;
	sourceBuffer.Size = srcSize;
	
	if (DXCComplie(&sourceBuffer, args, ArraySize(args), nullptr, compileOpResult) == false) return false;

	SmartPtr<IDxcBlob> byteCodes;
	compileOpResult->GetResult(byteCodes.ReleaseAndGetAddressOf());

	byteCodes.As(&outputBuffer);
	return true;
}

bool DXCompilerHelper::DXCComplie(const DxcBuffer* source, LPCWSTR* args, UINT32 argCount, IDxcIncludeHandler* pIncludeHandler, SmartPtr<IDxcResult>& compileRes)
{
	if (FAILED(m_compiler->Compile(source, args, argCount, pIncludeHandler, IID_PPV_ARGS(&compileRes)))) {
		SmartPtr<IDxcBlobEncoding> errBlob;
		compileRes->GetErrorBuffer(errBlob.ReleaseAndGetAddressOf());
		SmartPtr<IDxcBlobUtf8> utf8Blob;
		m_utils->GetBlobAsUtf8(errBlob.Get(), utf8Blob.GetAddressOf());
		m_err = reinterpret_cast<char*>(utf8Blob->GetBufferPointer());
		return false;
	}
	/** 检查编译结果 */
	HRESULT compileStatus;
	compileRes->GetStatus(&compileStatus);
	if (FAILED(compileStatus)) {
		SmartPtr<IDxcBlobEncoding> errBlob;
		compileRes->GetErrorBuffer(errBlob.ReleaseAndGetAddressOf());
		SmartPtr<IDxcBlobUtf8> utf8Blob;
		m_utils->GetBlobAsUtf8(errBlob.Get(), utf8Blob.GetAddressOf());
		m_err = reinterpret_cast<char*>(utf8Blob->GetBufferPointer());
		return false;
	}
	return true;
}

auto DXCompilerHelper::RetrieveShaderDescriptionFromByteCode(SmartPtr<ID3DBlob>& byteCodes)
	-> Microsoft::WRL::ComPtr<ID3D12ShaderReflection> {
	SmartPtr<ID3D12ShaderReflection> shrReflect = nullptr;
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
	bool bUsedDXIL = true;
	if (FAILED(reflection->FindFirstPartKind(dxil, &shaderIdx))) {
		bUsedDXIL = false;
	}
	if (bUsedDXIL == false) {
		/** 考虑是使用FXC编译的 */
		if (FAILED(D3DReflect(byteCodes->GetBufferPointer(), byteCodes->GetBufferSize(), IID_PPV_ARGS(&shrReflect)))) {
			m_err = "Get shader reflection with FXC failed";
			return shrReflect;
		}
		return shrReflect;
	}
	if (FAILED(reflection->GetPartReflection(shaderIdx, IID_PPV_ARGS(&shrReflect)))) {
		m_err = "Get shader reflection with DXIL failed";
		return shrReflect;
	}

	return shrReflect;
}

/** 从windows example拷贝的方法 */
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
