#pragma once
#include "RenderSys_UVConfig.h"
#include <string>

// �����Դ�ĳ������
/* ���ۺ���Resource����
����������Դ��Ӧ��ͼ�ο���������
��ͼ�ο���ص����ݴ洢�ڹ�������
��Դ�����������Դ�������Թ���ѯ
*/
class Resource {
public:
	Resource(std::string name, UINT id):Name(name), RID(id) {}
	const std::string Name;
	const UINT RID;
};

class Canvas : public Resource {
public:
	Canvas(std::string name, UINT id,
		UINT usage, float width, float height,
		std::string dataPath) : Resource(name, id),
		m_usage(usage), m_width(width), m_height(height), m_dataPath(dataPath) {}

public:
	bool CanWriteTo() const { return m_usage & Canvas_Usage_Write; }
	float GetWidth() const { return m_width; }
	float GetHeight() const { return m_height; }
	const char* GetDataPath() const { return m_dataPath.c_str(); }
private:
	UINT			m_usage;
	float			m_width, m_height;
	std::string	m_dataPath;
};

class Shader : public Resource {
public:
	Shader(std::string name, UINT id,
		Shader_Type type, std::string dataPath) : Resource(name, id),
		m_type(type), m_dataPath(dataPath) {}

public:
	Shader_Type GetType() const { return m_type; }
	const char* GetDataPath() const { return m_dataPath.c_str(); }
private:
	Shader_Type m_type;
	std::string		m_dataPath;
};