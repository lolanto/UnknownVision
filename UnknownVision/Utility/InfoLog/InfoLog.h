#pragma once
#include <string>
#include <iostream>
#include <Windows.h>

// LL: Log_Log ��ͨ�����Ϣ
// LW: Log_Warn ���������Ϣ
// LE: Log_Error ���������Ϣ
enum INFO_LEVEL {
	LL,
	LW,
	LE
};

class InfoLog {
public:
	static InfoLog& GetInstance();
public:
	template<typename T>
	void log(INFO_LEVEL level, T t) {
		switch (level) {
		case LW:
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 14);
			break;
		case LE:
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
			break;
		case LL:
		default:
			break;
		}
		std::cout << t << std::endl;
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
	}
	template<typename T, typename... Args>
	void log(INFO_LEVEL level, T t, Args... args) {
		switch (level) {
		case LW:
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 14);
			break;
		case LE:
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
			break;
		case LL:
		default:
			break;
		}
		std::cout << t << ' ';
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
		log(args...);
	}
};

#ifndef MLOG
#define MLOG(info_level, t, ...) InfoLog::GetInstance().log(info_level, t, ##__VA_ARGS__);
#endif // !MLOG
