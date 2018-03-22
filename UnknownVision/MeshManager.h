#pragma once
#include <map>
#include<wrl.h>
#include <d3d11.h>

typedef unsigned int UINT;

class Mesh;

struct MeshBuffers {
	Microsoft::WRL::ComPtr<ID3D11Buffer>							position;
	Microsoft::WRL::ComPtr<ID3D11Buffer>							normal;
	Microsoft::WRL::ComPtr<ID3D11Buffer>							tangent;
	Microsoft::WRL::ComPtr<ID3D11Buffer>							texcoord;

	Microsoft::WRL::ComPtr<ID3D11Buffer>							index;
	UINT																				numIndex;
};

typedef std::map<UINT, MeshBuffers> MeshList;

class MeshManager {
public:
	MeshManager(ID3D11Device**, ID3D11DeviceContext**);
public:
	bool Bind(Mesh*);
	void Setup(UINT t);

private:
	bool createBuffer(const void* data, size_t size, Microsoft::WRL::ComPtr<ID3D11Buffer>&, D3D11_BIND_FLAG flag = D3D11_BIND_VERTEX_BUFFER);
	UINT nextToken();
private:
	ID3D11Device**																m_dev;
	ID3D11DeviceContext**													m_devContext;
	MeshList																			m_list;

	Microsoft::WRL::ComPtr<ID3D11InputLayout>				m_inputLayout;

	UINT																				m_curToken;
};
