#ifndef FILE_CONTAINER_H
#define FILE_CONTAINER_H

#include <fstream>
#include <string>
/** 作为本地磁盘文件的容器，通过指定文件的磁盘路径进行创建。
 *	负责读取/写入文件内容，分析文件属性，自动关闭文件等。*/
class FileContainer {
public:
	/** 创建文件容器
	 * @param filePath 文件的目标路径，可能是已创建的文件也可能
	 * 是即将创建的文件，取决于mode的值
	 * @param mode 文件打开方式，具体参考std::ios_base::openmode的定义*/
	FileContainer(const char* filePath, std::ios_base::openmode mode);
	~FileContainer() {
		if (m_file.is_open()) m_file.close();
	}
	/** 返回打开文件的字节大小，文件未打开则返回0 */
	size_t FileSize() const {
		if (m_file.is_open()) return m_fileSize;
		return 0;
	}
	/** 从文件中读取字节并放入指定缓冲
	 * @param byteOffset 读取位置与文件开头位置的偏移
	 * @param bytes 需要读取的字节数量
	 * @param outputData 指向存储被读出字节的缓冲
	 *	@return 任何错误造成的读取失败均返回false，读取成功返回true
	 * @remark 由调用者保证byteOffset + bytes一定小于文件大小，同时
	 * outputData有足够大的空间容纳bytes个字节数据 */
	bool ReadFile(uint32_t byteOffset, uint32_t bytes, char* outputData);
	/** 向文件中写入数据
	 * @param byteOffset 写入位置与文件开头位置的偏移
	 * @param bytes 需要写入的字节数量
	 * @param inputData 指向存储需要写入的数据的缓存
	 * @return 写入成功返回true，失败返回false
	 * @remark 由调用者保证byteOffset一定处于有效范围内，同时
	 * inputData一定拥有bytes以上的字节以供写入 */
	bool WriteFile(uint32_t byteOffset, uint32_t bytes, const char* inputData);
	bool IsOpen() const { return m_file.is_open(); }
	/** 手动关闭文件 */
	void Close() { if (m_file.is_open()) m_file.close(); }
private:
	std::fstream m_file;
	size_t m_fileSize;
	std::string m_filePath; /**< 文件路径为空表示文件处在错误状态 */
};

#endif // FILE_CONTAINER_H
