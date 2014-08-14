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

namespace Addresses
{
	namespace Objects
	{
		const DWORD ChatInfo = 0x212A6C;
		const DWORD DialogInfo = 0x212A40;

        const DWORD InChatInfo = 0x212A94;
	}

	namespace Functions
	{
		const DWORD AddChatMessage = 0x7AA00;

		const DWORD ShowGameText = 0x643B0;

		const DWORD SendSay = 0x4CA0;
		const DWORD SendCommand = 0x7BDD0;

		const DWORD ShowDialog = 0x816F0;
	}

	namespace Other
	{
		const DWORD AdditionalDialogInfo = 0xCE8F1;

        const DWORD InChatOffset = 0x55;
	}
}
