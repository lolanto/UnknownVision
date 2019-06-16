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
	char outputInfo[32];
	sprintf(outputInfo, "Function: %s FAILED", __FUNCTION__);
	MLOG(outputInfo);
}

bool FileContainer::ReadFile(uint32_t byteOffset, uint32_t bytes, char* outputData) {
	char outputInfo[128];
	if (!m_file.is_open()) {
		sprintf(outputInfo, "%s: open file %s FAILED!", __FUNCTION__, outputData);
		MLOG(outputInfo);
		return false;
	}
	if (byteOffset + bytes > m_fileSize) {
		sprintf(outputInfo, "%s: reading out of range!");
		MLOG(outputInfo);
		return false;
	}
	if (!outputData) {
		sprintf(outputInfo, "%s: Invalid output buffer pointer!");
		MLOG(outputInfo);
		return false;
	}
	m_file.seekg(0, std::ios::beg);
	m_file.seekg(byteOffset);
	m_file.read(outputData, bytes);
	if (m_file.fail()) {
		sprintf(outputInfo, "%s: Something wrong happend!", __FUNCTION__);
		MLOG(outputInfo);
		return false;
	}
	return true;
}

bool FileContainer::WriteFile(uint32_t byteOffset, uint32_t bytes, const char * inputData)
{
	char outputInfo[128];
	if (!m_file.is_open()) {
		sprintf(outputInfo, "%s: Open file %s FAILED!", __FUNCTION__, inputData);
		MLOG(outputInfo);
		return false;
	}
	m_file.seekp(0, std::ios::beg);
	m_file.seekp(byteOffset);
	m_file.write(inputData, bytes);
	if (m_file.fail()) {
		sprintf(outputInfo, "%s: Something wrong happend!", __FUNCTION__);
		MLOG(outputInfo);
		return false;
	}
	return false;
}
