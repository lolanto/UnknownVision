#pragma once

#include "RenderDescriptor.h"

BEG_NAME_SPACE

/** 状态集合，资源绑定情况，pipeline的设置 */

class Renderable {
public:
	Renderable(const ProgramDescriptor& pd) : m_pd(pd) {}
	virtual ~Renderable() = default;
protected:
	const ProgramDescriptor& m_pd;
};

class MeshObject : public Renderable {
public:

};

END_NAME_SPACE
