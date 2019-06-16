#ifndef INFO_LOG_H
#define INFO_LOG_H
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

#endif // INFO_LOG_H
