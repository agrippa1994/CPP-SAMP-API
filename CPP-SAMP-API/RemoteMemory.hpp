/*
	Copyright(c) 2014, Manuel S.
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met :

	*Redistributions of source code must retain the above copyright notice, this
	list of conditions and the following disclaimer.

	* Redistributions in binary form must reproduce the above copyright notice,
	this list of conditions and the following disclaimer in the documentation
	and / or other materials provided with the distribution.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
	FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
	DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
	CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
	OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

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
