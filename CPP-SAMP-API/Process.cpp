#include "Process.h"
#include "algorithm.h"

#include <TlHelp32.h>

CProcess::CProcess(const std::string& strProcName) : m_hHandle(INVALID_HANDLE_VALUE), m_strProcName(strProcName)
{
	openProcess();
}


CProcess::~CProcess()
{
	m_vecAllocations.clear();

	if (m_hHandle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hHandle);
		m_hHandle = INVALID_HANDLE_VALUE;
	}
}

bool CProcess::openProcess()
{
	if (m_hHandle != INVALID_HANDLE_VALUE)
		CloseHandle(m_hHandle);

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
			if ((m_hHandle = OpenProcess(STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xFFF, FALSE, entry.th32ProcessID)))
				bRet = true;
			else
				m_hHandle = INVALID_HANDLE_VALUE;

			break;
		}

	} while (Process32Next(hSnapShot, &entry));

	CloseHandle(hSnapShot);
}

std::weak_ptr<CRemoteMemoryAllocation> CProcess::allocMemory(SIZE_T dwSize)
{
	std::shared_ptr<CRemoteMemoryAllocation> alloc(new CRemoteMemoryAllocation(this));

	if (!alloc->allocMemory(dwSize))
		return std::weak_ptr<CRemoteMemoryAllocation>();

	m_vecAllocations.insert(alloc);
	return alloc;
}

bool CProcess::freeMemory(std::weak_ptr<CRemoteMemoryAllocation> spMemory)
{
	try
	{
		auto p = spMemory.lock();
		return p->freeMemory() && m_vecAllocations.erase(p->shared_from_this());
	}
	catch (...){
	}

	return false;
}

HANDLE CProcess::handle() const
{
	return m_hHandle;
}

const CProcess::RemoteMemoryAllocations& CProcess::memoryAllocations() const
{
	return m_vecAllocations;
}