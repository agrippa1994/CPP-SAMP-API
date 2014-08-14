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
#include <TlHelp32.h>

#include "Addresses.hpp"
#include "RemoteFunctionCaller.hpp"

namespace SAMP
{
	class SAMP
	{
		DWORD m_dwPID = 0;
		DWORD m_dwSAMPBase = 0;
		HANDLE m_hHandle = INVALID_HANDLE_VALUE;

		bool openProcess()
		{
			DWORD dwPID = 0;
            GetWindowThreadProcessId(FindWindowA(0, "GTA:SA:MP"), &dwPID);

			if (dwPID == 0)
			{
				m_hHandle = INVALID_HANDLE_VALUE;
				return false;
			}
				
			if (dwPID == m_dwPID && m_hHandle != INVALID_HANDLE_VALUE)
				return true;

			m_dwPID = dwPID;
			m_hHandle = OpenProcess(STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xFFF, FALSE, m_dwPID);
			if (m_hHandle == INVALID_HANDLE_VALUE)
				return false;

			return true;
		}

		bool openSAMP()
		{
			if (!openProcess())
				return false;

			HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, m_dwPID);
			if (hSnapshot == INVALID_HANDLE_VALUE)
				return false;

			MODULEENTRY32 entry;
			entry.dwSize = sizeof(MODULEENTRY32);

			Module32First(hSnapshot, &entry);
			do
			{
#ifdef UNICODE
                if (_wcsicmp(entry.szModule, L"samp.dll") == 0)
#else
				if (_stricmp(entry.szModule, "samp.dll") == 0)
#endif
				{
					m_dwSAMPBase = (DWORD) entry.modBaseAddr;
					break;
				}
			} 
			while (Module32Next(hSnapshot, &entry));

			CloseHandle(hSnapshot);
			return m_dwSAMPBase != 0;
		}

		template<typename ...T>
		bool call(DWORD dwObject, DWORD dwFunction, bool stackCleanup, T... args)
		{
			if (!openSAMP())
				return false;

			try
			{
				RemoteFunctionCaller<T...>(m_hHandle, dwObject, dwFunction, stackCleanup, args...);
				return true;
			}
			catch (...)
			{
				return false;
			}
		}

		template<typename T>
		T read(DWORD dwAddress, T onFail = T())
		{
			T t;
			if (ReadProcessMemory(m_hHandle, (LPCVOID) dwAddress, &t, sizeof(t), 0))
				return t;

			return onFail;
		}

	public:
		explicit SAMP()
		{
		}

		SAMP(const SAMP&) = delete;
		SAMP(SAMP &&) = delete;

		bool showGameText(const char *text, int time, int style)
		{
			if (text == 0)
				return false;

			if (strlen(text) == 0)
				return false;

			if (!openSAMP())
				return false;

			return call(NO_OBJECT, m_dwSAMPBase + Addresses::Functions::ShowGameText, false, style, time, text);
		}

		bool sendChat(const char *text)
		{
			if (text == 0)
				return false;

			if (strlen(text) == 0)
				return false;

			DWORD dwAddress = text[0] == '/' ? Addresses::Functions::SendCommand : Addresses::Functions::SendSay;

			if (!openSAMP())
				return false;

			return call(NO_OBJECT, m_dwSAMPBase + dwAddress, false, text);
		}

		bool addChatMessage(const char *text)
		{
			if (text == 0)
				return false;

			if (strlen(text) == 0)
				return false;

			if (!openSAMP())
				return false;

			DWORD dwObject = read(m_dwSAMPBase + Addresses::Objects::ChatInfo, 0);
			if (dwObject == 0)
				return false;

			return call(NO_OBJECT, m_dwSAMPBase + Addresses::Functions::AddChatMessage, true, text, dwObject);
		}

		bool showDialog(int style, const char *caption, const char *info, const char *button)
		{
			if (caption == 0 || info == 0 || button == 0)
				return false;

			if (!openSAMP())
				return false;

			DWORD dwObject = read(m_dwSAMPBase + Addresses::Objects::DialogInfo, 0);
			if (dwObject == 0)
				return false;

			return call(dwObject, m_dwSAMPBase + Addresses::Functions::ShowDialog, false, 0, m_dwSAMPBase + Addresses::Other::AdditionalDialogInfo, button, info, caption, style, 1);
		}

        bool isInForeground() const
        {
            return GetForegroundWindow() == FindWindowA(0, "GTA:SA:MP");
        }

        bool isInChat()
        {
            if (!openSAMP())
                return false;

            DWORD dwPtr = m_dwSAMPBase + Addresses::Objects::InChatInfo;
            DWORD dwAddress = read(dwPtr, 0) + Addresses::Other::InChatOffset;
            if(dwAddress == 0)
                return false;

            return read(dwAddress, 0) > 0;
        }

        float getPlayerHealth()
        {
            if(!openSAMP())
                return false;

            DWORD dwCPed = read(0xB6F5F0, 0);
            if(dwCPed == 0)
                return 0.0f;

            return read<float>(dwCPed + 0x540, 0.0f);
        }
	};
}
