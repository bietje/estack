# E/STACK README

[![pipeline status](https://git.bietje.net/etaos/estack/badges/master/pipeline.svg)](https://git.bietje.net/etaos/estack/commits/master)

## About

E/STACK is a small, system independent network stack. The focus
is on TCP/IP traffic, but the user is by no means required to
use those protocols. The networking core is also completely independent
from any protocol, meaning that any implemented datalink layer protocols can
be utilised. Further more, the implementation makes have use of handlers which
allows applications to hook their own handlers, which might be useful for
proprietary protocols.

Some of E/STACKS features:

* Protocol independent networking core
* Ethernet
* IPv4 and IPv6
* TCP + UDP
* DNS
* DHCP + DHCPv6
* ICMP + ICMPv6

## License

E/STACK is licensed under the GNU Lesser General Public License, version 3. The COPYING
contains a full copy of said license, but here is the general stretch of it:

    E/STACK
    Copyright (C) 2017  E/STACK developers <etaos@googlegroups.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

See the GNU [website](gnu.org) to see which licenses are compatible.

## Compiling E/STACK

CMake oversees the compilation process of E/STACK. You need to following libraries / programs
to build a development build of E/STACK:

* gcc
* g++
* cmake
* libpcap-dev
* make
* host system runtime library

Download a copy of E/STACK and open a terminal window into that directory. After doing so, compile
E/STACK

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CI=False
make
```

## Contributing

Contributing to E/STACK is verry much appriciated, however there are some rules. Please read
CONTRIBUTING.md before writing a patch.
