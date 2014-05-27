#pragma once
#include "windows.h"
#include <memory>

class CProcess;

class CRemoteMemoryAllocation : public std::enable_shared_from_this<CRemoteMemoryAllocation>
{
	CProcess *m_pProcess;
	LPVOID m_pMemory;

public:
	CRemoteMemoryAllocation(CProcess *pProcess);
	~CRemoteMemoryAllocation();

	bool allocMemory(SIZE_T dwSize);
	bool freeMemory();

	LPVOID memory() const;
};

