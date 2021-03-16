# OpenHashTab

[![License](https://img.shields.io/github/license/namazso/OpenHashTab)](COPYING) [![Weblate translation status](https://hosted.weblate.org/widgets/openhashtab/-/main/svg-badge.svg)](https://hosted.weblate.org/projects/openhashtab/main/) [![Downloads](https://img.shields.io/github/downloads/namazso/OpenHashTab/total)](https://github.com/namazso/OpenHashTab/releases/latest) [![GitHub Version](https://img.shields.io/github/v/release/namazso/OpenHashTab)](https://github.com/namazso/OpenHashTab/releases/latest) [![Chocolatey Version](https://img.shields.io/chocolatey/v/openhashtab)](https://chocolatey.org/packages/openhashtab/) ![Commits since release](https://img.shields.io/github/commits-since/namazso/OpenHashTab/latest/master)

## About

OpenHashTab is a shell extension for conveniently calculating and checking file hashes from file properties.

## Features

* Support for 22 algorithms, see **Algorithms**
* High performance hash calculation
* Native Windows looks
* High DPI screen support
* Long path support\*
* Multilingual (Consider contributing to translation!)
* Check hashes against VirusTotal with a button
* Hash checking against a checksum file (Supported: hex hash next to file, \*sum output (hex or base64), corz .hash, SFV)
* Hash export to file or clipboard (Supported: \*sum output, corz .hash, SFV)
* Optional context menu option for faster access
* File associations and standalone mode

\* to the extent Windows and configuration supports it. [Enable long paths](https://www.tenforums.com/tutorials/51704-enable-disable-win32-long-paths-windows-10-a.html) on 1607+ for better support.

## System requirements

* Windows Vista or later (x86 / x64 / ARM64)
* 1 GB RAM or more (for efficent hashing of more than 512 files at a time)

## Usage

Most of the actions should be obvious. Some not-so-obvious features are listed here:

* You can select multiple files or folders, all files will be hashed, directories traversed
* Double click hash to copy it
* Double click name or algorithm to copy the line in sumfile format
* Right click for popup menu: copy hash, copy filename, copy line, copy everything
* The counters next to the status text is in the format `(match/mismatch/nothing to check against/error)`
* Columns sort lexographically, except the hash column which sorts by match type
* Selecting the tab on a sumfile will interpret it as such and hash the files listed in it.
* If a hashed file has a sumfile with same filename plus one of the recognized sumfile extensions and the option for it is enabled, the file hash is checked against it.

## Algorithms

* CRC32
* xxHash (XXH32, XXH64)
* xxHash3 (64 and 128 bit variants)
* MD4, MD5
* RipeMD160
* Blake2sp
* SHA-1
* SHA-2 (SHA-224, SHA-256, SHA-384, SHA-512)
* SHA-3 (SHA3-224, SHA3-256, SHA3-384, SHA3-512)
* BLAKE3
* KangarooTwelve (264 bit)
* ParallelHash128 (264 bit) and ParallelHash256 (528 bit)
* Streebog (GOST R 34.11-12)

## Download

[Latest release](https://github.com/namazso/OpenHashTab/releases/latest)

[Development builds](https://github.com/namazso/OpenHashTab/actions?query=branch%3Amaster)

## Screenshot

![Screenshot](resources/screenshot.png) ![Algorithms](resources/algorithms.png)

## Donations

This software is provided completely free of charge to you, however I spent time and effort developing it. If you like this software, please consider making a donation:

* Bitcoin: 1N6UzYgzn3sLV33hB2iS3FvYLzD1G4CuS2
* Monero: 83sJ6GoeKf1U47vD9Tk6y2MEKJKxPJkECG3Ms7yzVGeiBYg2uYhBAUAZKNDH8VnAPGhwhZeqBnofDPgw9PiVtTgk95k53Rd

## Translation

![Weblate translation status](https://hosted.weblate.org/widgets/openhashtab/-/main/multi-auto.svg)

Translate the project at [Weblate](https://hosted.weblate.org/projects/openhashtab/main/)

### Translation contributors

In addition to contributors reported by git, some translations were also contributed by: **xprism**, **[@NieLnchn](https://github.com/NieLnchn)** (Simplified Chinese), **Niccolò Zanichelli** (Italian)

## Building

### Requirements

* MSYS2 with make
* Visual Studio 2019 16.8+ (with ARM64 and clang-cl)
* [InnoSetup](http://www.jrsoftware.org/isinfo.php)

### Compiling

1. Run build-xkcp.bat
2. Build AlgorithmsDll.sln for all Release targets
3. Build OpenHashTab.sln for all Release targets
4. Use Inno Setup Compiler to compile installer.iss

More options and commands can be found in the [GitHub Actions workflow](.github/workflows/ci.yml)

## Relationship to HashTab

HashTab is a similar purpose proprietary software. While this software has been inspired by it, I was never an user of HashTab and this software contains no code or resources related to it.

## License

All original code in this repo are licensed under the following license, unless explicitly stated otherwise in the file:

	Copyright 2019-2021 namazso <admin@namazso.eu>
	OpenHashTab - File hashing shell extension
	
	OpenHashTab is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
	
	OpenHashTab is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with OpenHashTab.  If not, see <https://www.gnu.org/licenses/>.

This software also contains or uses code from various other sources, for a complete list see [license.installer.txt](license.installer.txt)
