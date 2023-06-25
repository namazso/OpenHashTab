//    Copyright 2019-2023 namazso <admin@namazso.eu>
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

struct HTTPRequest {
  const wchar_t* user_agent;
  const wchar_t* server_name;
  const wchar_t* method;
  const wchar_t* uri;
  const wchar_t* headers;
  const void* body;
  DWORD body_size;
};

struct HTTPResult {
  std::string body;
  DWORD error_code;

  union {
    DWORD http_code;
    DWORD error_location;
  };
};

HTTPResult DoHTTPS(const HTTPRequest& r);
