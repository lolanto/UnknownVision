#pragma once
#include <string>
#include <iostream>
#include <atomic>

#define thread_safe

class InfoLog {
public:
	static InfoLog& GetInstance();
public:
	template<typename T>
	void Log(T t) thread_safe {
		bool lock = true;
		bool unlock = false;
		while(!signal.compare_exchange_strong(unlock, true));
		std::cout << t;
		signal.compare_exchange_strong(lock, false);
	}
private:
	std::atomic_bool signal;
};

#ifndef MLOG
#define MLOG(info) InfoLog::GetInstance().Log(info);
#endif // !MLOG

#ifndef FLOG
#define FLOG(format, ...) \
{ \
	char outputInfo[128] = { 0 }; \
	snprintf(outputInfo, 128, format, __VA_ARGS__); \
	InfoLog::GetInstance().Log(outputInfo); \
}
#endif // !FLOG

#ifndef LFLOG /**< long FLOG */
#define LFLOG(size, format, ...) \
{ \
	std::string outputInfo(size, 0); \
	snprintf(outputInfo.data(), size, format, __VA_ARGS__); \
	InfoLog::GetInstance().Log(outputInfo.c_str()); \
}
#endif // !LFLOG

