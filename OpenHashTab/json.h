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

#include <list>
#include <string>

#include <tiny-json.h>

class json_parser : jsonPool_t
{
  static json_t* alloc_fn(jsonPool_t* pool)
  {
    const auto list_pool = static_cast<json_parser*>(pool);
    return &list_pool->_list.emplace_back();
  }

  std::list<json_t> _list{};
  std::string _str{};
  json_t const* _root{};

public:
  json_parser() : jsonPool_t{ &alloc_fn, &alloc_fn } {}
  json_parser(const char* str)
    : jsonPool_t{ &alloc_fn, &alloc_fn }
    , _str{ str }
  {
    _root = json_createWithPool(_str.data(), this);
  }
  json_parser(const json_parser&) = delete;
  json_parser(json_parser&&) = delete;
  json_parser& operator=(const json_parser&) = delete;
  json_parser& operator=(json_parser&&) = delete;

  void parse(const char* str)
  {
    _str = str;
    _list.clear();
    _root = json_createWithPool(_str.data(), this);
  }

  json_t const* root() const { return _root; }
};