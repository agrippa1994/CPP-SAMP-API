#include "RemoteMemoryAllocation.h"
#include "Process.h"

CRemoteMemoryAllocation::CRemoteMemoryAllocation(CProcess *pProcess) : m_pProcess(pProcess), m_pMemory(NULL)
{
}


CRemoteMemoryAllocation::~CRemoteMemoryAllocation()
{
	freeMemory();
}

bool CRemoteMemoryAllocation::allocMemory(SIZE_T dwSize)
{
	if (m_pMemory != NULL)
		return false;

	if (m_pProcess->handle() == INVALID_HANDLE_VALUE)
		return false;

	return VirtualAllocEx(m_pProcess->handle(), NULL, dwSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE) > 0;
}

bool CRemoteMemoryAllocation::freeMemory()
{
	if (m_pMemory == NULL)
		return false;

	if (m_pProcess->handle() == INVALID_HANDLE_VALUE)
		return false;

	return VirtualFreeEx(m_pProcess->handle(), m_pMemory, 0, MEM_RELEASE) > 0;
}

LPVOID CRemoteMemoryAllocation::memory() const
{
	return m_pMemory;
}