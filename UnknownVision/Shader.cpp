#define INITGUID
#include <fstream>
#include <iostream>
#include <d3d11shader.h>
#include "Shader.h"
#include "InfoLog.h"
using std::string;
using std::fstream;
///////////////////
// Document Private Function
///////////////////

bool readByteCodeFromFile(string& filePath, Microsoft::WRL::ComPtr<ID3DBlob>& blob) {
	fstream file = fstream(filePath.c_str(), std::ios::in | std::ios::binary);
	if (!file.is_open()) {
		MLOG(LL, "readByteCodeFromFile: filepath is invalid!");
		return false;
	}
	size_t size = -1;
	file.seekg(0, std::ios::end);
	size = file.tellg();
	file.seekg(0, std::ios::beg);
	if (FAILED(D3DCreateBlob(size, blob.ReleaseAndGetAddressOf()))) {
		MLOG(LL, "readByteCodeFromFile: create blob failed!");
		file.close();
		return false;
	}
	file.read(reinterpret_cast<char*>(blob.Get()->GetBufferPointer()), size);
	file.close();
	return true;
}

bool compileFromFile(string& filePath, Microsoft::WRL::ComPtr<ID3DBlob>& blob, LPCSTR target) {
	return false;
}

void shaderFactoryFunc(string& filePath, Microsoft::WRL::ComPtr<ID3DBlob>& blob, LPCSTR target) {
	string fileType = filePath.substr(filePath.rfind("."));
	if (!fileType.compare(".hlsl")) {
		// type is hlsl
		if (!compileFromFile(filePath, blob, target)) {
			MLOG(LL, "shaderFactoryFunc: compile shader from file failed!");
			abort();
		}
	}
	else if (!fileType.compare(".cso")) {
		// type is cso
		if (!readByteCodeFromFile(filePath, blob)) {
			MLOG(LL, "shaderFactoryFunc: read byte code from file failed!");
			abort();
		}
	}
	else {
		MLOG(LL, "shaderFactoryFunc: invalid filePath!");
		abort();
	}
}

///////////////////
// IShader Static Function
///////////////////
void IShader::ShaderFactory(string& filePath, Microsoft::WRL::ComPtr<ID3DBlob>& blob, LPCSTR target) {
	string fileType = filePath.substr(filePath.rfind("."));
	if (!fileType.compare(".hlsl")) {
		// type is cso
		if (!compileFromFile(filePath, blob, target)) {
			MLOG(LL, "VertexShader::VertexShader: compile shader from file failed!");
			abort();
		}
	}
	else if (!fileType.compare(".cso")) {
		// type is hlsl
		if (!readByteCodeFromFile(filePath, blob)) {
			MLOG(LL, "VertexShader::VertexShader: read byte code from file failed!");
			abort();
		}
	}
	else {
		MLOG(LL, "VertexShader::VertexShader: invalid filePath!");
		abort();
	}
}


/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   ShaderAnalyser   //////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

ShaderAnalyser::ShaderAnalyser(IShader* s) : m_target(s) {}

///////////////////
// public function
///////////////////
void ShaderAnalyser::Analyse(std::vector<ShaderConstBufDesc>& analyseResult) {
	using namespace std;
	HRESULT hr;
	if (!m_target) cout << "ShaderAnalyser::Analyse: Invalid Shader!" << endl;
	cout << "Start Shader Analyse!" << endl;
	hr = D3DReflect(m_target->GetByteCode(), m_target->GetByteCodeSize(), 
		IID_ID3D11ShaderReflection, (void**)m_reflection.ReleaseAndGetAddressOf());
	if (FAILED(hr)) {
		cout << "Create Reflect failed! Stop analysing!" << endl;
		return;
	}
	hr = m_reflection->GetDesc(&m_shaderDesc);
	if (FAILED(hr)) {
		cout << "Get Shader description failed!" << endl;
		return;
	}
	cout << "This shader has " << m_shaderDesc.ConstantBuffers << " constant buffers" << endl;
	cout << "This shader has " << m_shaderDesc.BoundResources << " resources" << endl;
	cout << "Start Analysing the constant buffer" << endl;
	for (auto i = 0; i < m_shaderDesc.ConstantBuffers; ++i) {
		ShaderConstBufDesc scbd;
		analyseConstantBuffer(i, scbd);
		analyseResult.push_back(scbd);
	}
}

