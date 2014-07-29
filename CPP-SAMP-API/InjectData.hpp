#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vector>

namespace SAMP
{
	typedef unsigned char byte;

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

		InjectData& operator << (DWORD d)
		{
			union {
				byte bytes[sizeof(DWORD)];
				DWORD d;
			} splitter;
			splitter.d = d;

			for (DWORD i = 0; i < 4; i++)
				m_cData.push_back(splitter.bytes[i]);

			return *this;
		}

		InjectData& operator << (int d)
		{
			union {
				byte bytes[sizeof(int)];
				int d;
			} splitter;
			splitter.d = d;

			for (DWORD i = 0; i < 4; i++)
				m_cData.push_back(splitter.bytes[i]);

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

		std::vector<byte>& raw()
		{
			return m_cData;
		}
	};
}