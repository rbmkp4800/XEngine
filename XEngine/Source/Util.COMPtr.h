#pragma once

#include <guiddef.h>

#include <XLib.NonCopyable.h>

namespace COMPtrInternal
{
	void IUnknown_AddRef(void* ptr);
	void IUnknown_Release(void* ptr);
}

template <typename Type>
class COMPtr : public XLib::NonCopyable
{
private:
	Type *ptr;

public:
	inline COMPtr() : ptr(nullptr) {}
	inline ~COMPtr() { destroy(); }

	inline void destroy()
	{
		if (ptr)
		{
			COMPtrInternal::IUnknown_Release(ptr);
			ptr = nullptr;
		}
	}

	Type* moveToPtr() { Type* result = ptr; ptr = nullptr; return result; }

	inline Type** initRef()
	{
		destroy();
		return &ptr;
	}
	inline void** voidInitRef() { return (void**)initRef(); }

	inline Type*const* operator & () { return &ptr; }

	inline Type* operator -> () { return ptr; }
	inline operator Type* () { return ptr; }
	inline void operator = (Type* value)
	{
		destroy();
		ptr = value;
		if (ptr)
			COMPtrInternal::IUnknown_AddRef(ptr);
	}

	inline GUID uuid() { return __uuidof(Type); }

	inline bool isInitialized() { return ptr ? true : false; }
};