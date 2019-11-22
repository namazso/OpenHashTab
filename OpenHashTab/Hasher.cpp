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
#include "stdafx.h"

#include <mbedtls/md_internal.h>
#include "Hasher.h"
#include "utl.h"

// TODO: faster SHA3
// TODO: CRC (?)
// TODO: Let users disable algorithms. Although on modern processors the storage should be the main limiting factor.

extern "C" const mbedtls_md_info_t blake2sp_info;
extern "C" const mbedtls_md_info_t sha3_256_info;
extern "C" const mbedtls_md_info_t sha3_384_info;
extern "C" const mbedtls_md_info_t sha3_512_info;

// this is used for substitude when an algorithm is disabled
const static mbedtls_md_info_t null_info
{
  (mbedtls_md_type_t)0,
  "NULL",
  0,
  1,
  [](void*) { return 0; },
  [](void*, const unsigned char*, size_t) { return 0; },
  [](void*, unsigned char*) { return 0; },
  [](const unsigned char*, size_t, unsigned char*) { return 0; },
  []() { return (void*)1; },
  [](void* ctx) { assert(ctx == (void*)1); },
  [](void*, const void*) {},
  [](void*, const unsigned char*) { return 1; }
};

const mbedtls_md_info_t* k_hashers[k_hashers_count] =
{
  // Very slow and an FTP search didn't turn up anyone actually using it
  //&mbedtls_ripemd160_info, 

  &mbedtls_md5_info,
  &mbedtls_sha1_info,
  //&mbedtls_sha224_info,
  &mbedtls_sha256_info,
  //&mbedtls_sha384_info,
  &mbedtls_sha512_info,
  //&sha3_256_info,
  //&sha3_384_info,
  //&sha3_512_info,
  //&blake2sp_info
};

const PCTSTR k_hashers_name[k_hashers_count]
{
  _T("MD5"),
  _T("SHA1"),
  //_T("SHA224"),
  _T("SHA256"),
  //_T("SHA384"),
  _T("SHA512"),
  //_T("SHA3-256"),
  //_T("SHA3-384"),
  //_T("SHA3-512"),
  //_T("BLAKE2sp")
};

tstring hasher_get_extension_search_string(const mbedtls_md_info_t* info)
{
  auto ext = utl::UTF8ToTString(mbedtls_md_get_name(info));
  // this function is broken on emojis. Let's hope there won't be a hash algo with emoji name
  CharLower(ext.data());
  return ext;
}