///////////////////
// private function
///////////////////
void ShaderAnalyser::analyseConstantBuffer(UINT index, ShaderConstBufDesc& scbd) {
	using namespace std;
	HRESULT hr;
	ID3D11ShaderReflectionConstantBuffer* tmpCBPtr = NULL;
	ID3D11ShaderReflectionVariable* tmpVarPtr = NULL;
	D3D11_SHADER_BUFFER_DESC shaderBufDesc;

	tmpCBPtr = m_reflection->GetConstantBufferByIndex(index);
	if (!tmpCBPtr) {
		cout << "ShaderAnalyser::analyseConstantBuffer: Invalid index!" << endl;
		return;
	}
	cout << "Analysing Constant Buffer " << index << endl;
	hr = tmpCBPtr->GetDesc(&shaderBufDesc);
	if (FAILED(hr)) {
		cout << "Can not get constant buffer description!" << endl;
		return;
	}
	D3D11_SHADER_INPUT_BIND_DESC bindDesc;
	m_reflection->GetResourceBindingDescByName(shaderBufDesc.Name, &bindDesc);
	cout << "Constant buffer name: " << shaderBufDesc.Name << endl
		<< "Variables: " << shaderBufDesc.Variables << endl
		<< "Size in byte: " << shaderBufDesc.Size << endl
		<< "Binding slot: " << bindDesc.BindPoint << endl;
	scbd.size = shaderBufDesc.Size;
	scbd.slot = bindDesc.BindPoint;
	cout << "Start Analysing Variables in this Constant Buffer!" << endl;
	for (auto i = 0; i < shaderBufDesc.Variables; ++i) {
		tmpVarPtr = tmpCBPtr->GetVariableByIndex(i);
		if (!tmpVarPtr) {
			cout << "Invalid variable index!" << endl;
			continue;
		}
		ShaderConstBufVarDesc scbvd;
		analyseCBVariable(i, tmpVarPtr, scbvd);
		scbd.variables.push_back(scbvd);
		tmpVarPtr = NULL;
	}
}

void ShaderAnalyser::analyseCBVariable(UINT index, ID3D11ShaderReflectionVariable*& ref, ShaderConstBufVarDesc& scbvd) {
	using namespace std;
	HRESULT hr;
	D3D11_SHADER_VARIABLE_DESC varDesc;
	D3D11_SHADER_TYPE_DESC shaderTypeDesc;
	hr = ref->GetDesc(&varDesc);
	if (FAILED(hr)) {
		cout << "ShaderAnalyser::analyseCBVariable: Can not get Variable description!" << endl;
		return;
	}
	// 暂时没有分析variable为struct的状况
	ref->GetType()->GetDesc(&shaderTypeDesc);
	cout << "Variable Name: " << varDesc.Name << endl
		<< "StartOffset: " << varDesc.StartOffset << endl
		<< "Size: " << varDesc.Size << endl;
	scbvd.name = varDesc.Name;
	scbvd.offset = varDesc.StartOffset;
	scbvd.size = varDesc.Size;
	scbvd.varClass = shaderTypeDesc.Class;
	scbvd.varType = shaderTypeDesc.Type;
}

void analyseVariable(BufferVariableDesc& desc, ID3D11ShaderReflectionVariable*& ref) {
	D3D11_SHADER_VARIABLE_DESC svd;
	if (FAILED(ref->GetDesc(&svd))) {
		MLOG(LL, "analyseVariable: can not get variable description!");
		return;
	}
	desc.name = svd.Name;
	desc.offset = svd.StartOffset;
	desc.size = svd.Size;
	ID3D11ShaderReflectionType* srt = ref->GetType();
	D3D11_SHADER_TYPE_DESC std;
	if (FAILED(srt->GetDesc(&std))) {
		MLOG(LL, "analyseVariable: can not get variable type description!");
		return;
	}
	desc.varClass = std.Class;
	desc.varType = std.Type;
}

void analyseConstBuffer(ConstBufferDesc& desc, ID3D11ShaderReflection* reflect, UINT i) {
	ID3D11ShaderReflectionConstantBuffer* refBuf = reflect->GetConstantBufferByIndex(i);
	D3D11_SHADER_BUFFER_DESC bufDesc;
	D3D11_SHADER_INPUT_BIND_DESC bindDesc;
	if (FAILED(refBuf->GetDesc(&bufDesc))) {
		MLOG(LL, "analyseConstBuffer: can not get constant buffer description!");
		return;
	}
	if (FAILED(reflect->GetResourceBindingDescByName(bufDesc.Name, &bindDesc))) {
		MLOG(LL, "analyseConstBuffer: can not get constant buffer binding slot!");
		return;
	}
	desc.name = bufDesc.Name;
	desc.size = bufDesc.Size;
	desc.slot = bindDesc.BindPoint;
	for (auto t = 0; t < bufDesc.Variables; ++t) {
		BufferVariableDesc bvd;
		ID3D11ShaderReflectionVariable* varRef = refBuf->GetVariableByIndex(t);
		analyseVariable(bvd, varRef);
		desc.variables.push_back(bvd);
	}
}

