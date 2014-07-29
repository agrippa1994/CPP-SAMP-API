#include <iostream>
#include <string>

#include "SAMP.hpp"

int main()
{
	SAMP::SAMP samp;

	for (;; Sleep(100))
	if (GetAsyncKeyState(VK_RETURN))
	{
		std::cout << samp.sendChat("hallo welt") << std::endl;
		std::cout << samp.showGameText("hallo", 5000, 1) << std::endl;
	}
			

	std::cin.get();
}