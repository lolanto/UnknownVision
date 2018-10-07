#pragma once
#include "RenderSys_UVConfig.h"
#include "Resource.h"

// ��Դ�������ĳ�����࣬��������������Դ����ӿ�
class ResourceManager {
public:
	virtual void GetResourceData(Resource*, NameParams*) = 0;
};

class CanvasManager : public ResourceManager {
public:
	virtual void GetResourceData(_In_  Resource* re, _Inout_ NameParams& param) = 0;
	virtual Canvas Create(std::string name, UINT usage, char* path = nullptr) = 0;
	virtual Canvas Create(std::string name, UINT usage, float width, float height) = 0;
private:
};

class ShaderManager : public ResourceManager {
public:
	virtual void GetResourceData(_In_ Resource* re, _Inout_ NameParams& param) = 0;
	virtual Shader Create(std::string name, Shader_Type type, char* path = nullptr) = 0;
};