bool analyseTexture(TextureDesc& desc, ID3D11ShaderReflection* reflect, UINT i) {
	D3D11_SHADER_INPUT_BIND_DESC sibd;
	if (FAILED(reflect->GetResourceBindingDesc(i, &sibd))) {
		MLOG(LL, "analyseTexture: can not get resource description!");
		return false;
	}
	if (sibd.Type != D3D_SIT_TEXTURE) return false;
	desc.name = sibd.Name;
	desc.slot = sibd.BindPoint;
	return true;
}

inline bool shaderReflectHelper(const void* byteCode, SIZE_T size, 
	Microsoft::WRL::ComPtr<ID3D11ShaderReflection>& ref) {
	HRESULT hr = S_OK;
	hr = D3DReflect(byteCode, size, IID_ID3D11ShaderReflection,
		reinterpret_cast<void**>(ref.ReleaseAndGetAddressOf()));
	if (FAILED(hr)) {
		MLOG(LL, "Can not get shader reflection!");
		return false;
	}
	return true;
}

void analyseShader(const void* byteCode, SIZE_T size, ShaderDesc& desc) {
	Microsoft::WRL::ComPtr<ID3D11ShaderReflection> reflection;
	if (!shaderReflectHelper(byteCode, size, reflection)) {
		MLOG(LL, "analyseShader failed!");
		return;
	}
	D3D11_SHADER_DESC shaderDesc;
	if (FAILED(reflection.Get()->GetDesc(&shaderDesc))) {
		MLOG(LL, "analyseShader: can not get shader description");
		return;
	}
	desc.constBufferNum = shaderDesc.ConstantBuffers;
	for (auto i = 0; i < shaderDesc.ConstantBuffers; ++i) {
		ConstBufferDesc cbd;
		analyseConstBuffer(cbd, reflection.Get(), i);
		desc.constBuffers.push_back(cbd);
	}
	desc.textureNum = 0;
	for (auto i = 0; i < shaderDesc.BoundResources; ++i) {
		TextureDesc td;
		if (analyseTexture(td, reflection.Get(), i)) {
			++desc.textureNum;
			desc.textures.push_back(td);
		}
	}
}

void analyseShaderParamInputLayout(const void* byteCode, SIZE_T size, std::vector<ParamIOLayout>& il) {
	Microsoft::WRL::ComPtr<ID3D11ShaderReflection> reflection;
	if (!shaderReflectHelper(byteCode, size, reflection)) {
		MLOG(LL, "analyseShaderInputLayout failed!");
		return;
	}
	D3D11_SHADER_DESC shaderDesc;
	if (FAILED(reflection.Get()->GetDesc(&shaderDesc))) {
		MLOG(LL, "analyseShaderInputLayout: can not get shader description!");
		return;
	}
	for (auto i = 0; i < shaderDesc.InputParameters; ++i) {
		D3D11_SIGNATURE_PARAMETER_DESC spd;
		ParamIOLayout tmp;
		reflection.Get()->GetInputParameterDesc(i, &spd);
		tmp.name = spd.SemanticName;
		tmp.index = spd.SemanticIndex;
		il.push_back(tmp);
	}
}

void analyseShaderParamOutputLayout(const void* byteCode, SIZE_T size, std::vector<ParamIOLayout>& ol) {
	Microsoft::WRL::ComPtr<ID3D11ShaderReflection> reflection;
	if (!shaderReflectHelper(byteCode, size, reflection)) {
		MLOG(LL, "analyseShaderParamOutputLayout failed!");
		return;
	}
	D3D11_SHADER_DESC shaderDesc;
	if (FAILED(reflection.Get()->GetDesc(&shaderDesc))) {
		MLOG(LL, "analyseShaderInputLayout: can not get shader description!");
		return;
	}
	for (auto i = 0; i < shaderDesc.OutputParameters; ++i) {
		D3D11_SIGNATURE_PARAMETER_DESC spd;
		ParamIOLayout tmp;
		reflection.Get()->GetOutputParameterDesc(i, &spd);
		tmp.name = spd.SemanticName;
		tmp.index = spd.SemanticIndex;
		ol.push_back(tmp);
	}
}

ShaderDesc* ShaderObject::GetDescription() { return &m_desc; }

