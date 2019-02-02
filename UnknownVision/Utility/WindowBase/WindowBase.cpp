#include "WindowBase.h"
MainLoopFunctionPointer && WindowBase::MainLoop = [](float) {};
MouseEventCallBack&& WindowBase::MouseCallBack = [](float, float, float, float, float) {};
KeyboardEventCallBack&& WindowBase::KeyboardCallBack = [](uint32_t, bool, float) {};
