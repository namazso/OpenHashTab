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
#include "https.h"

HTTPResult DoHTTPS(const HTTPRequest& r) {
  HTTPResult result{};

  // Use WinHttpOpen to obtain a session handle.
  const auto session = WinHttpOpen(
    r.user_agent,
    WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
    WINHTTP_NO_PROXY_NAME,
    WINHTTP_NO_PROXY_BYPASS,
    0
  );

  if (session) {
    // Specify an HTTP server.
    const auto connection = WinHttpConnect(
      session,
      r.server_name,
      INTERNET_DEFAULT_HTTPS_PORT,
      0
    );

    if (connection) {
      // Create an HTTP Request handle.
      const auto request = WinHttpOpenRequest(
        connection,
        r.method,
        r.uri,
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE
      );

      if (request) {
        // Send a Request.
        auto ret = WinHttpSendRequest(
          request,
          r.headers,
          static_cast<DWORD>(-1L),
          const_cast<LPVOID>(r.body),
          r.body_size,
          r.body_size,
          0
        );

        if (ret) {
          ret = WinHttpReceiveResponse(
            request,
            nullptr
          );

          if (ret) {
            DWORD http_code = 0;
            DWORD size = sizeof(http_code);
            ret = WinHttpQueryHeaders(
              request,
              WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
              WINHTTP_HEADER_NAME_BY_INDEX,
              &http_code,
              &size,
              WINHTTP_NO_HEADER_INDEX
            );

            if (ret) {
              DWORD bytes_available;
              std::string out;
              do {
                bytes_available = 0;
                ret = WinHttpQueryDataAvailable(
                  request,
                  &bytes_available
                );
                if (!ret) {
                  result.error_location = 8;
                  break;
                }

                if (bytes_available == 0) {
                  result.error_code = 0;
                  result.http_code = http_code;
                  result.body = std::move(out);
                  goto success;
                }

                DWORD bytes_read = 0;
                const auto old_size = out.size();
                out.resize(old_size + bytes_available);
                ret = WinHttpReadData(
                  request,
                  (LPVOID)(out.data() + old_size),
                  bytes_available,
                  &bytes_read
                );
                if (!ret) {
                  result.error_location = 8;
                  break;
                }
                out.resize(old_size + bytes_read);
              } while (true);

              result.error_code = GetLastError();

success:;
            } else {
              result.error_code = GetLastError();
              result.error_location = 7;
            }
          } else {
            result.error_code = GetLastError();
            result.error_location = 6;
          }
        } else {
          result.error_code = GetLastError();
          result.error_location = 5;
        }
        WinHttpCloseHandle(request);
      } else {
        result.error_code = GetLastError();
        result.error_location = 4;
      }
      WinHttpCloseHandle(connection);
    } else {
      result.error_code = GetLastError();
      result.error_location = 3;
    }
    WinHttpCloseHandle(session);
  } else {
    result.error_code = GetLastError();
    result.error_location = 2;
  }

  return result;
}