ConstBufferDesc* ShaderObject::GetShaderCBuffer(UINT c) {
	if (m_desc.constBufferNum > c) return &m_desc.constBuffers[c];
	MLOG(LW, "ShaderObject::GetShaderCBuffer: no such constant buffer!");
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   VertexShader1   ///////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

VertexShader::VertexShader(string filePath) {
	shaderFactoryFunc(filePath, m_codeBlob, "vs_5_0");
	// analyse shader
	analyseShader(m_codeBlob.Get()->GetBufferPointer(), m_codeBlob.Get()->GetBufferSize(), m_desc);
	// analyse input layout
	analyseShaderParamInputLayout(m_codeBlob.Get()->GetBufferPointer(),
		m_codeBlob.Get()->GetBufferSize(), m_inputLayout);
}

bool VertexShader::Setup(ID3D11Device* dev) {
	if (FAILED(dev->CreateVertexShader(m_codeBlob->GetBufferPointer(),
		m_codeBlob->GetBufferSize(), NULL, m_shader.ReleaseAndGetAddressOf()))) {
		MLOG(LE, "VertexShader::Setup: Create Vertex Shader Failed!");
		return false;
	}
	return true;
}

void VertexShader::Bind(ID3D11DeviceContext* devCtx) {
	devCtx->VSSetShader(m_shader.Get(), NULL, 0);
}

void VertexShader::Unbind(ID3D11DeviceContext*) {}

std::vector<ParamIOLayout>* VertexShader::GetInputLayout() {
	return &m_inputLayout;
}

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   PixelShader1   /////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

PixelShader::PixelShader(string filePath) {
	shaderFactoryFunc(filePath, m_codeBlob, "ps_5_0");
	// analyse shader
	analyseShader(m_codeBlob.Get()->GetBufferPointer(), m_codeBlob.Get()->GetBufferSize(), m_desc);
	// analyse shader output
	analyseShaderParamOutputLayout(m_codeBlob.Get()->GetBufferPointer(),
		m_codeBlob.Get()->GetBufferSize(), m_outputLayout);
}

bool PixelShader::Setup(ID3D11Device* dev) {
	if (FAILED(dev->CreatePixelShader(m_codeBlob->GetBufferPointer(),
		m_codeBlob->GetBufferSize(), NULL, m_shader.ReleaseAndGetAddressOf()))) {
		MLOG(LE, "PixelShader::Setup: Create Pixel shader failed!");
		return false;
	}
	return true;
}

void PixelShader::Bind(ID3D11DeviceContext* devCtx) {
	devCtx->PSSetShader(m_shader.Get(), NULL, 0);
}

void PixelShader::Unbind(ID3D11DeviceContext* devCtx) {}

std::vector<ParamIOLayout>* PixelShader::GetOutputLayout() { return &m_outputLayout; }


/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   GeometryShader   /////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

GeometryShader::GeometryShader(string filePath) {
	shaderFactoryFunc(filePath, m_codeBlob, "gs_5_0");
	// analyse shader
	analyseShader(m_codeBlob->GetBufferPointer(), m_codeBlob->GetBufferSize(), m_desc);
	// TODO:
	// analyse shader output
}

bool GeometryShader::Setup(ID3D11Device* dev) {
	if (FAILED(dev->CreateGeometryShader(m_codeBlob->GetBufferPointer(),
		m_codeBlob->GetBufferSize(), NULL, m_shader.ReleaseAndGetAddressOf()))) {
		MLOG(LE, "GeometryShader::Setup Create Geometry shader failed!");
		return false;
	}
	return true;
}

void GeometryShader::Bind(ID3D11DeviceContext* devCtx) {
	devCtx->GSSetShader(m_shader.Get(), NULL, 0);
}

void GeometryShader::Unbind(ID3D11DeviceContext* devCtx) {
	devCtx->GSSetShader(NULL, NULL, 0);
}

/////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   Compute Shader   /////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

ComputeShader::ComputeShader(string filePath) {
	shaderFactoryFunc(filePath, m_codeBlob, "cs_5_0");
	analyseShader(m_codeBlob->GetBufferPointer(), m_codeBlob->GetBufferSize(), m_desc);
	// No output
}

bool ComputeShader::Setup(ID3D11Device* dev) {
	if (FAILED(dev->CreateComputeShader(m_codeBlob->GetBufferPointer(),
		m_codeBlob->GetBufferSize(), NULL, m_shader.ReleaseAndGetAddressOf()))) {
		MLOG(LE, "ComputeShader::Setup Create Compute shader failed!");
		return false;
	}
	return true;
}

void ComputeShader::Bind(ID3D11DeviceContext* devCtx) {
	devCtx->CSSetShader(m_shader.Get(), NULL, 0);
}

void ComputeShader::Unbind(ID3D11DeviceContext* devCtx) {
	devCtx->CSSetShader(NULL, NULL, 0);
}
