#include "FileContainer.h"
#include "../InfoLog/InfoLog.h"

FileContainer::FileContainer(const char* filePath, std::ios_base::openmode mode)
	: m_fileSize(0), m_filePath("") {
	m_file.open(filePath, mode);
	if (m_file.is_open()) {
		m_filePath = filePath;
		m_file.seekg(0, std::ios::end);
		m_fileSize = static_cast<size_t>(m_file.tellg());
		m_file.seekg(0, std::ios::beg);
		return;
	}
	FLOG("Function: %s FAILED", __FUNCTION__);
}

bool FileContainer::ReadFile(uint32_t byteOffset, uint32_t bytes, char* outputData) {
	if (!m_file.is_open()) {
		FLOG("%s: open file %s FAILED!", __FUNCTION__, outputData);
		return false;
	}
	if (byteOffset + bytes > m_fileSize) {
		FLOG("%s: reading out of range!", __FUNCTION__);
		return false;
	}
	if (!outputData) {
		FLOG("%s: Invalid output buffer pointer!", __FUNCTION__);
		return false;
	}
	m_file.seekg(0, std::ios::beg);
	m_file.seekg(byteOffset);
	m_file.read(outputData, bytes);
	if (m_file.fail()) {
		FLOG("%s: Something wrong happend!", __FUNCTION__);
		return false;
	}
	return true;
}

bool FileContainer::WriteFile(uint32_t byteOffset, uint32_t bytes, const char * inputData)
{
	if (!m_file.is_open()) {
		FLOG("%s: Open file %s FAILED!", __FUNCTION__, inputData);
		return false;
	}
	m_file.seekp(0, std::ios::beg);
	m_file.seekp(byteOffset);
	m_file.write(inputData, bytes);
	if (m_file.fail()) {
		FLOG("%s: Something wrong happend!", __FUNCTION__);
		return false;
	}
	return false;
}
