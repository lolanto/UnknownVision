#include "Material.h"
#include "Texture.h"
#include "Shader.h"
#include "Sampler.h"
#include "InfoLog.h"

ConstantBufferState::ConstantBufferState(ConstantBuffer* cb,
	ShaderBindTarget cbbt, UINT slot)
	: buffer(cb), target(cbbt), slot(slot) {}

TextureState::TextureState(ITexture* it, ShaderBindTarget sbt, UINT slot)
	: texture(it), target(sbt), slot(slot) {}

SamplerState::SamplerState(Sampler* sa, ShaderBindTarget sbt, UINT slot)
	: sampler(sa), target(sbt), slot(slot) {}

Material::Material() 
	: m_inputInterface(NULL), m_outputInterface(NULL),
	m_vertexShader(NULL), m_pixelShader(NULL), m_geometryShader(NULL),
	m_enable(false) {}

///////////////////
// public function
///////////////////

bool Material::Setup(ID3D11Device*) { return true; }

void Material::Bind(ID3D11DeviceContext* devCtx) {
	m_vertexShader->Bind(devCtx);
	m_pixelShader->Bind(devCtx);
	if (m_geometryShader) m_geometryShader->Bind(devCtx);
	// set constant buffer
	for (auto iter = m_bufStates.begin(), end = m_bufStates.end(); iter != end; ++iter) {
		iter->buffer->SetTarget(iter->target);
		iter->buffer->SetSlot(iter->slot);
		iter->buffer->Bind(devCtx);
	}
	// set sampler state
	for (auto iter = m_samplerStates.begin(), end = m_samplerStates.end(); iter != end; ++iter) {
		iter->sampler->SetSlot(iter->slot);
		iter->sampler->SetTarget(iter->target);
		iter->sampler->Bind(devCtx);
	}
	// set texture
	for (auto iter = m_texStates.begin(), end = m_texStates.end(); iter != end; ++iter) {
		switch (iter->target)
		{
		case SBT_VERTEX_SHADER:
			devCtx->VSSetShaderResources(iter->slot, 1, iter->texture->GetSRV());
			break;
		case SBT_PIXEL_SHADER:
			devCtx->PSSetShaderResources(iter->slot, 1, iter->texture->GetSRV());
			break;
		default:
			break;
		}
	}
}

void Material::Unbind(ID3D11DeviceContext* devCtx) {
	static ID3D11ShaderResourceView* NULLPTR[] = { NULL };
	m_vertexShader->Unbind(devCtx);
	m_pixelShader->Unbind(devCtx);
	if (m_geometryShader) m_geometryShader->Unbind(devCtx);
	// unbind constant buffer
	for (auto iter = m_bufStates.begin(), end = m_bufStates.end(); iter != end; ++iter) {
		iter->buffer->Unbind(devCtx);
	}
	// unbind sampler state
	for (auto iter = m_samplerStates.begin(), end = m_samplerStates.end(); iter != end; ++iter) {
		iter->sampler->Unbind(devCtx);
	}
	// unbind texture
	for (auto iter = m_texStates.begin(), end = m_texStates.end(); iter != end; ++iter) {
		switch (iter->target) {
		case SBT_VERTEX_SHADER:
			devCtx->VSSetShaderResources(iter->slot, 1, NULLPTR);
			break;
		case SBT_PIXEL_SHADER:
			devCtx->PSSetShaderResources(iter->slot, 1, NULLPTR);
			break;
		default:
			break;
		}
	}
}

bool Material::SetVertexShader(VertexShader1* vs) {
	// 设置新的vs之前，先将之前的vs以及与其绑定的constant buffer删除
	if (m_vertexShader) removeVertexShader();
	if (!vs) return true;
	m_vertexShader = vs;
	m_inputInterface = vs->GetInputLayout();
	return true;
}

bool Material::SetPixelShader(PixelShader1* ps) {
	// 设置新的ps之前，先将之前的ps以及与其绑定的constant buffer 删除
	if (m_pixelShader) removePixelShader();
	if (!ps) return true;
	m_pixelShader = ps;
	m_outputInterface = ps->GetOutputLayout();
	return true;
}

bool Material::SetGeometryShader(GeometryShader1* gs) {
	if (m_geometryShader) removeGeometryShader();
	if (!gs) return true;
	m_geometryShader = gs;
	return true;
}

bool Material::SetConstantBuffer(ConstantBuffer* cb, ShaderBindTarget cbbt, UINT slot) {
	static const char* funcTag = "Material::SetConstantBuffer: ";
	if (!cb->IsUsable()) {
		MLOG(LL, funcTag, LW, "Invalid constant buffer!");
		return false;
	}
	// 获得绑定管线阶段的描述
	ShaderDesc* shaderDesc = NULL;
	switch (cbbt) {
	case SBT_VERTEX_SHADER:
		if (!m_vertexShader) {
			MLOG(LL, funcTag, LW, "there is no vertex shader yet!");
			return false;
		}
		shaderDesc = m_vertexShader->GetDescription();
		break;
	case SBT_PIXEL_SHADER:
		if (!m_pixelShader) {
			MLOG(LL, funcTag, LW, "there is no pixel shader yet!");
			return false;
		}
		shaderDesc = m_pixelShader->GetDescription();
		break;
	case SBT_GEOMETRY_SHADER:
		if (!m_geometryShader) {
			MLOG(LL, funcTag, LW, "there is no geometry shader yet!");
			return false;
		}
		shaderDesc = m_geometryShader->GetDescription();
		break;
	default:
		MLOG(LL, funcTag, LE, "invalid binding target!");
		return false;
	}
	// 检查目标接口和该缓冲区是否匹配
	ConstBufferDesc* bufInterface = NULL; // 需要绑定的接口
	for (auto iter = shaderDesc->constBuffers.begin(), end = shaderDesc->constBuffers.end();
		iter != end; ++iter) {
		if (iter->slot == slot) {
			bufInterface = iter._Ptr;
			break;
		}
	}
	if (!bufInterface) {
		// 当前shader上并没有可以绑定的接口
		MLOG(LL, funcTag, LW, "Binding target or slot is unvalid!");
		return false;
	}
	if (!ConstantBuffersTest(cb->GetDescription(), bufInterface)) {
		// 绑定的目标和当前的缓冲区不匹配
		MLOG(LL, funcTag, LW, "Binding target and source buffer is not match!");
		return false;
	}
	// 检查当前绑定位置是否已经有绑定
	for (auto iter = m_bufStates.begin(), end = m_bufStates.end(); iter != end; ++iter) {
		if (iter->target == cbbt && iter->slot == slot) {
			// 删除当前绑定
			m_bufStates.erase(iter);
			break;
		}
	}
	m_bufStates.push_back(ConstantBufferState(cb, cbbt, slot));
	return true;
}

