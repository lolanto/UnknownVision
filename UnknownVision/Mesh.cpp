#include <assimp\Importer.hpp>
#include <assimp\scene.h>
#include <assimp\postprocess.h>

#include "Mesh.h"
#include "Model.h"
#include "InfoLog.h"

using std::vector;
using std::string;
using DirectX::XMFLOAT3;
using DirectX::XMFLOAT2;

Mesh::Mesh(vector<XMFLOAT3>& pos,
	vector<XMFLOAT3>& nor,
	vector<XMFLOAT3>& tan,
	vector<XMFLOAT2>& td,
	vector<UINT>& ind, string name) : m_position(pos), m_normal(nor), 
	m_tangent(tan), m_texcoord(td), 
	m_index(ind), m_name(name), m_hasSetup(false) {}


bool Mesh::Setup(ID3D11Device* dev) {
	if (m_hasSetup) return true;
	static const char* funcTag = "Mesh::Setup: ";
	// create buffers
	D3D11_BUFFER_DESC desc;
	D3D11_SUBRESOURCE_DATA subData;
	size_t size = -1;
	ZeroMemory(&desc, sizeof(desc));
	ZeroMemory(&subData, sizeof(subData));

	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;

	subData.SysMemPitch = 0;
	subData.SysMemSlicePitch = 0;

	auto fp = [&](size_t size, void* data,
		Microsoft::WRL::ComPtr<ID3D11Buffer>& buf,
		D3D11_BIND_FLAG flag = D3D11_BIND_VERTEX_BUFFER)->bool {
		desc.ByteWidth = size;
		desc.BindFlags = flag;
		subData.pSysMem = data;
		if (FAILED(dev->CreateBuffer(&desc, &subData, buf.ReleaseAndGetAddressOf()))) return false;
		return true;
	};

	// create position buffer
	if (!fp(m_position.size() * sizeof(XMFLOAT3),
		&m_position[0], m_bufPosition)) {
		MLOG(LL, funcTag, LW, "create position buffer failed!");
		return false;
	}
	
	// create normal buffer
	if (!fp(m_normal.size() * sizeof(XMFLOAT3),
		&m_normal[0], m_bufNormal)) {
		MLOG(LL, funcTag, LW, "create normal buffer failed!");
		return false;
	}

	// create tangent buffer
	if (!fp(m_tangent.size() * sizeof(XMFLOAT3),
		&m_tangent[0], m_bufTangent)) {
		MLOG(LL, funcTag, LW, "create tangent buffer failed!");
		return false;
	}

	// create texcoord buffer
	if (!fp(m_texcoord.size() * sizeof(XMFLOAT2),
		&m_texcoord[0], m_bufTexcoord)) {
		MLOG(LL, funcTag, LW, "create texcoord buffer failed!");
		return false;
	}

	// create index buffer
	if (!fp(m_index.size() * sizeof(UINT),
		&m_index[0], m_bufIndex, D3D11_BIND_INDEX_BUFFER)) {
		MLOG(LL, funcTag, LW, "create index buffer failed!");
		return false;
	}
	m_hasSetup = true;
	return true;
}

void Mesh::Bind(ID3D11DeviceContext* devCtx, ShaderBindTarget sbt, SIZE_T slot) {
	static UINT stride = -1;
	static UINT offset = 0;
	// Model
	// position
	stride = sizeof(XMFLOAT3);
	devCtx->IASetVertexBuffers(0, 1, m_bufPosition.GetAddressOf(), &stride, &offset);
	// normal
	devCtx->IASetVertexBuffers(1, 1, m_bufNormal.GetAddressOf(), &stride, &offset);
	// tangent
	devCtx->IASetVertexBuffers(2, 1, m_bufTangent.GetAddressOf(), &stride, &offset);
	// texcoord
	stride = sizeof(XMFLOAT2);
	devCtx->IASetVertexBuffers(3, 1, m_bufTexcoord.GetAddressOf(), &stride, &offset);
	// index
	stride = sizeof(UINT);
	devCtx->IASetIndexBuffer(m_bufIndex.Get(), DXGI_FORMAT_R32_UINT, 0);

	// Draw call
	devCtx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	devCtx->DrawIndexed(m_index.size(), 0, 0);
}

