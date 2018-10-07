#pragma once
template<typename T, void(*deletor)(T*) = nullptr>
class SmartPtr;

template<typename T, void(*deletor)(T*) = nullptr>
SmartPtr<T, deletor> Make(T* ptr) {
	SmartPtr<T, deletor> ret;
	ret._ptr = ptr;
	ret._count = new size_t(1);
	return ret;
}

template<typename T, void(*deletor)(T*)>
class SmartPtr {
	friend  SmartPtr Make<T, deletor>(T*);
public:
	SmartPtr() : _ptr(nullptr), _count(nullptr) {}
	~SmartPtr() {
#ifdef DEBUG
		if (!_count ^ !_ptr) abort();
#endif // DEBUG
		if (!_count) return;
		--(*_count);
		if (!*_count) {
			if (deletor) deletor(_ptr);
			else delete _ptr;
			delete _count;
			_count = nullptr;
			_ptr = nullptr;
		}
	}
	SmartPtr(const SmartPtr& ref) {
		_ptr = ref._ptr;
		++(*ref._count);
		_count = ref._count;
	}
	const SmartPtr& operator=(const SmartPtr& rhs) {
		if (rhs._count) ++(*rhs._count);
		this->~SmartPtr();
		_ptr = rhs._ptr;
		_count = rhs._count;
		return *this;
	}
	SmartPtr(SmartPtr&& ref) {
		_count = ref._count;
		_ptr = ref._ptr;
		ref._count = nullptr;
		ref._ptr = nullptr;
	}
	const SmartPtr& operator=(SmartPtr&& rhs) {
		this->~SmartPtr();
		_ptr = rhs._ptr;
		_count = rhs._count;
		rhs._count = nullptr;
		rhs._ptr = nullptr;
		return *this;
	}
	bool operator!() { return !_ptr; }
	operator bool() { return _ptr; }
	T* operator->() { return _ptr; }
	T* const _Get() const { return _ptr; }
	size_t GetCount() const { return *_count; }
private:
	T * _ptr;
	size_t* _count;
};