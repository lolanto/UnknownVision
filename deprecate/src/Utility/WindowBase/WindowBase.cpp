#include "WindowBase.h"
using namespace UVWindows;
MainLoopFunctionPointer WindowBase::MainLoop = [](float) {};
MouseEventCallBack WindowBase::MouseCallBack = [](float, float, MouseEvent, bool) {};
KeyboardEventCallBack WindowBase::KeyboardCallBack = [](KeyBoardEvent, bool) {};
