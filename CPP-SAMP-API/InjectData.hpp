#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vector>
#include <type_traits>

namespace SAMP
{
	typedef unsigned char byte;
	typedef unsigned short word;

	// InjectData is a vector of bytes, which can add dwords too.
	class InjectData
	{
		std::vector<byte> m_cData;

	public:

		template<typename T, typename = std::enable_if<std::is_arithmetic<T>::value>::type>
		InjectData& operator << (T t)
		{
			const size_t size = sizeof(T);

			union{
				byte bytes[size];
				T complete;
			} splitter;

			splitter.complete = t;
			for (size_t i = 0; i < size; i++)
				m_cData.push_back(splitter.bytes[i]);

			return *this;
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
}