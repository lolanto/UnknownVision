#pragma once
// ����������

#include <d3d11.h>
#include <vector>
#include <wrl.h>

class Plane {
public:
	Plane(float width, float height);
private:
	float										m_width;
	float										m_height;
};
