#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>

namespace XEngine
{
	enum class XEResultCode : uint32
	{
		Success = 0,
	};

	class XECountableBase abstract : public XLib::NonCopyable
	{
	private:
		uint32 instanceCount = 1;

	protected:
		virtual void destroy() = 0;

	public:
		inline void addReference() { instanceCount++; }
		inline void removeReference()
		{
			instanceCount--;
			if (instanceCount == 0)
				destroy();
		}
	};
}