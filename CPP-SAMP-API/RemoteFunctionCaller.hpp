#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <memory>
#include <exception>

#include "RemoteMemory.hpp"
#include "InjectData.hpp"

#define NO_OBJECT 0

namespace SAMP
{
	// List of all needed X86 opcodes.
	namespace X86
	{
		const byte NOP = 0x90;
		const byte CALL = 0xE8;
		const byte MOV_ECX = 0x8B;
		const byte PUSH = 0x68;
		const byte RET = 0xC3;
	}

	// Class which can call a function in a remote process
	template<typename ...ArgTypes>
	class RemoteFunctionCaller
	{
		RemoteMemory m_callStack;
		InjectData m_injectData;
		const HANDLE m_hHandle;
		DWORD m_argumentCount = 0;

		std::vector< std::shared_ptr<RemoteMemory> > m_otherAllocations; // Memory-Allocations for strings

		template<typename T>
		typename std::enable_if< std::is_same<T, const char *>::value || std::is_same<T, char *>::value, T >::type addArgument(T t)
		{
			std::shared_ptr<RemoteMemory> memory(new RemoteMemory(m_hHandle));
			m_otherAllocations.push_back(memory);

			LPVOID ptr = memory->address();

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

		explicit RemoteFunctionCaller(HANDLE hHandle, DWORD obj, DWORD func, bool cleanUpStack, ArgTypes... params) : m_hHandle(hHandle), m_callStack(hHandle)
		{
			if (obj)
				m_injectData << X86::MOV_ECX << (DWORD) obj;

			// Add arguments
			addArguments(params...);

			// Calculate the address (has to be relative!)
			DWORD stackOffset = m_injectData.raw().size();
			DWORD callOffset = func - (DWORD) m_callStack.address() - stackOffset - 5; // relative

			m_injectData << X86::CALL << (DWORD) callOffset;

			// Clean up the stack if necessary
			if (cleanUpStack) // add esp, N
				m_injectData << (byte) 0x83 << (byte) 0xC4 << (byte) (m_argumentCount * 4) << X86::RET;
			else
				m_injectData << X86::RET;

			if(!WriteProcessMemory(m_hHandle, m_callStack, m_injectData.raw().data(), m_injectData.raw().size(), NULL))
				throw std::exception("Memory couldn't be written!");

			HANDLE hThread = CreateRemoteThread(m_hHandle, 0, 0, (LPTHREAD_START_ROUTINE) (LPVOID) m_callStack, 0, 0, 0);
			if (hThread == 0)
				throw std::exception("Remote-Thread couldn't be created!");

			WaitForSingleObject(hThread, INFINITE);
			CloseHandle(hThread);
		}
	};
}