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
#pragma once

#include "../Algorithms/Hasher.h"

class Settings
{
  bool _enabled[HashAlgorithm::k_count]{};

  Settings()
  {
    _enabled[HashAlgorithm::IdxByName("MD5")] = true;
    _enabled[HashAlgorithm::IdxByName("SHA-1")] = true;
    _enabled[HashAlgorithm::IdxByName("SHA-256")] = true;
    _enabled[HashAlgorithm::IdxByName("SHA-512")] = true;
    for (const auto& algo : HashAlgorithm::g_hashers)
      LoadHashEnabled(&algo);
  }

  void LoadHashEnabled(const HashAlgorithm* algorithm);
  void StoreHashEnabled(const HashAlgorithm* algorithm) const;

public:
  bool IsHashEnabled(int idx) const { return _enabled[idx]; }
  bool IsHashEnabled(const HashAlgorithm* algorithm) const { return IsHashEnabled(algorithm->Idx()); }
  void SetHashEnabled(int idx, bool enabled) { SetHashEnabled(&HashAlgorithm::g_hashers[idx], enabled); }
  void SetHashEnabled(const HashAlgorithm* algorithm, bool enabled)
  {
    _enabled[algorithm->Idx()] = enabled;
    StoreHashEnabled(algorithm);
  }

  static Settings instance;
};