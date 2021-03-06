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

#ifndef _ZT_UTILS_HPP
#define _ZT_UTILS_HPP

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include <string>
#include <stdexcept>
#include <vector>
#include <map>

#include "Constants.hpp"

#include "../ext/lz4/lz4.h"
#include "../ext/lz4/lz4hc.h"

#ifdef __WINDOWS__
#include <WinSock2.h>
#include <Windows.h>
#else
#include <unistd.h>
#include <sys/time.h>
#include <arpa/inet.h>
#endif

/**
 * Maximum compression/decompression block size (do not change)
 */
#define ZT_COMPRESSION_BLOCK_SIZE 16777216

namespace ZeroTier {

/**
 * Miscellaneous utility functions and global constants
 */
class Utils
{
public:
	/**
	 * Perform a time-invariant binary comparison
	 *
	 * @param a First binary string
	 * @param b Second binary string
	 * @param len Length of strings
	 * @return True if strings are equal
	 */
	static inline bool secureEq(const void *a,const void *b,unsigned int len)
		throw()
	{
		const char *p1 = (const char *)a;
		const char *p2 = (const char *)b;
		uint64_t diff = 0;

		while (len >= 8) {
			diff |= (*((const uint64_t *)p1) ^ *((const uint64_t *)p2));
			p1 += 8;
			p2 += 8;
			len -= 8;
		}
		while (len--)
			diff |= (uint64_t)(*p1++ ^ *p2++);

		return (diff == 0ULL);
	}

	/**
	 * Delete a file
	 *
	 * @param path Path to delete
	 * @return True if delete was successful
	 */
	static inline bool rm(const char *path)
		throw()
	{
#ifdef __WINDOWS__
		return (DeleteFileA(path) != FALSE);
#else
		return (unlink(path) == 0);
#endif
	}
	static inline bool rm(const std::string &path)
		throw()
	{
		return rm(path.c_str());
	}

	/**
	 * List a directory's contents
	 *
	 * @param path Path to list
	 * @param files Set to fill with files
	 * @param directories Set to fill with directories
	 * @return Map of entries and whether or not they are also directories (empty on failure)
	 */
	static std::map<std::string,bool> listDirectory(const char *path);

	/**
	 * @param data Data to convert to hex
	 * @param len Length of data
	 * @return Hexadecimal string
	 */
	static std::string hex(const void *data,unsigned int len);
	static inline std::string hex(const std::string &data) { return hex(data.data(),(unsigned int)data.length()); }

	/**
	 * @param hex Hexadecimal ASCII code (non-hex chars are ignored)
	 * @return Binary data
	 */
	static std::string unhex(const char *hex);
	static inline std::string unhex(const std::string &hex) { return unhex(hex.c_str()); }

	/**
	 * @param hex Hexadecimal ASCII
	 * @param buf Buffer to fill
	 * @param len Length of buffer
	 * @return Number of characters actually written
	 */
	static unsigned int unhex(const char *hex,void *buf,unsigned int len);
	static inline unsigned int unhex(const std::string &hex,void *buf,unsigned int len) { return unhex(hex.c_str(),buf,len); }

	/**
	 * @param hex Hexadecimal ASCII
	 * @param hexlen Length of hex ASCII
	 * @param buf Buffer to fill
	 * @param len Length of buffer
	 * @return Number of bytes actually written to buffer
	 */
	static unsigned int unhex(const char *hex,unsigned int hexlen,void *buf,unsigned int len)
		throw();

	/**
	 * @param buf Buffer to fill
	 * @param bytes Number of random bytes to generate
	 */
	static void getSecureRandom(void *buf,unsigned int bytes);

	/**
	 * Set modes on a file to something secure
	 * 
	 * This locks a file so that only the owner can access it. What it actually
	 * does varies by platform.
	 * 
	 * @param path Path to lock
	 * @param isDir True if this is a directory
	 */
	static void lockDownFile(const char *path,bool isDir);

	/**
	 * Get file last modification time
	 *
	 * Resolution is often only second, not millisecond, but the return is
	 * always in ms for comparison against now().
	 *
	 * @param path Path to file to get time
	 * @return Last modification time in ms since epoch or 0 if not found
	 */
	static uint64_t getLastModified(const char *path);

	/**
	 * @param path Path to check
	 * @return True if file or directory exists at path location
	 */
	static inline bool fileExists(const char *path)
	{
		return (getLastModified(path) != 0);
	}

