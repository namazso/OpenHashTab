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
#include <Hasher.h>

class FileHashTask;
struct Settings;

class Exporter {
protected:
  ~Exporter() = default;

  constexpr Exporter() = default;

public:
  [[nodiscard]] virtual const char* GetName() const = 0;
  [[nodiscard]] virtual const char* GetExtension() const = 0;
  virtual bool IsEnabled(Settings* settings) const = 0;
  virtual std::string GetExportString(
    Settings* settings,
    bool for_clipboard,
    const std::list<FileHashTask*>& files
  ) const = 0;

  static constexpr auto k_count = LegacyHashAlgorithm::k_count + 2;
  static const std::array<const Exporter*, k_count> k_exporters;
};
