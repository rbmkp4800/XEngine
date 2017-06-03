#include <Unknwn.h>

#include "Util.COMPtr.h"

void COMPtrInternal::IUnknown_Release(void* ptr)
{
	((IUnknown*)ptr)->Release();
}