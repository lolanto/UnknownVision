#pragma once
#include <map>
#include <d3d11.h>
#include <wrl.h>
#include <memory>

enum ShaderType {
	ST_VS = 0,
	ST_PS
};

typedef std::map<unsigned int, Microsoft::WRL::ComPtr<ID3D11VertexShader>> VertexShaderList;
typedef std::map<unsigned int, Microsoft::WRL::ComPtr<ID3D11PixelShader>> PixelShaderList;
typedef unsigned int Shader;

class ShaderManager {
public:
	ShaderManager(ID3D11Device** dev, ID3D11DeviceContext** devContext);

public:
	bool CreateShader(const char* filePath, ShaderType, Shader&);
	void Setup(Shader, ShaderType);
private:
	bool createVertexShader(std::shared_ptr<byte>&, size_t&, Shader&);
	bool createPixelShader(std::shared_ptr<byte>&, size_t&, Shader&);
	bool readFile(const char* path, std::shared_ptr<byte>&, size_t&);
	void calcNextVSToken();
	void calcNextPSToken();
private:
	ID3D11Device**													m_dev;
	ID3D11DeviceContext**										m_devContext;

	VertexShaderList													m_vs;
	unsigned int															m_vsToken;
	PixelShaderList														m_ps;
	unsigned int															m_psToken;
};