//    Copyright 2019-2021 namazso <admin@namazso.eu>
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

#include "utl.h"

#include "stringencrypt.h"
#include "https.h"
#include "json.h"

#include <stdexcept>

utl::Version utl::GetLatestVersion()
{
  const auto user_agent = ESTR(L"OpenHashTab_Update_Checker")();
  const auto server_name = ESTR(L"api.github.com")();
  const auto method = ESTR(L"GET")();
  const auto uri = ESTR(L"/repos/namazso/OpenHashTab/tags")();
  const auto headers = ESTR(L"Content-Type: application/json\r\nAccept: application/vnd.github.v3+json\r\n")();

  HTTPRequest r{};
  r.user_agent = user_agent.data();
  r.server_name = server_name.data();
  r.method = method.data();
  r.uri = uri.data();
  r.headers = headers.data();

  const auto reply = DoHTTPS(r);

  if (reply.error_code)
    throw std::runtime_error(FormatString(
      ESTRt("Error %08X at %d: %s"),
      reply.error_code,
      reply.error_location,
      ErrorToString(reply.error_code).c_str()
    ));

  if (reply.http_code != 200)
    throw std::runtime_error(FormatString(
      ESTRt("HTTP Status %d received. Server says: %s"),
      reply.http_code,
      reply.body.c_str()
    ));


  json_parser parser{ reply.body.c_str() };
  const auto j_root = parser.root();

  if (!j_root)
    throw std::runtime_error(FormatString(
      ESTRt("JSON parse error. Body: %s"),
      reply.body.c_str()
    ));

  const json_t* j_child;
  const json_t* j_name;

  if (json_getType(j_root) != JSON_ARRAY
    || !((j_child = json_getChild(j_root)))
    || json_getType(j_child) != JSON_OBJ
    || !((j_name = json_getProperty(j_child, "name")))
    || json_getType(j_name) != JSON_TEXT)
    throw std::runtime_error(FormatString(
      ESTRt("Malformed reply. Body: %s"),
      reply.body.c_str()
    ));

  const auto ver = json_getValue(j_name);
  Version v{};
  if(3 != sscanf_s(ver, "v%hu.%hu.%hu", &v.major, &v.minor, &v.patch))
    throw std::runtime_error(FormatString(
      ESTRt("Malformed version number: %s"),
      ver
    ));

  return v;
}
