#include "FileContainer.h"
#include "../InfoLog/InfoLog.h"

FileContainer::FileContainer(const char* filePath, std::ios_base::openmode mode) {
	m_file.open(filePath, mode);
	if (m_file.is_open()) {
		m_filePath = filePath;
		m_file.seekg(0, std::ios::end);
		m_fileSize = static_cast<size_t>(m_file.tellg());
		m_file.seekg(0, std::ios::beg);
		return;
	}
	MLOG(LW, __FUNCTION__, LL, " open file failed!");
}

bool FileContainer::ReadFile(uint32_t byteOffset, uint32_t bytes, char* outputData) {
	if (!m_file.is_open()) {
		MLOG(LE, __FUNCTION__, LL, " file was not opened");
		return false;
	}
	if (byteOffset + bytes > m_fileSize) {
		MLOG(LE, __FUNCTION__, LL, " reading out of range");
		return false;
	}
	if (!outputData) {
		MLOG(LE, __FUNCTION__, LL, " invalid output buffer pointer");
		return false;
	}
	m_file.seekg(0, std::ios::beg);
	m_file.seekg(byteOffset);
	m_file.read(outputData, bytes);
	if (m_file.fail()) {
		MLOG(LE, __FUNCTION__, LL, "something wrong happened!");
		return false;
	}
	return true;
}

bool FileContainer::WriteFile(uint32_t byteOffset, uint32_t bytes, const char * inputData)
{
	if (!m_file.is_open()) {
		MLOG(LE, __FUNCTION__, LL, "file was not opened!");
		return false;
	}
	m_file.seekp(0, std::ios::beg);
	m_file.seekp(byteOffset);
	m_file.write(inputData, bytes);
	if (m_file.fail()) {
		MLOG(LE, __FUNCTION__, LL, "something wrong happend!");
		return false;
	}
	return false;
}
