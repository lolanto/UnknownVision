#ifndef DXIL_COMPILER_HELPER_H
#define DXIL_COMPILER_HELPER_H

/** 该文件提供对DXIL编译器的封装支持 */

#include <Windows.h> /**< 提供com操作支持的 */
#include <dxcapi.h> /**< 提供DXIL API支持的 */
#include <wrl.h> /**< 提供COM智能指针支持的 */
#include <vector>

class DXILCompilerHelper {
public:
	/** 构造函数，用于初始化编译辅助结构的必要组建
	 * @param err 若该指针不为空，则在初始化失败时记录错误信息 */
	DXILCompilerHelper(std::vector<char>* err = nullptr);
	~DXILCompilerHelper() = default;
public:
	/** 从Shader源文件编译产生字节码供Shader生成使用
	 * @param srcFilePath Shader源文件路径
	 * @param profile 编译时必须设置的属性，决定了shader model 以及shader的类型
	 * @param outputBuffer 编译成功时会存储字节码，否则不做任何修改
	 * @param outputDebugInfo 是否输出debug需要使用的信息
	 * @param err 若该指针不为空，则会在编译失败时记录错误信息
	 * @return 编译成功返回true，编译失败返回false */
	bool CompileToByteCode(const char* srcFilePath, const char* profile,
		std::vector<uint8_t>& outputBuffer, bool outputDebugInfo = false, std::vector<char>* err = nullptr);
private:
	Microsoft::WRL::ComPtr<IDxcLibrary> m_library; /**< 负责加载文档的对象，同时提供缓冲区创建方法 */
	Microsoft::WRL::ComPtr<IDxcCompiler2> m_compiler; /**< 负责提供编译方法的对象 */
};

#endif // DXIL_COMPILER_HELPER_H
