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
		DWORD	m_dwPID = 0;
		DWORD	m_dwSAMPBase = 0;
		HANDLE	m_hHandle = INVALID_HANDLE_VALUE;

		bool openProcess()
		{
			DWORD dwPID = 0;
			GetWindowThreadProcessId(FindWindow(0, "GTA:SA:MP"), &dwPID);

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
				if (_stricmp(entry.szModule, "samp.dll") == 0)
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
		bool call(DWORD dwObject, DWORD dwFunction, bool stackCleanup, bool shouldPushObject, T... args)
		{
			if (!openSAMP())
				return false;

			try
			{
				RemoteFunctionCaller<T...>(m_hHandle, dwObject, dwFunction, stackCleanup, shouldPushObject, args...);
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

		bool showGameText(const char *text, int time, int style)
		{
			if (text == 0)
				return false;

			if (strlen(text) == 0)
				return false;

			if (!openSAMP())
				return false;

			return call(NO_OBJECT, m_dwSAMPBase + Addresses::Functions::ShowGameText, false, false, style, time, text);
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

			return call(NO_OBJECT, m_dwSAMPBase + dwAddress, false, false, text);
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

			return call(dwObject, m_dwSAMPBase + Addresses::Functions::AddChatMessage, true, true, text);
		}
	};
}
