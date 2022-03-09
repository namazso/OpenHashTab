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
#include "OpenHashTab_i.h"

class COpenHashTabModule : public ATL::CAtlDllModuleT<COpenHashTabModule>
{
public:
  DECLARE_LIBID(LIBID_OpenHashTabLib)
  DECLARE_REGISTRY_APPID_RESOURCEID(IDR_OPENHASHTAB, "{715dae2b-0063-4d88-8f94-3dc072fc3fb0}")
};

extern class COpenHashTabModule _AtlModule;