void Mesh::Unbind(ID3D11DeviceContext* devCtx, ShaderBindTarget sbt, SIZE_T slot) {
	return;
}

// 获取Mesh中的位置信息
// 输出: 获取成功返回true,否则返回false
// 输入: 
// data: 获取成功时返回指向顶点位置信息的数组的指针，否则返回空指针
// size: 获取成功返回顶点位置信息数组的长度，获取失败时返回-1

bool Mesh::GetPosition(const void*& data, size_t& size)
{
	// 有位置信息才能够获取
	if (m_position.size()) {
		data = &m_position[0];
		size = m_position.size() * sizeof(XMFLOAT3);
		return true;
	}
	else {
		data = NULL;
		size = -1;
		MLOG(LL, "Mesh::GetPosition: There're no position data");
		return false;
	}
	return false;
}

// 获取Mesh中的法线信息
// 输出: 获取成功返回true, 否则返回false
// 输入: 
// data: 获取成功返回指向法线信息数组的指针，否则返回空指针
// size: 获取成功时返回法线信息数组的长度，否则返回-1

bool Mesh::GetNormal(const void*& data, size_t& size) {
	// 有法线信息才能够获取
	if (m_normal.size()) {
		data = &m_normal[0];
		size = m_normal.size() * sizeof(XMFLOAT3);
		return true;
	}
	else {
		data = NULL;
		size = -1;
		MLOG(LL, "Mesh::GetNormal: There're no normal data");
		return false;
	}
	return false;
}

// 获取Mesh中的切线信息
// 输出: 获取成功返回true, 获取失败返回false
// 输入:
// data: 获取成功时返回指向切线信息数组的指针，否则返回空指针
// size: 获取成功返回切线信息数组的长度，否则返回-1

bool Mesh::GetTangent(const void*& data, size_t& size) {
	if (m_tangent.size()) {
		data = &m_tangent[0];
		size = m_tangent.size() * sizeof(XMFLOAT3);
		return true;
	}
	else {
		data = NULL;
		size = -1;
		MLOG(LL, "Mesh::GetTangent: There're no tangent data");
		return false;
	}
	return false;
}

// 获取Mesh中的顶点UV信息

bool Mesh::GetTexcoord(const void*& data, size_t& size) {
	if (m_texcoord.size()) {
		data = &m_texcoord[0];
		size = m_texcoord.size() * sizeof(XMFLOAT2);
		return true;
	}
	else {
		data = NULL;
		size = -1;
		MLOG(LL, "Mesh::GetTexcoord: There're no texture coordinate data");
		return false;
	}
	return false;
}

bool Mesh::GetIndex(const void*& data, size_t& size) {
	if (m_index.size()) {
		data = &m_index[0];
		size = m_index.size() * sizeof(UINT);
		return true;
	}
	else {
		data = NULL;
		size = -1;
		MLOG(LL, "Mesh::GetIndex: There're no index data");
		return false;
	}
	return false;
}

string& Mesh::GetName() { return m_name; }

////////////////////////////////////////////////////////////////////////////////////////
// Mesh Factory!
////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////
// Public Fuction
////////////////////////////

std::vector<std::shared_ptr<Mesh>>& MeshFactory::Load(const char* path) {
	static const char* funcTag = "ModelFactory::Load: ";
	m_tempMesh.clear();

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path,
		aiProcess_CalcTangentSpace |
		aiProcess_Triangulate |
		/*使用Direct时候必须将模型文件中的坐标转换成左手坐标系*/
		aiProcess_ConvertToLeftHanded);
	if (!scene) {
		// 文件读取失败
		MLOG(LE, funcTag, LL, "create importer failed!");
		return m_tempMesh;
	}

	processNode(scene->mRootNode, scene);
	if (!m_tempMesh.size()) {
		// 没有任何的网格被载入
		MLOG(LW, funcTag, LL, "there is no mesh has been loaded!");
		return m_tempMesh;
	}

	return m_tempMesh;
}

