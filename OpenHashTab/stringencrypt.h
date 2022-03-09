//    Copyright 2019-2022 namazso <admin@namazso.eu>
//    This file is part of OpenHashTab.
//
//    OpenHashTab is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    OpenHashTab is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with OpenHashTab.  If not, see <https://www.gnu.org/licenses/>.
#pragma once

#include <cstdint>
#include <array>

// This primitive string encryption can defeat shitty antiviruses that scream on sight of some strings / functions.
// Make sure to use it after user interaction, else it's pretty much worthless because behavioral analysis will see
// everything. This also means it's hardly useful for actual malware.

namespace detail
{
	constexpr uint32_t lcg(uint32_t seed, size_t round)
	{
		return round == 0 ? seed : static_cast<uint32_t>(1013904223ull + 1664525ull * lcg(seed, round - 1));
	}

	template <uint32_t Key, size_t N, typename Char>
	constexpr auto encrypt(const Char(&str)[N])
	{
		std::array<Char, N> arr{};
		for (auto i = 0u; i < N; ++i)
			arr[i] = static_cast<Char>(static_cast<uint32_t>(str[i]) ^ lcg(Key, i));
		return arr;
	}

	template <std::size_t N, typename Char>
	auto decrypt(const std::array<Char, N>& str, uint32_t Key)
	{
		std::array<Char, N> arr{};
		for (auto i = 0u; i < N; ++i)
			arr[i] = static_cast<Char>(static_cast<uint32_t>(str[i]) ^ lcg(Key, i));
		return arr;
	}
}


#define ESTR(str) []{\
	constexpr static auto enc = detail::encrypt<(uint32_t)L'a'>(str); \
	return detail::decrypt(enc, (uint32_t)(wchar_t)(uintptr_t)CharLowerW((LPWSTR)L'A'));\
}

#define ESTRt(str) ((ESTR(str)()).data())
