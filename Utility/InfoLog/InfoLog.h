/** 自定义的简单Log工具，必须支持线程安全以及跨平台 */
#pragma once
#include <string>
#include <iostream>
#include <atomic>
#include <cstdlib>

class InfoLog {
public:
	template<class ...Args>
	static void Log(const char* fmt, Args... args) {
		bool lock = true;
		bool unlock = false;
		while(!signal.compare_exchange_strong(unlock, true));
		printf(fmt, args...);
		signal.compare_exchange_strong(lock, false);
	}
private:
	static std::atomic_bool signal;
};

#define LOG_INFO(fmt, ...) InfoLog::Log("[INFO]" __FUNCTION__ ": " fmt "\n", __VA_ARGS__);
#define LOG_WARN(fmt, ...) InfoLog::Log("[WARN]" __FUNCTION__ ": " fmt "\n", __VA_ARGS__);
#define LOG_ERROR(fmt, ...) InfoLog::Log("[ERROR]" __FUNCTION__ ": " fmt "\n", __VA_ARGS__);