	/**
	 * @param t64 Time in ms since epoch
	 * @return RFC1123 date string
	 */
	static std::string toRfc1123(uint64_t t64);

	/**
	 * @param tstr Time in RFC1123 string format
	 * @return Time in ms since epoch
	 */
	static uint64_t fromRfc1123(const char *tstr);
	static inline uint64_t fromRfc1123(const std::string &tstr) { return fromRfc1123(tstr.c_str()); }

	/**
	 * String append output function object for use with compress/decompress
	 */
	class StringAppendOutput
	{
	public:
		StringAppendOutput(std::string &s) : _s(s) {}
		inline void operator()(const void *data,unsigned int len) { _s.append((const char *)data,len); }
	private:
		std::string &_s;
	};

	/**
	 * STDIO FILE append output function object for compress/decompress
	 *
	 * Throws std::runtime_error on write error.
	 */
	class FILEAppendOutput
	{
	public:
		FILEAppendOutput(FILE *f) : _f(f) {}
		inline void operator()(const void *data,unsigned int len)
			throw(std::runtime_error)
		{
			if ((int)fwrite(data,1,len,_f) != (int)len)
				throw std::runtime_error("write failed");
		}
	private:
		FILE *_f;
	};

	/**
	 * Compress data
	 *
	 * O must be a function or function object that takes the following
	 * arguments: (const void *data,unsigned int len)
	 *
	 * @param in Input iterator that reads bytes (char, uint8_t, etc.)
	 * @param out Output iterator that writes bytes
	 */
	template<typename I,typename O>
	static inline void compress(I begin,I end,O out)
	{
		unsigned int bufLen = LZ4_compressBound(ZT_COMPRESSION_BLOCK_SIZE);
		char *buf = new char[bufLen * 2];
		char *buf2 = buf + bufLen;

		try {
			I inp(begin);
			for(;;) {
				unsigned int readLen = 0;
				while ((readLen < ZT_COMPRESSION_BLOCK_SIZE)&&(inp != end)) {
					buf[readLen++] = (char)*inp;
					++inp;
				}
				if (!readLen)
					break;

				uint32_t l = hton((uint32_t)readLen);
				out((const void *)&l,4); // original size

				if (readLen < 32) { // don't bother compressing itty bitty blocks
					l = 0; // stored
					out((const void *)&l,4);
					out((const void *)buf,readLen);
					continue;
				}

				int lz4CompressedLen = LZ4_compressHC(buf,buf2,(int)readLen);
				if ((lz4CompressedLen <= 0)||(lz4CompressedLen >= (int)readLen)) {
					l = 0; // stored
					out((const void *)&l,4);
					out((const void *)buf,readLen);
					continue;
				}

				l = hton((uint32_t)lz4CompressedLen); // lz4 only
				out((const void *)&l,4);
				out((const void *)buf2,(unsigned int)lz4CompressedLen);
			}

			delete [] buf;
		} catch ( ... ) {
			delete [] buf;
			throw;
		}
	}

	/**
	 * Decompress data
	 *
	 * O must be a function or function object that takes the following
	 * arguments: (const void *data,unsigned int len)
	 *
	 * @param in Input iterator that reads bytes (char, uint8_t, etc.)
	 * @param out Output iterator that writes bytes
	 * @return False on decompression error
	 */
	template<typename I,typename O>
	static inline bool decompress(I begin,I end,O out)
	{
		volatile char i32c[4];
		void *const i32cp = (void *)i32c;
		unsigned int bufLen = LZ4_compressBound(ZT_COMPRESSION_BLOCK_SIZE);
		char *buf = new char[bufLen * 2];
		char *buf2 = buf + bufLen;

		try {
			I inp(begin);
			while (inp != end) {
				i32c[0] = (char)*inp; if (++inp == end) { delete [] buf; return false; }
				i32c[1] = (char)*inp; if (++inp == end) { delete [] buf; return false; }
				i32c[2] = (char)*inp; if (++inp == end) { delete [] buf; return false; }
				i32c[3] = (char)*inp; if (++inp == end) { delete [] buf; return false; }
				unsigned int originalSize = ntoh(*((const uint32_t *)i32cp));
				i32c[0] = (char)*inp; if (++inp == end) { delete [] buf; return false; }
				i32c[1] = (char)*inp; if (++inp == end) { delete [] buf; return false; }
				i32c[2] = (char)*inp; if (++inp == end) { delete [] buf; return false; }
				i32c[3] = (char)*inp; if (++inp == end) { delete [] buf; return false; }
				uint32_t _compressedSize = ntoh(*((const uint32_t *)i32cp));
				unsigned int compressedSize = _compressedSize & 0x7fffffff;

				if (compressedSize) {
					if (compressedSize > bufLen) {
						delete [] buf;
						return false;
					}
					unsigned int readLen = 0;
					while ((readLen < compressedSize)&&(inp != end)) {
						buf[readLen++] = (char)*inp;
						++inp;
					}
					if (readLen != compressedSize) {
						delete [] buf;
						return false;
					}

					if (LZ4_uncompress_unknownOutputSize(buf,buf2,compressedSize,bufLen) != (int)originalSize) {
						delete [] buf;
						return false;
					} else out((const void *)buf2,(unsigned int)originalSize);
				} else { // stored
					if (originalSize > bufLen) {
						delete [] buf;
						return false;
					}
					unsigned int readLen = 0;
					while ((readLen < originalSize)&&(inp != end)) {
						buf[readLen++] = (char)*inp;
						++inp;
					}
					if (readLen != originalSize) {
						delete [] buf;
						return false;
					}

					out((const void *)buf,(unsigned int)originalSize);
				}
			}

			delete [] buf;
			return true;
		} catch ( ... ) {
			delete [] buf;
			throw;
		}
	}

