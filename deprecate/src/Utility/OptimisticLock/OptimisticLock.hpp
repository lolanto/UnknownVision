#pragma once
#include <atomic>

/** 乐观锁，假如需要加锁处理的内容非常简单
* 则可以考虑使用该方式进行加/解锁，让程序陷入忙等中
* 减少context的切换 */

struct OptimisticLock {
	std::atomic_bool signal;
	OptimisticLock() : signal(false) {}
	void lock() {
		bool unlock = false;
		while (!signal.compare_exchange_strong(unlock, true));
		return;
	}
	void unlock() {
		bool lock = true;
		while (!signal.compare_exchange_strong(lock, false));
		return;
	}
};