//    Copyright 2019-2024 namazso <admin@namazso.eu>
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
#include "Exporter.h"

#include "FileHashTask.h"
#include "Settings.h"
#include "utl.h"

static std::string TimeISO8601() {
  time_t now;
  time(&now);
  char buf[sizeof "2011-10-08T07:07:09Z"];
  tm _tm{};
  gmtime_s(&_tm, &now);
  strftime(buf, sizeof buf, "%FT%TZ", &_tm);
  return {buf};
}

class SumfileExporter : public Exporter {
  size_t idx;

public:
  constexpr SumfileExporter(size_t idx)
      : idx(idx) {}

  bool IsEnabled(Settings* settings) const override { return settings->algorithms[idx]; }

  std::string GetExportString(Settings* settings, bool for_clipboard, const std::list<FileHashTask*>& files) const override;

  [[nodiscard]] const char* GetName() const override { return LegacyHashAlgorithm::Algorithms()[idx].GetName(); }

  [[nodiscard]] const char* GetExtension() const override {
    const auto v = LegacyHashAlgorithm::Algorithms()[idx].GetExtensions()[0];
    return v ? v : "sums";
  }
};

class SFVExporter : public Exporter {
public:
  constexpr SFVExporter() = default;

  [[nodiscard]] const char* GetName() const override { return "SFV (CRC32)"; }

  bool IsEnabled(Settings* settings) const override { return settings->algorithms[LegacyHashAlgorithm::IdxByName("CRC32")]; }

  std::string GetExportString(Settings* settings, bool for_clipboard, const std::list<FileHashTask*>& files) const override;

  [[nodiscard]] const char* GetExtension() const override { return "sfv"; }
};

class DotHashExporter : public Exporter {
public:
  constexpr DotHashExporter() = default;

  [[nodiscard]] const char* GetName() const override { return ".hash (corz)"; }

  bool IsEnabled(Settings* settings) const override { return true; }

  std::string GetExportString(Settings* settings, bool for_clipboard, const std::list<FileHashTask*>& files) const override;

  [[nodiscard]] const char* GetExtension() const override { return "hash"; }
};

static bool SortByName(FileHashTask* a, FileHashTask* b) {
  return a->GetDisplayName() < b->GetDisplayName();
}

std::string SFVExporter::GetExportString(
  Settings* settings,
  bool for_clipboard,
  const std::list<FileHashTask*>& files
) const {
  std::stringstream ss;
  const auto line_end = for_clipboard || !settings->sumfile_unix_endings ? "\r\n" : "\n";
  if (!for_clipboard && settings->sumfile_banner) {
    ss << "; Generated by OpenHashTab " << CI_VERSION;
    if (settings->sumfile_banner_date)
      ss << " at " << TimeISO8601();
    ss << line_end;
    ss << "; https://github.com/namazso/OpenHashTab/" << line_end;
    ss << ";" << line_end;
  }

  const auto crc32 = LegacyHashAlgorithm::IdxByName("CRC32");

  std::vector<FileHashTask*> files_copy{files.cbegin(), files.cend()};
  std::sort(files_copy.begin(), files_copy.end(), SortByName);

  for (const auto file : files_copy) {
    if (file->GetError())
      continue;
    auto filename = utl::WideToUTF8(file->GetDisplayName().c_str());
    if (settings->sumfile_forward_slashes)
      std::replace(begin(filename), end(filename), '\\', '/');
    char hash[LegacyHashAlgorithm::k_max_size * 2 + 1];
    utl::HashBytesToString(hash, file->GetHashResult()[crc32], settings->sumfile_uppercase);
    ss << filename << " " << hash << line_end;
  }

  return ss.str();
}

