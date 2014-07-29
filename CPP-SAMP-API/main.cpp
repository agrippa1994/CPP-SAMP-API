#include <iostream>
#include <string>

#include "SAMP.hpp"


void __stdcall test(const char *x)
{
	std::cout << "Hallo " << x << std::endl;
}

int main()
{
	using namespace SAMP;
	DWORD dwPID = 0;
	GetWindowThreadProcessId(FindWindow(0, "Rechner"), &dwPID);

	std::cout << dwPID << std::endl;

	HANDLE hHandle = OpenProcess(STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xFFF, FALSE, dwPID);
	std::cout << (DWORD) hHandle << std::endl;

	{
		try
		{
			FunctionCaller<const char *> caller(GetCurrentProcess(), 0, (DWORD) test, "Hallo");
		}
		catch (const std::exception &e)
		{
			std::cout << e.what() << std::endl;
			std::cout << "Error: " << GetLastError() << std::endl;
		}
	}
	
	std::cin.get();
}