#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <TlHelp32.h>

#include <exception>
#include <string>
#include <iostream>
#include <vector>
#include <type_traits>
#include <memory>

#define NO_OBJECT 0

namespace SAMP
{
	typedef unsigned char byte;

	// List of all needed X86 opcodes.
	namespace X86
	{
		const byte NOP = 0x90;
		const byte CALL = 0xE8;
		const byte MOV_ECX = 0x8B;
		const byte PUSH = 0x68;
		const byte PUSH_ECX = 0x51;
		const byte RET = 0xC3;
		const byte RETN = 0xC4;
	}

	namespace Addresses
	{
		namespace Objects
		{
			const DWORD ChatInfo = 0x212A6C;
		}

		namespace Functions
		{
			const DWORD AddChatMessage = 0x7AA00;

			const DWORD ShowGameText = 0x643B0;

			const DWORD SendSay = 0x4CA0;
			const DWORD SendCommand = 0x7BDD0;
		}
	}

	// InjectData is a vector of bytes, which can add dwords too.
	class InjectData
	{
		std::vector<byte> m_cData;

	public:
		InjectData& operator << (byte d)
		{
			m_cData.push_back(d);
			return *this;
		}

		InjectData& operator << (DWORD d)
		{
			union {
				byte bytes[sizeof(DWORD)];
				DWORD d;
			} splitter;
			splitter.d = d;

			for (DWORD i = 0; i < 4; i++)
				m_cData.push_back(splitter.bytes[i]);

			return *this;
		}

		InjectData& operator << (int d)
		{
			union {
				byte bytes[sizeof(int)];
				int d;
			} splitter;
			splitter.d = d;

			for (DWORD i = 0; i < 4; i++)
				m_cData.push_back(splitter.bytes[i]);

			return *this;
		}

		template<typename T>
		InjectData& operator+=(T t)
		{
			return *this << (t);
		}

		operator std::vector<byte>&()
		{
			return m_cData;
		}

		std::vector<byte>& raw()
		{
			return m_cData;
		}
	};

	// Class which holds memory and uses RAII for deinitialization
	class RemoteMemory
	{
		HANDLE m_hProc;
		LPVOID m_pMemory = 0;

	public:
		static const DWORD minAllocationSize = 1024;

		explicit RemoteMemory(HANDLE hProc) : m_hProc(hProc)
		{
			byte nulls[minAllocationSize] = { 0 };

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

	// Class which can call a function in a remote process
	template<typename ...ArgTypes>
	class FunctionCaller
	{
		RemoteMemory	m_callStack;
		InjectData		m_injectData;
		const HANDLE	m_hHandle;
		DWORD			m_argumentCount = 0;

		std::vector< std::shared_ptr<RemoteMemory> > m_otherAllocations; // Memory-Allocations for strings

		template<typename T>
		typename std::enable_if< std::is_same<T, std::string>::value, T >::type addArgument(T t)
		{

			m_otherAllocations.push_back(std::shared_ptr<RemoteMemory>(new RemoteMemory(m_hHandle)));
			LPVOID ptr = m_otherAllocations.back()->address();
			
			BOOL bRet = WriteProcessMemory(m_hHandle, ptr, t.c_str(), t.len(), 0);
			if (bRet == 0)
				throw std::exception("Memory couldn't be written!");

			m_injectData << X86::PUSH << DWORD(ptr);
		}

		template<typename T>
		typename std::enable_if< std::is_same<T, const char *>::value, T >::type addArgument(T t)
		{
			m_otherAllocations.push_back(std::shared_ptr<RemoteMemory>(new RemoteMemory(m_hHandle)));
			LPVOID ptr = m_otherAllocations.back()->address();

			BOOL bRet = WriteProcessMemory(m_hHandle, ptr, t, strlen(t), 0);
			if (bRet == 0)
				throw std::exception("Memory couldn't be written!");

			m_injectData << X86::PUSH << DWORD(ptr);
		}

		template<typename T>
		typename std::enable_if< std::is_arithmetic<T>::value>::type addArgument(T t)
		{
			m_injectData << X86::PUSH << T(t);
		}

		void addArguments()
		{
		}

		template<typename T, typename ...U>
		void addArguments(T t, U... p)
		{
			m_argumentCount++;

			addArgument(t);
			addArguments(p...);
		}
	public:
		explicit FunctionCaller(HANDLE hHandle, DWORD obj, DWORD func, bool cleanUpStack, bool shouldPushObject, ArgTypes... params) : m_hHandle(hHandle), m_callStack(hHandle)
		{
			if (obj && !shouldPushObject)
				m_injectData << X86::MOV_ECX << (DWORD) obj;
			
			// Add arguments
			addArguments(params...);

			if (shouldPushObject && obj)
				addArguments(obj);
				
			// Calculate the address (has to be relative!)
			DWORD stackOffset = m_injectData.raw().size();
			DWORD callOffset = func - (DWORD) m_callStack.address() - stackOffset - 5; // relative

			m_injectData << X86::CALL << (DWORD) callOffset;
			if (cleanUpStack)
			{
				// add esp, N
				m_injectData << (byte) 0x83 << (byte) 0xC4 << (byte) (m_argumentCount * 4) << X86::RET;
			}
			else
			{
				m_injectData << X86::RET;
			}

			DWORD dwWritten = 0;
			BOOL bRet = WriteProcessMemory(m_hHandle, m_callStack, m_injectData.raw().data(), m_injectData.raw().size(), &dwWritten);
			if (bRet == 0 || dwWritten != m_injectData.raw().size())
				throw std::exception("Memory couldn't be written!");

			auto hThread = CreateRemoteThread(m_hHandle, 0, 0, (LPTHREAD_START_ROUTINE) (LPVOID) m_callStack, 0, 0, 0);
			if (hThread == 0)
				throw std::exception("Remote-Thread couldn't be created!");

			WaitForSingleObject(hThread, INFINITE);
			CloseHandle(hThread);
		}
	};

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
				
			if (dwPID == m_dwPID && m_hHandle != INVALID_HANDLE_VALUE && m_dwSAMPBase)
				return true;

			m_dwPID = dwPID;
			m_hHandle = OpenProcess(STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xFFF, FALSE, m_dwPID);
			if (m_hHandle == INVALID_HANDLE_VALUE)
				return false;

			if (!openSAMP())
			{
				CloseHandle(m_hHandle);
				m_hHandle = INVALID_HANDLE_VALUE;

				return false;
			}

			return m_hHandle != INVALID_HANDLE_VALUE && m_dwSAMPBase;
		}

		bool openSAMP()
		{
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
			if (!openProcess())
				return false;

			try
			{
				FunctionCaller<T...>(m_hHandle, dwObject, dwFunction, stackCleanup, shouldPushObject, args...);
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

			if (!openProcess())
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

			if (!openProcess())
				return false;

			return call(NO_OBJECT, m_dwSAMPBase + dwAddress, false, false, text);
		}

		bool addChatMessage(const char *text)
		{
			if (text == 0)
				return false;

			if (strlen(text) == 0)
				return false;

			if (!openProcess())
				return false;

			DWORD dwObject = read(m_dwSAMPBase + Addresses::Objects::ChatInfo, 0);
			if (dwObject == 0)
				return false;

			return call(dwObject, m_dwSAMPBase + Addresses::Functions::AddChatMessage, true, true, text);
		}
	};
}
