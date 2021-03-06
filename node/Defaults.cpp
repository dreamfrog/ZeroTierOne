/*
 * ZeroTier One - Global Peer to Peer Ethernet
 * Copyright (C) 2012-2013  ZeroTier Networks LLC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * --
 *
 * ZeroTier may be used and distributed under the terms of the GPLv3, which
 * are available at: http://www.gnu.org/licenses/gpl-3.0.html
 *
 * If you would like to embed ZeroTier into a commercial application or
 * redistribute it in a modified binary form, please contact ZeroTier Networks
 * LLC. Start here: http://www.zerotier.com/
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Constants.hpp"
#include "Defaults.hpp"
#include "Utils.hpp"

#ifdef __WINDOWS__
#include <WinSock2.h>
#include <Windows.h>
#endif

namespace ZeroTier {

const Defaults ZT_DEFAULTS;

static inline std::map< Identity,std::vector<InetAddress> > _mkSupernodeMap()
{
	std::map< Identity,std::vector<InetAddress> > sn;
	Identity id;
	std::vector<InetAddress> addrs;

	// Nothing special about a supernode... except that they are
	// designated as such and trusted to provide WHOIS lookup.

	// cthulhu.zerotier.com - New York, New York, USA
	addrs.clear();
	if (!id.fromString("8acf059fe3:0:482f6ee5dfe902319b419de5bdc765209c0ecda38c4d6e4fcf0d33658398b4527dcd22f93112fb9befd02fd78bf7261b333fc105d192a623ca9e50fc60b374a5"))
		throw std::runtime_error("invalid identity in Defaults");
	addrs.push_back(InetAddress("162.243.77.111",ZT_DEFAULT_UDP_PORT));
	sn[id] = addrs;

	// nyarlathotep.zerotier.com - San Francisco, California, USA
	addrs.clear();
	if (!id.fromString("7e19876aba:0:2a6e2b2318930f60eb097f70d0f4b028b2cd6d3d0c63c014b9039ff35390e41181f216fb2e6fa8d95c1ee9667156411905c3dccfea78d8c6dfafba688170b3fa"))
		throw std::runtime_error("invalid identity in Defaults");
	addrs.push_back(InetAddress("198.199.97.220",ZT_DEFAULT_UDP_PORT));
	sn[id] = addrs;

	// shub-niggurath.zerotier.com - Amsterdam, Netherlands
	addrs.clear();
	if (!id.fromString("36f63d6574:0:67a776487a1a99b32f413329f2b67c43fbf6152e42c6b66e89043e69d93e48314c7d709b58a83016bd2612dd89400b856e18c553da94892f7d3ca16bf2c92c24"))
		throw std::runtime_error("invalid identity in Defaults");
	addrs.push_back(InetAddress("198.211.127.172",ZT_DEFAULT_UDP_PORT));
	sn[id] = addrs;

	return sn;
}

static inline std::string _mkDefaultHomePath()
{
#ifdef __UNIX_LIKE__
#ifdef __APPLE__
	return std::string("/Library/Application Support/ZeroTier/One");
#else
	return std::string("/var/lib/zerotier-one");
#endif
#else
#ifdef __WINDOWS__
	OSVERSIONINFO vi;
	memset (&vi,0,sizeof(vi));
	vi.dwOSVersionInfoSize = sizeof(vi);
	GetVersionEx(&vi);
	if (vi.dwMajorVersion < 6)
		return std::string("C:\\Documents and Settings\\All Users\\Application Data\\ZeroTier\\One");
	return std::string("C:\\ProgramData\\ZeroTier\\One");
#else
	// unknown platform
#endif
#endif
}

Defaults::Defaults() :
#ifdef ZT_TRACE_MULTICAST
	multicastTraceWatcher(ZT_TRACE_MULTICAST),
#endif
	defaultHomePath(_mkDefaultHomePath()),
	supernodes(_mkSupernodeMap())
{
}

} // namespace ZeroTier
