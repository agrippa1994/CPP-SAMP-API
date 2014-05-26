#pragma once
#include "windows.h"
#include <string>
#include <vector>

class CProcess
{
	typedef std::vector<LPVOID> RemoteMemoryAllocations;
	typedef RemoteMemoryAllocations::iterator RemoteMemoryAllocationIterator;

	HANDLE m_hHandle = INVALID_HANDLE_VALUE;
	std::string m_strProcName;

	RemoteMemoryAllocations m_vecAllocations;

public:
	CProcess(const std::string& strProcName);
	~CProcess();

	template<typename AddrType, typename ValueType>
	bool writeMemory(AddrType addr, ValueType val)
	{
		return writeMemory(addr, (const ValueType&) val);
	}

	template<typename AddrType, typename ValueType>
	bool writeMemory(AddrType addr, const ValueType& val)
	{
		if (m_hHandle == INVALID_HANDLE_VALUE)
			return false;

		SIZE_T uiWritten = 0;
		BOOL bRet = WriteProcessMemory(_handle, (LPVOID) addr, (LPCVOID) &val, sizeof(ValueType), &uiWritten);

		return bRet > 0 && uiWritten == sizeof(ValueType);
	}

	RemoteMemoryAllocationIterator allocMemory(SIZE_T dwSize);
	RemoteMemoryAllocationIterator freeMemory(RemoteMemoryAllocationIterator iterator);

private:
	bool openProcess();
};

