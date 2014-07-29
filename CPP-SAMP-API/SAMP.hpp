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


namespace SAMP
{
	typedef unsigned char byte;
	typedef DWORD dword;
	typedef HANDLE handle;
	typedef unsigned int uint;

	// List of all needed X86 opcodes.
	namespace X86
	{
		const byte NOP = 0x90;
		const byte CALL = 0xE8;
		const byte MOV_ECX = 0xB9;
		const byte PUSH = 0x68;
		const byte RET = 0xC3;
		const byte RETN = 0xC4;
	}

	namespace Addresses
	{
		namespace Functions
		{
			const uint ShowGameText = 0x643B0;
			const uint SendSay = 0x4CA0;
			const uint SendCommand = 0x7BDD0;
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

		InjectData& operator << (dword d)
		{
			union {
				byte bytes[sizeof(dword)];
				dword d;
			} splitter;
			splitter.d = d;

			for (uint i = 0; i < 4; i++)
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

			for (uint i = 0; i < 4; i++)
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
		static const uint minAllocationSize = 1024;

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
		uint			m_argumentCount = 0;

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
		explicit FunctionCaller(handle hHandle, DWORD obj, DWORD func, ArgTypes... params) : m_hHandle(hHandle), m_callStack(hHandle)
		{
			if (obj)
				m_injectData << X86::MOV_ECX << obj;
			
			// Add arguments
			addArguments(params...);

			// Calculate the address (has to be relative!)
			uint stackOffset = m_injectData.raw().size();
			DWORD callOffset = func - (uint) m_callStack.address() - stackOffset - 5; // relative

			m_injectData << X86::CALL << (DWORD) callOffset << X86::RET;

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
				return false;

			if (dwPID == m_dwPID && m_hHandle != INVALID_HANDLE_VALUE)
				return true;

			m_dwPID = dwPID;
			m_hHandle = OpenProcess(STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xFFF, FALSE, m_dwPID);

			return m_hHandle != INVALID_HANDLE_VALUE;
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

		bool openAll()
		{
			return openProcess() && openSAMP();
		}

	public:
		explicit SAMP()
		{

		}

		bool showGameText(const char *text, uint time, uint size)
		{
			if (!openAll())
				return false;

			try
			{
				FunctionCaller<const char *, int, int>(m_hHandle, 0, m_dwSAMPBase + Addresses::Functions::ShowGameText, text, time, size);
				return true;
			}
			catch (...)
			{
				return false;
			}
		}

		bool sendChat(const char *text)
		{
			if (!openAll() || text == 0)
				return false;

			if (strlen(text) == 0)
				return 0;

			try
			{
				DWORD dwAddress = text[0] == '/' ? Addresses::Functions::SendCommand : Addresses::Functions::SendSay;

				FunctionCaller<const char *>(m_hHandle, 0, m_dwSAMPBase + dwAddress, text);
				return true;
			}
			catch (...)
			{
				return false;
			}
		}
	};
}
