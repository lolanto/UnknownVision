#ifndef FILE_CONTAINER_H
#define FILE_CONTAINER_H

#include <fstream>
#include <string>
/* 作为本地磁盘文件的容器，通过指定文件的磁盘路径进行创建。
	负责读取文件内容，分析文件属性，自动关闭文件等。
**/
class FileContainer {
public:
	FileContainer(const char* filePath, std::ios_base::openmode mode);
	~FileContainer() {
		if (m_file.is_open()) m_file.close();
	}
	// 返回打开文件的字节大小，文件未打开则返回0
	size_t FileSize() const {
		if (m_file.is_open()) return m_fileSize;
		return 0;
	}
	/* 从文件开头|byteOffset|个字节处开始，读取|bytes|个字节到|outputData|指向的缓冲中
		@ret
			任何错误造成的读取失败均返回false，读取成功返回true
	**/
	bool ReadFile(uint32_t byteOffset, uint32_t bytes, char* outputData);
	bool IsOpen() const { return m_file.is_open(); }
	// 手动关闭文件
	void Close() { if (m_file.is_open()) m_file.close(); }
private:
	std::ifstream m_file;
	size_t m_fileSize;
	std::string m_fileName;
	std::string m_filePath;
};

#endif // FILE_CONTAINER_H