	/**
	 * @return Current time in milliseconds since epoch
	 */
	static inline uint64_t now()
		throw()
	{
#ifdef __WINDOWS__
		FILETIME ft;
		SYSTEMTIME st;
		ULARGE_INTEGER tmp;
		GetSystemTime(&st);
		SystemTimeToFileTime(&st,&ft);
		tmp.LowPart = ft.dwLowDateTime;
		tmp.HighPart = ft.dwHighDateTime;
		return ( ((tmp.QuadPart - 116444736000000000ULL) / 10000L) + st.wMilliseconds );
#else
		struct timeval tv;
		gettimeofday(&tv,(struct timezone *)0);
		return ( (1000ULL * (uint64_t)tv.tv_sec) + (uint64_t)(tv.tv_usec / 1000) );
#endif
	};

	/**
	 * @return Current time in seconds since epoch, to the highest available resolution
	 */
	static inline double nowf()
		throw()
	{
#ifdef __WINDOWS__
		FILETIME ft;
		SYSTEMTIME st;
		ULARGE_INTEGER tmp;
		GetSystemTime(&st);
		SystemTimeToFileTime(&st,&ft);
		tmp.LowPart = ft.dwLowDateTime;
		tmp.HighPart = ft.dwHighDateTime;
		return (((double)(tmp.QuadPart - 116444736000000000ULL)) / 10000000.0);
#else
		struct timeval tv;
		gettimeofday(&tv,(struct timezone *)0);
		return ( ((double)tv.tv_sec) + (((double)tv.tv_usec) / 1000000.0) );
#endif
	}

	/**
	 * Read the full contents of a file into a string buffer
	 *
	 * The buffer isn't cleared, so if it already contains data the file's data will
	 * be appended.
	 *
	 * @param path Path of file to read
	 * @param buf Buffer to fill
	 * @return True if open and read successful
	 */
	static bool readFile(const char *path,std::string &buf);

	/**
	 * Write a block of data to disk, replacing any current file contents
	 *
	 * @param path Path to write
	 * @param buf Buffer containing data
	 * @param len Length of buffer
	 * @return True if entire file was successfully written
	 */
	static bool writeFile(const char *path,const void *buf,unsigned int len);

	/**
	 * Write a block of data to disk, replacing any current file contents
	 *
	 * @param path Path to write
	 * @param s Data to write
	 * @return True if entire file was successfully written
	 */
	static inline bool writeFile(const char *path,const std::string &s)
	{
		return writeFile(path,s.data(),(unsigned int)s.length());
	}

	/**
	 * Split a string by delimiter, with optional escape and quote characters
	 *
	 * @param s String to split
	 * @param sep One or more separators
	 * @param esc Zero or more escape characters
	 * @param quot Zero or more quote characters
	 * @return Vector of tokens
	 */
	static std::vector<std::string> split(const char *s,const char *const sep,const char *esc,const char *quot);

