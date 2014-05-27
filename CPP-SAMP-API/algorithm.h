#pragma once
#include <algorithm>
#include <string>

// String functions
bool ifind(std::string str1, std::string str2)
{
	std::transform(str1.begin(), str1.end(), str1.begin(), tolower);
	std::transform(str2.begin(), str2.end(), str2.begin(), tolower);

	return str1.find(str2) != std::string::npos;
}