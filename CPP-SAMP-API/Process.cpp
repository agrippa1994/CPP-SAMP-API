#include "Process.h"
#include "algorithm.h"

#include <TlHelp32.h>

CProcess::CProcess(const std::string& strProcName) : m_strProcName(strProcName)
{
	openProcess();
}


CProcess::~CProcess()
{
	if (m_hHandle != INVALID_HANDLE_VALUE)
	{
		for (auto it = m_vecAllocations.begin(); it != m_vecAllocations.end(); it++)
			it = freeMemory(it);

		CloseHandle(m_hHandle);
		m_hHandle = INVALID_HANDLE_VALUE;
	}
}

bool CProcess::openProcess()
{
	bool bRet = false;

	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapShot == INVALID_HANDLE_VALUE)
		return false;

	Process32First(hSnapShot, &entry);
	do
	{
		if (ifind(std::string(entry.szExeFile), m_strProcName))
		{
			if ((m_hHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID)))
				bRet = true;
			else
				m_hHandle = INVALID_HANDLE_VALUE;

			break;
		}

	} while (Process32Next(hSnapShot, &entry));

	CloseHandle(hSnapShot);
}

CProcess::RemoteMemoryAllocationIterator CProcess::allocMemory(SIZE_T dwSize)
{
	RemoteMemoryAllocationIterator iterator = m_vecAllocations.end();

	if (dwSize == 0)
		return iterator;

	if (m_hHandle == INVALID_HANDLE_VALUE)
		return iterator;

	LPVOID lpMemory = VirtualAllocEx(m_hHandle, NULL, dwSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (lpMemory == NULL)
		return iterator;

	return m_vecAllocations.insert(m_vecAllocations.end(), lpMemory);
}

CProcess::RemoteMemoryAllocationIterator CProcess::freeMemory(CProcess::RemoteMemoryAllocationIterator iterator)
{
	LPVOID lpMemory = *iterator;
	BOOL bRet = VirtualFreeEx(m_hHandle, lpMemory, 0, MEM_RELEASE);

	return m_vecAllocations.erase(iterator);
}