	/**
	 * Tokenize a string (alias for strtok_r or strtok_s depending on platform)
	 *
	 * @param str String to split
	 * @param delim Delimiters
	 * @param saveptr Pointer to a char * for temporary reentrant storage
	 */
	static inline char *stok(char *str,const char *delim,char **saveptr)
		throw()
	{
#ifdef __WINDOWS__
		return strtok_s(str,delim,saveptr);
#else
		return strtok_r(str,delim,saveptr);
#endif
	}

	// String to number converters -- defined here to permit portability
	// ifdefs for platforms that lack some of the strtoXX functions.
	static inline unsigned int strToUInt(const char *s)
		throw()
	{
		return (unsigned int)strtoul(s,(char **)0,10);
	}
	static inline int strToInt(const char *s)
		throw()
	{
		return (int)strtol(s,(char **)0,10);
	}
	static inline unsigned long strToULong(const char *s)
		throw()
	{
		return strtoul(s,(char **)0,10);
	}
	static inline long strToLong(const char *s)
		throw()
	{
		return strtol(s,(char **)0,10);
	}
	static inline unsigned long long strToU64(const char *s)
		throw()
	{
#ifdef __WINDOWS__
		return (unsigned long long)_strtoui64(s,(char **)0,10);
#else
		return strtoull(s,(char **)0,10);
#endif
	}
	static inline long long strTo64(const char *s)
		throw()
	{
#ifdef __WINDOWS__
		return (long long)_strtoi64(s,(char **)0,10);
#else
		return strtoll(s,(char **)0,10);
#endif
	}
	static inline unsigned int hexStrToUInt(const char *s)
		throw()
	{
		return (unsigned int)strtoul(s,(char **)0,16);
	}
	static inline int hexStrToInt(const char *s)
		throw()
	{
		return (int)strtol(s,(char **)0,16);
	}
	static inline unsigned long hexStrToULong(const char *s)
		throw()
	{
		return strtoul(s,(char **)0,16);
	}
	static inline long hexStrToLong(const char *s)
		throw()
	{
		return strtol(s,(char **)0,16);
	}
	static inline unsigned long long hexStrToU64(const char *s)
		throw()
	{
#ifdef __WINDOWS__
		return (unsigned long long)_strtoui64(s,(char **)0,16);
#else
		return strtoull(s,(char **)0,16);
#endif
	}
	static inline long long hexStrTo64(const char *s)
		throw()
	{
#ifdef __WINDOWS__
		return (long long)_strtoi64(s,(char **)0,16);
#else
		return strtoll(s,(char **)0,16);
#endif
	}
	static inline double strToDouble(const char *s)
		throw()
	{
		return strtod(s,(char **)0);
	}

	/**
	 * Perform a safe C string copy
	 *
	 * @param dest Destination buffer
	 * @param len Length of buffer
	 * @param src Source string
	 * @return True on success, false on overflow (buffer will still be 0-terminated)
	 */
	static inline bool scopy(char *dest,unsigned int len,const char *src)
		throw()
	{
		if (!len)
			return false; // sanity check
		char *end = dest + len;
		while ((*dest++ = *src++)) {
			if (dest == end) {
				*(--dest) = (char)0;
				return false;
			}
		}
		return true;
	}

	/**
	 * Trim whitespace from the start and end of a string
	 *
	 * @param s String to trim
	 * @return Trimmed string
	 */
	static std::string trim(const std::string &s);

	/**
	 * Like sprintf, but appends to std::string
	 *
	 * @param s String to append to
	 * @param fmt Printf format string
	 * @param ... Format arguments
	 * @throws std::bad_alloc Memory allocation failure
	 * @throws std::length_error Format + args exceeds internal buffer maximum
	 */
	static void stdsprintf(std::string &s,const char *fmt,...)
		throw(std::bad_alloc,std::length_error);

	/**
	 * Variant of snprintf that is portable and throws an exception
	 *
	 * @param buf Buffer to write to
	 * @param len Length of buffer in bytes
	 * @param fmt Format string
	 * @param ... Format arguments
	 * @throws std::length_error buf[] too short (buf[] will still be left null-terminated)
	 */
	static unsigned int snprintf(char *buf,unsigned int len,const char *fmt,...)
		throw(std::length_error);

	/**
	 * Count the number of bits set in an integer
	 *
	 * @param v 32-bit integer
	 * @return Number of bits set in this integer (0-32)
	 */
	static inline uint32_t countBits(uint32_t v)
		throw()
	{
		v = v - ((v >> 1) & (uint32_t)0x55555555);
		v = (v & (uint32_t)0x33333333) + ((v >> 2) & (uint32_t)0x33333333);
		return ((((v + (v >> 4)) & (uint32_t)0xF0F0F0F) * (uint32_t)0x1010101) >> 24);
	}

