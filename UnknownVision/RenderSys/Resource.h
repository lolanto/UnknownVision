#pragma once
#include "RenderSys_UVConfig.h"
#include <string>
#include <vector>
#include <memory>

// 针对资源的抽象基类
/* 无论何种Resource对象
均不包含资源对应的图形库的相关内容
与图形库相关的内容存储在管理器中
资源对象仅包含资源的属性以供查询
*/
class Resource {
public:
	Resource(std::string name, UINT id):Name(name), RID(id) {}
	const std::string Name;
	const UINT RID;
};

class Shader : public Resource {
public:
	Shader(std::string name, UINT id,
		Shader_Type type, std::string dataPath) : Resource(name, id),
		m_type(type), m_dataPath(dataPath) {}

public:
	Shader_Type Type() const { return m_type; }
	const char* DataPath() const { return m_dataPath.c_str(); }
private:
	Shader_Type m_type;
	std::string		m_dataPath;
};

class Buffer : public Resource {
public:
	Buffer(std::string name, UINT id,
		Buffer_Type type, std::unique_ptr<std::vector<uint8_t> >& data)
		: Resource(name, id) {}
public:
	Buffer_Type Type() const { return m_type; }
	const std::vector<uint8_t>& Data() const { *m_data; }
private:
	Buffer_Type m_type;
	std::unique_ptr<std::vector<uint8_t> > m_data;
};

class Texture : 