bool Material::RemoveConstantBuffer(ConstantBuffer* cb) {
	for (auto iter = m_bufStates.begin(), end = m_bufStates.end(); iter != end; ++iter) {
		if (iter->buffer == cb) {
			m_bufStates.erase(iter);
			return true;
		}
	}
	MLOG(LL, "Material::RemoveConstantBuffer: ", LW, "No such buffer!");
	return false;
}

bool Material::SetTexture(ITexture* it, ShaderBindTarget sbt, UINT slot) {
	static const char* funcTag = "Material::SetTexture: ";
	// 获得绑定管线阶段的描述
	ShaderDesc* sd = NULL;
	switch (sbt)
	{
	case SBT_VERTEX_SHADER:
		if (!m_vertexShader) {
			MLOG(LE, funcTag, LL, "There is no vertex shader!");
			return false;
		}
		sd = m_vertexShader->GetDescription();
		break;
	case SBT_PIXEL_SHADER:
		if (!m_pixelShader) {
			MLOG(LE, funcTag, LL, "There is no pixel shader!");
			return false;
		}
		sd = m_pixelShader->GetDescription();
		break;
	case SBT_GEOMETRY_SHADER:
		if (!m_geometryShader) {
			MLOG(LE, funcTag, LL, "There is no geometry shader!");
			return false;
		}
		sd = m_geometryShader->GetDescription();
	default:
		MLOG(LL, funcTag, LW, "Invalid binding type!");
		return false;
	}
	// 检查绑定的贴图与当前shader中的接口是否匹配
	TextureDesc* td = NULL;
	for (auto iter = sd->textures.begin(), end = sd->textures.end(); iter != end; ++iter) {
		if (iter->slot == slot) {
			td = iter._Ptr;
			break;
		}
	}
	if (!td) {
		MLOG(LL, funcTag, LW, "No such binding point!");
		return false;
	}
	// 检查当前绑定口是否已经有绑定的贴图
	for (auto iter = m_texStates.begin(), end = m_texStates.end(); iter != end; ++iter) {
		if (iter->slot == slot && iter->target == sbt) {
			m_texStates.erase(iter);
			break;
		}
	}
	m_texStates.push_back({ it, sbt, slot });
	return true;
}

bool Material::RemoveTexture(ITexture* it) {
	for (auto iter = m_texStates.begin(), end = m_texStates.end(); iter != end; ++iter) {
		if (iter->texture == it) {
			m_texStates.erase(iter);
			return true;
		}
	}
	return false;
}

bool Material::SetSamplerState(Sampler* sa, ShaderBindTarget sbt, UINT slot) {
	// 检查当前是否已经有绑定
	for (auto iter = m_samplerStates.begin(), end = m_samplerStates.end(); iter != end; ++iter) {
		if (iter->slot == slot && iter->target == sbt) {
			m_samplerStates.erase(iter);
			break;
		}
	}
	m_samplerStates.push_back({ sa, sbt, slot });
	return true;
}

bool Material::RemoveSamplerState(Sampler* sa) {
	for (auto iter = m_samplerStates.begin(), end = m_samplerStates.end(); iter != end; ++iter) {
		if (iter->sampler == sa) {
			m_samplerStates.erase(iter);
			return true;
		}
	}
	return false;
}

///////////////////
// private function
///////////////////
bool Material::checkEnable() {
	m_enable = true;
	if (!m_inputInterface) m_enable = false;
	if (!m_outputInterface) m_enable = false;
	if (!m_vertexShader) m_enable = false;
	if (!m_pixelShader) m_enable = false;
	return m_enable;
}

void Material::removeVertexShader() {
	m_vertexShader = NULL;
	m_inputInterface = NULL;
	// 删除所有绑定的缓冲区
	for (auto iter = m_bufStates.begin(), end = m_bufStates.end(); iter != end;) {
		if (iter->target == SBT_VERTEX_SHADER) iter = m_bufStates.erase(iter);
		else ++iter;
	}
}

void Material::removePixelShader() {
	m_pixelShader = NULL;
	m_outputInterface = NULL;
	// 删除所有绑定的缓冲区
	for (auto iter = m_bufStates.begin(), end = m_bufStates.end(); iter != end;) {
		if (iter->target == SBT_PIXEL_SHADER) iter = m_bufStates.erase(iter);
		else ++iter;
	}
}

void Material::removeGeometryShader() {
	m_geometryShader = NULL;
	// 删除所有绑定的缓冲区
	for (auto iter = m_bufStates.begin(), end = m_bufStates.end(); iter != end;) {
		if (iter->target == SBT_GEOMETRY_SHADER) iter = m_bufStates.erase(iter);
		else ++iter;
	}
}
