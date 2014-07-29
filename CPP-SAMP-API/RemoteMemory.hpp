#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <exception>

namespace SAMP
{
	// Class which holds memory and uses RAII for deinitialization
	class RemoteMemory
	{
		HANDLE m_hProc;
		LPVOID m_pMemory = 0;

	public:
		static const DWORD minAllocationSize = 1024;

		explicit RemoteMemory(HANDLE hProc) : m_hProc(hProc)
		{
			unsigned char nulls[minAllocationSize] = { 0 };

			m_pMemory = VirtualAllocEx(m_hProc, NULL, minAllocationSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

			if (m_pMemory)
				WriteProcessMemory(hProc, m_pMemory, nulls, minAllocationSize, 0);
			else
				throw std::exception("Memory couldn't be allocated!");
		}

		~RemoteMemory()
		{
			if (m_pMemory)
				VirtualFreeEx(m_hProc, m_pMemory, 0, MEM_RELEASE);
		}

		LPVOID address()
		{
			return m_pMemory;
		}

		operator LPVOID()
		{
			return m_pMemory;
		}
	};
}