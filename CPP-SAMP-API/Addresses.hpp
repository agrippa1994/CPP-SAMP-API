#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

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