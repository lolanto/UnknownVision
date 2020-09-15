#pragma once
#include "../UVType.h"

BEG_NAME_SPACE

class ShaderResourceView {
public:
	virtual ~ShaderResourceView() = default;
};

class ConstantBufferView {
public:
	virtual ~ConstantBufferView() = default;
};

class UnorderAccessView {
public:
	virtual ~UnorderAccessView() = default;
};

class RenderTargetView {
public:
	virtual ~RenderTargetView() = default;
};

class DepthStencilView {
public:
	virtual ~DepthStencilView() = default;
};

class VertexBufferView {
public:
	virtual ~VertexBufferView() = default;
};

class IndexBufferView {
public:
	virtual ~IndexBufferView() = default;
};

END_NAME_SPACE
