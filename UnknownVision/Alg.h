#pragma once
class DXRenderer;
class MainClass;

#define DefaultParameters DXRenderer* renderer, MainClass* mc
#define BindSourceUA(t, sbt, slot)  BindSource(static_cast<IUnorderAccess*>(t),sbt, slot)
#define BindSourceTex(t, sbt, slot) BindSource(static_cast<ITexture*>(t), sbt, slot)

#define ToUA(t) static_cast<IUnorderAccess*>(t)
#define ToTex(t) static_cast<ITexture*>(t)

const float WIDTH = 960.0f;
const float HEIGHT = 960.0f;

void ImageBasedLighting(DefaultParameters);

void ScreenSpaceRayTracing(DefaultParameters);

void LTC(DefaultParameters);

void PullPush(DefaultParameters);