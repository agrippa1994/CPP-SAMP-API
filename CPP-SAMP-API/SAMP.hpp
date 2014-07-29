#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
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

		InjectData& operator << (int d)
		{
			byte bytes[4] = { d >> 24 & 0xFF, d >> 16 & 0xFF, d >> 8 & 0xFF, d & 0xFF };

			for (uint i = 0; i < 4; i++)
				m_cData.push_back(bytes[i]);

			return *this;
		}

		InjectData& operator << (dword d)
		{
			byte bytes[4] = { d >> 24 & 0xFF, d >> 16 & 0xFF, d >> 8 & 0xFF, d & 0xFF };

			for (uint i = 0; i < 4; i++)
				m_cData.push_back(bytes[i]);

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
	};

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

	template<typename ...ArgTypes>
	class FunctionCaller
	{
		RemoteMemory m_callStack;
		InjectData m_injectData;
		const HANDLE m_hHandle;

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
			m_injectData << X86::PUSH << DWORD(t);
		}

		void addArguments()
		{
		}

		template<typename T, typename ...U>
		void addArguments(T t, U... p)
		{
			addArgument(t);
			addArguments(p...);
		}
	public:
		explicit FunctionCaller(handle hHandle, DWORD obj, DWORD func, ArgTypes... params) : m_hHandle(hHandle), m_callStack(hHandle)
		{
			if (obj)
				m_injectData << X86::MOV_ECX << obj;
			
			addArguments(params...);

			std::vector<byte>& bytes = m_injectData;
			int callOffset = func - ((uint) (LPVOID) m_callStack + bytes.size() + 5); // relative

			m_injectData += X86::CALL;
			m_injectData += int(callOffset);
			m_injectData += X86::RET;

			BOOL bRet = WriteProcessMemory(m_hHandle, m_callStack, bytes.data(), bytes.size(), 0);
			if (bRet == 0)
				throw std::exception("Memory couldn't be written!");

// 			auto hThread = CreateRemoteThread(m_hHandle, 0, 0, (LPTHREAD_START_ROUTINE) (LPVOID) m_callStack, 0, 0, 0);
// 			if (hThread == 0)
// 				throw std::exception("Remote-Thread couldn't be created!");
// 
// 			WaitForSingleObject(hThread, INFINITE);
// 			CloseHandle(hThread);
		}
	};

	class SAMP
	{
		DWORD	m_dwPID = 0;
		HANDLE	m_hHandle = INVALID_HANDLE_VALUE;
		LPVOID  m_pRemoteMemory = 0;

		bool openProcess()
		{

		}

		template<typename T>
		bool callRemoteFunction();
	public:
		explicit SAMP()
		{

		}
	};
}