	/**
	 * Check if a memory buffer is all-zero
	 *
	 * @param p Memory to scan
	 * @param len Length of memory
	 * @return True if memory is all zero
	 */
	static inline bool isZero(const void *p,unsigned int len)
		throw()
	{
		for(unsigned int i=0;i<len;++i) {
			if (((const unsigned char *)p)[i])
				return false;
		}
		return true;
	}

	/**
	 * Match two strings with bits masked netmask-style
	 *
	 * @param a First string
	 * @param abits Number of bits in first string
	 * @param b Second string
	 * @param bbits Number of bits in second string
	 * @return True if min(abits,bbits) match between a and b
	 */
	static inline bool matchNetmask(const void *a,unsigned int abits,const void *b,unsigned int bbits)
		throw()
	{
		const unsigned char *aptr = (const unsigned char *)a;
		const unsigned char *bptr = (const unsigned char *)b;

		while ((abits >= 8)&&(bbits >= 8)) {
			if (*aptr++ != *bptr++)
				return false;
			abits -= 8;
			bbits -= 8;
		}

		unsigned char mask = 0xff << (8 - ((abits > bbits) ? bbits : abits));
		return ((*aptr & mask) == (*aptr & mask));
	}

	static inline uint8_t hton(uint8_t n) throw() { return n; }
	static inline int8_t hton(int8_t n) throw() { return n; }
	static inline uint16_t hton(uint16_t n) throw() { return htons(n); }
	static inline int16_t hton(int16_t n) throw() { return (int16_t)htons((uint16_t)n); }
	static inline uint32_t hton(uint32_t n) throw() { return htonl(n); }
	static inline int32_t hton(int32_t n) throw() { return (int32_t)htonl((uint32_t)n); }
	static inline uint64_t hton(uint64_t n)
		throw()
	{
#if __BYTE_ORDER == __LITTLE_ENDIAN
#ifdef __GNUC__
		return __builtin_bswap64(n);
#else
		return (
			((n & 0x00000000000000FFULL) << 56) | 
			((n & 0x000000000000FF00ULL) << 40) | 
			((n & 0x0000000000FF0000ULL) << 24) | 
			((n & 0x00000000FF000000ULL) <<  8) | 
			((n & 0x000000FF00000000ULL) >>  8) | 
			((n & 0x0000FF0000000000ULL) >> 24) | 
			((n & 0x00FF000000000000ULL) >> 40) | 
			((n & 0xFF00000000000000ULL) >> 56)
		);
#endif
#else
		return n;
#endif
	}
	static inline int64_t hton(int64_t n) throw() { return (int64_t)hton((uint64_t)n); }

	static inline uint8_t ntoh(uint8_t n) throw() { return n; }
	static inline int8_t ntoh(int8_t n) throw() { return n; }
	static inline uint16_t ntoh(uint16_t n) throw() { return ntohs(n); }
	static inline int16_t ntoh(int16_t n) throw() { return (int16_t)ntohs((uint16_t)n); }
	static inline uint32_t ntoh(uint32_t n) throw() { return ntohl(n); }
	static inline int32_t ntoh(int32_t n) throw() { return (int32_t)ntohl((uint32_t)n); }
	static inline uint64_t ntoh(uint64_t n)
		throw()
	{
#if __BYTE_ORDER == __LITTLE_ENDIAN
#ifdef __GNUC__
		return __builtin_bswap64(n);
#else
		return (
			((n & 0x00000000000000FFULL) << 56) | 
			((n & 0x000000000000FF00ULL) << 40) | 
			((n & 0x0000000000FF0000ULL) << 24) | 
			((n & 0x00000000FF000000ULL) <<  8) | 
			((n & 0x000000FF00000000ULL) >>  8) | 
			((n & 0x0000FF0000000000ULL) >> 24) | 
			((n & 0x00FF000000000000ULL) >> 40) | 
			((n & 0xFF00000000000000ULL) >> 56)
		);
#endif
#else
		return n;
#endif
	}
	static inline int64_t ntoh(int64_t n) throw() { return (int64_t)ntoh((uint64_t)n); }

	/**
	 * Hexadecimal characters 0-f
	 */
	static const char HEXCHARS[16];
};

} // namespace ZeroTier

#endif

