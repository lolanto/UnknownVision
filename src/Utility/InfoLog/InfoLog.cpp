#include <iostream>
#include "InfoLog.h"

using std::cout;
using std::endl;

InfoLog& InfoLog::GetInstance() {
	static InfoLog _instance;
	return _instance;
}