static std::string GetExportStringSumfile(
  Settings* settings,
  bool for_clipboard,
  const std::list<FileHashTask*>& files,
  size_t algorithm,
  bool dot_hash
) {
  std::stringstream ss;
  // corz checksum .hash files use CRLF
  const auto line_end = for_clipboard || dot_hash || !settings->sumfile_unix_endings ? "\r\n" : "\n";
  const auto separator = (!dot_hash && settings->sumfile_use_double_space) ? "  " : " *";
  const auto uppercase = !dot_hash && settings->sumfile_uppercase;
  const auto forward_slashes = !dot_hash && settings->sumfile_forward_slashes;
  const auto dot_hash_compatible = dot_hash || settings->sumfile_dot_hash_compatible;

  if (!for_clipboard && settings->sumfile_banner) {
    ss << "# Generated by OpenHashTab " << CI_VERSION;
    if (settings->sumfile_banner_date)
      ss << " at " << TimeISO8601();
    ss << line_end;
    ss << "# https://github.com/namazso/OpenHashTab/" << line_end;
    ss << "#" << line_end;
  }

  std::string hash_name_dothash[LegacyHashAlgorithm::k_count];
  for (auto i = 0u; i < LegacyHashAlgorithm::k_count; ++i) {
    std::string name = LegacyHashAlgorithm::Algorithms()[i].GetName();
    std::transform(begin(name), end(name), begin(name), tolower);
    name.erase(std::remove(begin(name), end(name), '-'), end(name));
    hash_name_dothash[i] = std::move(name);
  }

  std::vector<FileHashTask*> files_copy{files.cbegin(), files.cend()};
  std::sort(files_copy.begin(), files_copy.end(), SortByName);

  for (const auto file : files_copy) {
    if (file->GetError())
      continue;

    auto filename_original = utl::WideToUTF8(file->GetDisplayName().c_str());
    auto filename = filename_original;
    if (forward_slashes)
      std::replace(begin(filename), end(filename), '\\', '/');
    const auto write_hash = [&](size_t idx) {
      char hash[LegacyHashAlgorithm::k_max_size * 2 + 1];
      utl::HashBytesToString(hash, file->GetHashResult()[idx], uppercase);
      if (dot_hash_compatible)
        ss << "#" << hash_name_dothash[idx] << "#" << filename_original << "#1970.01.01@00.00:00" << line_end; // ISO8601 or gtfo
      ss << hash << separator << filename << line_end;
    };
    if (!dot_hash)
      write_hash(algorithm);
    else
      for (auto i = 0u; i < LegacyHashAlgorithm::k_count; ++i)
        if (settings->algorithms[i])
          write_hash(i);
  }

  return ss.str();
}

std::string SumfileExporter::GetExportString(
  Settings* settings,
  bool for_clipboard,
  const std::list<FileHashTask*>& files
) const {
  return GetExportStringSumfile(settings, for_clipboard, files, idx, false);
}

std::string DotHashExporter::GetExportString(
  Settings* settings,
  bool for_clipboard,
  const std::list<FileHashTask*>& files
) const {
  return GetExportStringSumfile(settings, for_clipboard, files, -1, true);
}

template <typename T, size_t N, size_t... Rest>
struct Array_impl {
  static constexpr auto& value = Array_impl<T, N - 1, N, Rest...>::value;
};

template <typename T, size_t... Rest>
struct Array_impl<T, 0, Rest...> {
  static constexpr SumfileExporter value[] = {0, Rest...};
};

template <typename T, size_t N>
struct Array {
  static constexpr auto& value = Array_impl<T, N>::value;

  Array() = delete;
  Array(const Array&) = delete;
  Array(Array&&) = delete;
};

static constexpr auto s_dot_hash_exporter = DotHashExporter();
static constexpr auto s_sfv_exporter = SFVExporter();

constexpr std::array<const Exporter*, Exporter::k_count> Exporter::k_exporters = [] {
  static_assert(Exporter::k_count == LegacyHashAlgorithm::k_count + 2, "Wrong exporter count");
  std::array<const Exporter*, k_count> elems{};
  auto i = 0u;
  for (; i < LegacyHashAlgorithm::k_count; ++i)
    elems[i] = &Array<SumfileExporter, LegacyHashAlgorithm::k_count>::value[i];
  elems[LegacyHashAlgorithm::k_count] = &s_dot_hash_exporter;
  elems[LegacyHashAlgorithm::k_count + 1] = &s_sfv_exporter;
  return elems;
}();
