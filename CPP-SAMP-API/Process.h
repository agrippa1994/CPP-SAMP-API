#pragma once
#include "windows.h"
#include "RemoteMemoryAllocation.h"

#include <string>
#include <set>
#include <memory>

class CProcess
{
	typedef std::set< std::shared_ptr<CRemoteMemoryAllocation> > RemoteMemoryAllocations;

	HANDLE m_hHandle = INVALID_HANDLE_VALUE;
	std::string m_strProcName;

	RemoteMemoryAllocations m_vecAllocations;

public:
	CProcess(const std::string& strProcName);
	~CProcess();

	template<typename AddrType, typename ValueType>
	bool writeMemory(AddrType addr, const ValueType& val)
	{
		if (m_hHandle == INVALID_HANDLE_VALUE)
			return false;

		SIZE_T uiWritten = 0;
		BOOL bRet = WriteProcessMemory(m_hHandle, (LPVOID) addr, (LPCVOID) &val, sizeof(ValueType), &uiWritten);

		return bRet > 0 && uiWritten == sizeof(ValueType);
	}

	std::weak_ptr<CRemoteMemoryAllocation> allocMemory(SIZE_T dwSize);
	bool freeMemory(std::weak_ptr<CRemoteMemoryAllocation> spMemory);

	HANDLE handle() const;
	const RemoteMemoryAllocations& memoryAllocations() const;

private:
	bool openProcess();
};

