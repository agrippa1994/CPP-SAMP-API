#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <exception>
#include "InjectData.hpp"

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
			m_pMemory = VirtualAllocEx(m_hProc, NULL, minAllocationSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

			if (m_pMemory)
				clear();
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

		bool clear()
		{
			unsigned char nulls[minAllocationSize] = { 0 };

			if (m_pMemory)
				if (WriteProcessMemory(m_hProc, m_pMemory, nulls, minAllocationSize, 0))
					return true;
			
			return false;
		}

		template<typename T>
		bool write(T t)
		{
			if (!m_pMemory)
				return false;

			return WriteProcessMemory(m_hProc, m_pMemory, &t, s, 0) == TRUE;
		}

		template<typename T>
		bool writeArray(T t, size_t len)
		{
			if (!m_pMemory)
				return false;

			return WriteProcessMemory(m_hProc, m_pMemory, (LPCVOID) t, len, 0)  == TRUE;
		}

		bool operator()()
		{
			HANDLE hThread = CreateRemoteThread(m_hProc, 0, 0, (LPTHREAD_START_ROUTINE) (LPVOID) m_pMemory, 0, 0, 0);
			if (hThread == 0)
				return false;

			WaitForSingleObject(hThread, INFINITE);
			CloseHandle(hThread);

			return true;
		}

		bool operator=(const InjectData& data)
		{
			if (!clear())
				return false;

			return writeArray(data.raw().data(), data.raw().size());
		}
	};
}