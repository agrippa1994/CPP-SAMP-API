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
		const byte MOV_ECX = 0xB9;
		const byte PUSH = 0x68;
		const byte RET = 0xC3;
		const word ADD_ESP = 0xC483;
	}

	// Class which can call a function in a remote process
	template<typename ...ArgTypes>
	class RemoteFunctionCaller
	{
		RemoteMemory m_remoteMemory;
		InjectData m_injectData;
		const HANDLE m_hHandle;
		DWORD m_argumentCount = 0;

		std::vector< std::shared_ptr<RemoteMemory> > m_otherAllocations; // Memory-Allocations for strings

		template<typename T>
		typename std::enable_if< std::is_same<T, const char *>::value, T >::type addArgument(T t)
		{
			std::shared_ptr<RemoteMemory> memory(new RemoteMemory(m_hHandle));
			m_otherAllocations.push_back(memory);

			if(!memory->writeArray(t, strlen(t)))
				throw std::exception("Memory couldn't be written!");

			m_injectData << X86::PUSH << (DWORD) memory->address();
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

		explicit RemoteFunctionCaller(HANDLE hHandle, DWORD obj, DWORD func, bool cleanUpStack, ArgTypes... params) : m_hHandle(hHandle), m_remoteMemory(hHandle)
		{
			if (obj)
				m_injectData << X86::MOV_ECX << (DWORD) obj;

			// Add arguments
			addArguments(params...);

			// Calculate the address (has to be relative!)
			DWORD stackOffset = m_injectData.raw().size();
			DWORD callOffset = func - (DWORD) m_remoteMemory.address() - stackOffset - 5; // relative

			m_injectData << X86::CALL << (DWORD) callOffset;

			// Clean up the stack if necessary
			if (cleanUpStack) // add esp, N
				m_injectData << X86::ADD_ESP << (byte)(m_argumentCount * 4) << X86::RET;
			else
				m_injectData << X86::RET;

			// Load X86-Code into the process
			if (!(m_remoteMemory = m_injectData))
				throw std::exception("Memory couldn't be written!");

			// Execute loaded X86-Code
			if (!m_remoteMemory())
				throw std::exception("Thread couldn't be executed!");
		}
	};
}