//    Copyright 2019-2020 namazso <admin@namazso.eu>
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

#include "https.h"

#include <winhttp.h>

#include "stringencrypt.h"

struct WinHTTPFunctions
{
  HMODULE h;

#define FN(name) decltype(&::name) name = (decltype(&::name))GetProcAddress(h, ESTRt(#name))

  FN(WinHttpOpen);
  FN(WinHttpConnect);
  FN(WinHttpOpenRequest);
  FN(WinHttpSendRequest);
  FN(WinHttpReceiveResponse);
  FN(WinHttpQueryHeaders);
  FN(WinHttpQueryDataAvailable);
  FN(WinHttpReadData);
  FN(WinHttpCloseHandle);

#undef FN

  WinHTTPFunctions(HMODULE h) : h(h) {}
};

HTTPResult DoHTTPS(const HTTPRequest& r)
{
  HTTPResult result{};

  const auto h = LoadLibraryExW(ESTRt(L"winhttp"), nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
  if (h)
  {
    const auto fn = WinHTTPFunctions{ h };

    // Use WinHttpOpen to obtain a session handle.
    const auto session = fn.WinHttpOpen(
      r.user_agent,
      WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
      WINHTTP_NO_PROXY_NAME,
      WINHTTP_NO_PROXY_BYPASS,
      0
    );

    if (session)
    {
      // Specify an HTTP server.
      const auto connection = fn.WinHttpConnect(
        session,
        r.server_name,
        INTERNET_DEFAULT_HTTPS_PORT,
        0
      );

      if (connection)
      {
        // Create an HTTP Request handle.
        const auto request = fn.WinHttpOpenRequest(
          connection,
          r.method,
          r.uri,
          nullptr,
          WINHTTP_NO_REFERER,
          WINHTTP_DEFAULT_ACCEPT_TYPES,
          WINHTTP_FLAG_SECURE
        );

        if (request)
        {
          // Send a Request.
          auto ret = fn.WinHttpSendRequest(
            request,
            r.headers,
            static_cast<DWORD>(-1L),
            const_cast<LPVOID>(r.body),
            r.body_size,
            r.body_size,
            0
          );

          if (ret)
          {
            ret = fn.WinHttpReceiveResponse(
              request,
              nullptr
            );

            if (ret)
            {
              DWORD http_code = 0;
              DWORD size = sizeof(http_code);
              ret = fn.WinHttpQueryHeaders(
                request,
                WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                WINHTTP_HEADER_NAME_BY_INDEX,
                &http_code,
                &size,
                WINHTTP_NO_HEADER_INDEX
              );

              if (ret)
              {
                DWORD bytes_available = 0;
                ret = fn.WinHttpQueryDataAvailable(
                  request,
                  &bytes_available
                );

                if (ret)
                {
                  std::string out;
                  DWORD bytes_read = 0;
                  out.resize(bytes_available);
                  ret = fn.WinHttpReadData(
                    request,
                    (LPVOID)out.data(),
                    static_cast<DWORD>(out.size()),
                    &bytes_read
                  );

                  if (ret)
                  {
                    out.resize(bytes_read);
                    result.error_code = 0;
                    result.http_code = http_code;
                    result.body = std::move(out);
                  }
                  else
                  {
                    result.error_code = GetLastError();
                    result.error_location = 9;
                  }

                }
                else
                {
                  result.error_code = GetLastError();
                  result.error_location = 8;
                }

              }
              else
              {
                result.error_code = GetLastError();
                result.error_location = 7;
              }

            }
            else
            {
              result.error_code = GetLastError();
              result.error_location = 6;
            }

          }
          else
          {
            result.error_code = GetLastError();
            result.error_location = 5;
          }

          fn.WinHttpCloseHandle(request);
        }
        else
        {
          result.error_code = GetLastError();
          result.error_location = 4;
        }

        fn.WinHttpCloseHandle(connection);
      }
      else
      {
        result.error_code = GetLastError();
        result.error_location = 3;
      }

      fn.WinHttpCloseHandle(session);
    }
    else
    {
      result.error_code = GetLastError();
      result.error_location = 2;
    }

    FreeLibrary(h);
  }
  else
  {
    result.error_code = GetLastError();
    result.error_location = 1;
  }

  return result;
}