void MeshFactory::Load(BasicMeshType type, std::shared_ptr<Mesh>& re, float a, float b, float c) {
	m_tempMesh.clear();
	Mesh* tmpMesh = NULL;
	switch (type) {
	case BMT_PLANE:
		tmpMesh = createPlane(a, b);
		if (!tmpMesh) {
			MLOG(LL, "MeshFactory::Load: create plane failed!");
			break;
		}
		break;
	default:
		MLOG(LL, "MeshFactory::Load: Invalid basic mesh type!");
		break;
	}
	re = std::shared_ptr<Mesh>(tmpMesh);
}

//////////////////////////
// Private Function
//////////////////////////

Mesh* MeshFactory::createPlane(float width, float height) {
	// create position/normal/tangent/texcoord/index buffer data
	// position
	std::vector<DirectX::XMFLOAT3> position = {
		{ width / -2.0f, height / 2.0f, 0.0f },
		{ width / 2.0f, height / 2.0f, 0.0f },
		{ width / 2.0f, height / -2.0f, 0.0f },
		{ width / -2.0f, height / -2.0f, 0.0f }
	};
	// normal
	std::vector<DirectX::XMFLOAT3> normal = {
		{ 0.0f, 0.0f, -1.0f },
		{ 0.0f, 0.0f, -1.0f },
		{ 0.0f, 0.0f, -1.0f },
		{ 0.0f, 0.0f, -1.0f }
	};
	// tangent
	std::vector<DirectX::XMFLOAT3> tangent = {
		{ 1.0f, 0.0f, 0.0f },
		{ 1.0f, 0.0f, 0.0f },
		{ 1.0f, 0.0f, 0.0f },
		{ 1.0f, 0.0f, 0.0f }
	};
	// texcoord
	std::vector<DirectX::XMFLOAT2> texcoord = {
		{ 0.0f, 0.0f },
		{ 1.0f, 0.0f },
		{ 1.0f, 1.0f },
		{ 0.0f, 1.0f }
	};
	// index
	std::vector<UINT> index = {
		0, 1, 2,
		0, 2, 3
	};
	return new Mesh(position, normal, tangent, texcoord, index);
}

void MeshFactory::processNode(aiNode* node, const aiScene* scene) {
	for (UINT i = 0; i < node->mNumMeshes; ++i) {
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		Mesh* t = processMesh(mesh, scene);
		if (!t) continue;
		m_tempMesh.push_back(std::shared_ptr<Mesh>(t));
	}
	for (UINT i = 0; i < node->mNumChildren; ++i) {
		processNode(node->mChildren[i], scene);
	}
}

Mesh* MeshFactory::processMesh(aiMesh* mesh, const aiScene* scene) {

	static const char* funcTag = "ModelFactory::processMesh: ";
	if (!mesh->HasNormals()) {
		MLOG(LL, funcTag, LW, "this mesh has no normal data! create failed");
		return NULL;
	}
	if (!mesh->HasTangentsAndBitangents()) {
		MLOG(LL, funcTag, LW, "this mesh has no tangent data! create failed");
		return NULL;
	}
	if (!mesh->HasTextureCoords(0)) {
		MLOG(LL, funcTag, LW, "this mesh has no texture coordinate data! create failed!");
		return NULL;
	}

	vector<XMFLOAT3> position;
	vector<XMFLOAT3> normal;
	vector<XMFLOAT3> tangent;
	vector<XMFLOAT2> texcoord;
	vector<UINT>			 index;

	for (UINT i = 0; i < mesh->mNumVertices; ++i) {
		position.push_back(XMFLOAT3(
			mesh->mVertices[i].x,
			mesh->mVertices[i].y,
			mesh->mVertices[i].z
		));

		normal.push_back(XMFLOAT3(
			mesh->mNormals[i].x,
			mesh->mNormals[i].y,
			mesh->mNormals[i].z
		));

		tangent.push_back(XMFLOAT3(
			mesh->mTangents[i].x,
			mesh->mTangents[i].y,
			mesh->mTangents[i].z
		));

		texcoord.push_back(XMFLOAT2(
			mesh->mTextureCoords[0][i].x,
			mesh->mTextureCoords[0][i].y
		));
	}

	for (UINT i = 0; i < mesh->mNumFaces; ++i) {
		for (UINT j = 0; j < mesh->mFaces[i].mNumIndices; ++j) {
			index.push_back(mesh->mFaces[i].mIndices[j]);
		}
	}

	return new Mesh(position, normal, tangent, texcoord, index);
}

