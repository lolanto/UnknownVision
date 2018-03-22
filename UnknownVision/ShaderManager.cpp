#include "ShaderManager.h"
#include "InfoLog.h"
#include <fstream>
#include <memory>

ShaderManager::ShaderManager(ID3D11Device** dev, ID3D11DeviceContext** devContext)
	: m_dev(dev), m_devContext(devContext), m_vsToken(0), m_psToken(0) {}

bool ShaderManager::CreateShader(const char* filePath, ShaderType type, Shader& output) {
	static const char* funcTag = "ShaderManager::CreateShader: ";
	std::shared_ptr<byte> byteCode;
	size_t size = -1;
	if (!readFile(filePath, byteCode, size)) {
		MLOG(LL, funcTag, LL, "can not get shader byte code!");
		return false;
	}
	switch (type) {
	case ST_VS:
		if (!createVertexShader(byteCode, size, output)) {
			MLOG(LL, funcTag, LL, "create vertex shader failed!");
			return false;
		}
		break;
	case ST_PS:
		if (!createPixelShader(byteCode, size, output)) {
			MLOG(LL, funcTag, LL, "create pixel shader failed!");
			return false;
		}
		break;
	default:
		MLOG(LL, funcTag, LL, "invalid shader type!");
		return false;
	}

	return true;
}

void ShaderManager::Setup(Shader s, ShaderType type) {
	static const char* funcTag = "ShaderManager::Setup: ";
	if (type == ST_VS) {
		auto iter = m_vs.find(s);
		if (iter == m_vs.end()) {
			MLOG(LL, funcTag, LL, "can not find responding vertex shader!");
			return;
		}
		(*m_devContext)->VSSetShader(iter->second.Get(), 0, 0);
	}
	else if (type == ST_PS) {
		auto iter = m_ps.find(s);
		if (iter == m_ps.end()) {
			MLOG(LL, funcTag, LL, "can not find responding pixel shader!");
			return;
		}
		(*m_devContext)->PSSetShader(iter->second.Get(), 0, 0);
	}
	else {
		MLOG(LL, funcTag, LW, "Invalid shader type!");
		return;
	}
}

void ShaderManager::calcNextVSToken() {
	++m_vsToken;
}

void ShaderManager::calcNextPSToken() {
	++m_psToken;
}

bool ShaderManager::readFile(const char* filePath, std::shared_ptr<byte>& byteCode, size_t& size) {
	std::fstream file = std::fstream(filePath, std::ios::in | std::ios::binary);
	if (!file.is_open()) {
		MLOG(LL, "ShaderManager::ReadFile Can not open file: ", LW, filePath);
		return false;
	}
	byte* tmpCode = NULL;
	file.seekg(0, std::ios::end);
	size = file.tellg();
	file.seekg(0, std::ios::beg);
	tmpCode = new byte[size];
	file.read(reinterpret_cast<char*>(tmpCode), size);
	byteCode.reset(tmpCode);
	file.close();
	return true;
}

bool ShaderManager::createVertexShader(std::shared_ptr<byte>& byteCode, size_t& size, Shader& output) {
	static const char* funcTag = "ShaderManager::CreateVertexShader: ";
	Microsoft::WRL::ComPtr<ID3D11VertexShader> tmpVS;
	HRESULT hr;
	hr = (*m_dev)->CreateVertexShader(byteCode.get(), size, 0, tmpVS.ReleaseAndGetAddressOf());
	if (FAILED(hr)) {
		MLOG(LL, funcTag, LL, "create shader failed!");
		return false;
	}
	auto iter = m_vs.insert(std::make_pair(m_vsToken, tmpVS));
	if (!iter.second) {
		MLOG(LL, funcTag, LL, "insert shader failed!");
		return false;
	}
	output = m_vsToken;
	calcNextVSToken();
	return true;
}

bool ShaderManager::createPixelShader(std::shared_ptr<byte>& byteCode, size_t& size, Shader& output) {
	static const char* funcTag = "ShaderManager::CreatePixelShader: ";
	Microsoft::WRL::ComPtr<ID3D11PixelShader> tmpPS;
	HRESULT hr;
	hr = (*m_dev)->CreatePixelShader(byteCode.get(), size, 0, tmpPS.ReleaseAndGetAddressOf());
	if (FAILED(hr)) {
		MLOG(LL, funcTag, LL, "create shader failed!");
		return false;
	}
	auto iter = m_ps.insert(std::make_pair(m_psToken, tmpPS));
	if (!iter.second) {
		MLOG(LL, funcTag, LL, "insert shader failed!");
		return false;
	}
	output = m_psToken;
	calcNextPSToken();
	return true;
}
