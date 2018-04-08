#pragma once
class DXRenderer;
class MainClass;

#define DefaultParameters DXRenderer* renderer, MainClass* mc
#define BindSourceUA(t, sbt, slot)  BindSource(reinterpret_cast<IUnorderAccess*>(t),sbt, slot)
#define BindSourceTex(t, sbt, slot) BindSource(reinterpret_cast<ITexture*>(t), sbt, slot)

const float WIDTH = 960.0f;
const float HEIGHT = 960.0f;

void ImageBasedLighting(DefaultParameters);

void ScreenSpaceRayTracing(DefaultParameters);

void LTC(DefaultParameters);