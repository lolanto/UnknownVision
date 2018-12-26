#pragma once

#include <DirectXMath.h>
#include <vector>
#include <memory>
#include <string>
#include <wrl.h>
#include "UnknownObject.h"

typedef unsigned int UINT;

struct aiNode;
struct aiScene;
struct aiMesh;

class Model;

enum BasicMeshType {
	BMT_PLANE = 0
};

// 存储Mesh的顶点信息
// Position, Texture coordinate, Normal, Tangent
// 后续可能会有更多的顶点信息

class Mesh : public UnknownObject {
public:
	Mesh(std::vector<DirectX::XMFLOAT3>& pos,
		std::vector<DirectX::XMFLOAT3>& nor,
		std::vector<DirectX::XMFLOAT3>& tan,
		std::vector<DirectX::XMFLOAT2>& td,
		std::vector<UINT>& ind, std::string name = "");
public:
	bool GetPosition(const void*&, size_t&);
	bool GetNormal(const void*&, size_t&);
	bool GetTangent(const void*&, size_t&);
	bool GetTexcoord(const void*&, size_t&);
	bool GetIndex(const void*&, size_t&);

	std::string& GetName();

	// Setup vertex buffer and index buffer
	bool Setup(ID3D11Device*);
	// bind vertex/index buffer and draw call
	void Bind(ID3D11DeviceContext*, ShaderBindTarget, SIZE_T);
	void Unbind(ID3D11DeviceContext*, ShaderBindTarget, SIZE_T);

private:
	std::vector<DirectX::XMFLOAT3>						m_position;
	std::vector<DirectX::XMFLOAT3>						m_normal;
	std::vector<DirectX::XMFLOAT3>						m_tangent;
	std::vector<DirectX::XMFLOAT2>						m_texcoord;
	std::vector<UINT>												m_index;

	Microsoft::WRL::ComPtr<ID3D11Buffer>			m_bufPosition;
	Microsoft::WRL::ComPtr<ID3D11Buffer>			m_bufNormal;
	Microsoft::WRL::ComPtr<ID3D11Buffer>			m_bufTangent;
	Microsoft::WRL::ComPtr<ID3D11Buffer>			m_bufTexcoord;
	Microsoft::WRL::ComPtr<ID3D11Buffer>			m_bufIndex;
	std::string																m_name;
	bool																		m_hasSetup;
};

class MeshFactory {
public:
	std::vector<std::shared_ptr<Mesh>>& Load(const char* path);
	/*
	for plane: a: width, b: height
	for cubic: a: width, b: height, c: depth
	*/
	void Load(BasicMeshType type, std::shared_ptr<Mesh>&, float a = 1, float b = 1, float c = 1);
private:
	Mesh* createPlane(float width, float height);
	Mesh* createCubic(float width, float height, float depth);

	void processNode(aiNode*, const aiScene*);
	Mesh* processMesh(aiMesh*, const aiScene*);

private:
	std::vector<std::shared_ptr<Mesh>>					m_tempMesh;
};
