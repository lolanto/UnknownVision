#pragma once

/** 该文件提供对DXIL编译器的封装支持 */

#include <Windows.h> /**< 提供com操作支持的 */
#include <DXC/dxcapi.h> /**< 提供DXIL API支持的 */
#include <wrl.h> /**< 提供COM智能指针支持的 */
#include <DXC/d3d12shader.h>
#include <string>

#define SHADER_VERSION(s) #s
#define SHADER_MODEL(type) SHADER_VERSION(type##_6_0)

class DXCompilerHelper {
public:
	/** 构造函数，用于初始化编译辅助结构的必要组建
	 * @param err 若该指针不为空，则在初始化失败时记录错误信息 */
	DXCompilerHelper();
	~DXCompilerHelper() = default;
public:
	/** 检验shader源码文件的存在性和时间戳 */
	auto TimeStampOfShaderSourceCode(const char* shaderName)
		->std::pair<uint64_t, bool>;
	/** 加载指定的shader(源文件或字节码)
	 * @param shaderName 需要加载的shader的(路径/)名称
	 * @param profile 编译时必须设置的属性，决定了shader model以及shader的类型
	 * @return 返回存储字节码的缓冲区，假如编译失败，返回空的blob
	 * @remark 调用者必须保证源码文件(.hlsl)的存在
	 * 该函数保证相同shader加载过程可安全并行进行*/
	Microsoft::WRL::ComPtr<ID3DBlob> LoadShader(const char* shaderName, const char* profile);
	/** 从Shader源文件编译产生字节码供Shader生成使用
	 * @param srcFilePath Shader源文件路径
	 * @param profile 编译时必须设置的属性，决定了shader model 以及shader的类型
	 * @param outputBuffer 编译成功时会存储字节码，否则不做任何修改
	 * @param outputDebugInfo 是否输出debug需要使用的信息
	 * @return 编译成功返回true，编译失败返回false */
	bool CompileToByteCode(const wchar_t* srcFilePath, const char* profile,
		Microsoft::WRL::ComPtr<ID3DBlob>& outputBuffer, bool outputDebugInfo = false);
	/** 从源码字符串编译产生字节码供Shader生成使用
	 * @param srcCode Shader源码字符串
	 * @param srcSize 源码字符串缓冲大小
	 * @param profile 编译时必须设置的属性，决定了shader model 以及shader的类型
	 * @param outputBuffer 编译成功时会存储字节码，否则不做任何修改
	 * @param outputDebugInfo 是否输出debug需要使用的信息
	 * @param shaderName 可选的，用于标记当前编译Shader的名称，或许Debug会用到
	 * @return 编译成功返回true，编译失败返回false */
	bool CompileToByteCode(const char* srcCode, size_t srcSize, const char* profile,
		Microsoft::WRL::ComPtr<ID3DBlob>& outputBuffer, bool outputDebugInfo = false, const char* shaderName = "");
	/** 从ByteCode中提取该shader的描述信息
	 * @param byteCodes 存储DXC编译后，shader的字节码
	 * @param outputDescription 存储shader描述信息的结构体的引用
	 * @return 返回用于查询反射信息的COM对象指针，若失败，返回NULL */
	auto RetrieveShaderDescriptionFromByteCode(Microsoft::WRL::ComPtr<ID3DBlob>& byteCodes)
		->Microsoft::WRL::ComPtr<ID3D12ShaderReflection>;
	const char* LastErrorMsg() const { return m_err.data(); }
private:
	bool DXCComplie(const DxcBuffer*, LPCWSTR* args, UINT32 arrgCount, IDxcIncludeHandler* pIncludeHandler, Microsoft::WRL::ComPtr<IDxcResult>& compileRes);
private:
	Microsoft::WRL::ComPtr<IDxcUtils> m_utils; /**< 负责加载文档的对象，同时提供缓冲区创建方法 */
	Microsoft::WRL::ComPtr<IDxcCompiler3> m_compiler; /**< 负责提供编译方法的对象 */
	std::string m_err;
};

