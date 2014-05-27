#include <iostream>
#include <string>

#include "Process.h"

int main()
{
	CProcess process("cpp-samp-api");

	int x = 5;
	std::cout << process.writeMemory(&x, 10) << std::endl;

	std::cout << x << std::endl;

	std::cin.get();
}