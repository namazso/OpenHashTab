//    Copyright 2019 namazso <admin@namazso.eu>
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

constexpr static auto k_hashers_count = 4;

// As of now, 512 bits is the longest hash we handle
constexpr static size_t k_max_hash_size = MBEDTLS_MD_MAX_SIZE;

extern const mbedtls_md_info_t* k_hashers[k_hashers_count];
extern const PCTSTR k_hashers_name[k_hashers_count];

tstring hasher_get_extension_search_string(const mbedtls_md_info_t* info);