/*
 *  Phusion Passenger - http://www.modrails.com/
 *  Copyright (c) 2009 Phusion
 *
 *  "Phusion Passenger" is a trademark of Hongli Lai & Ninh Bui.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
 */
#ifndef _PASSENGER_CACHED_FILE_STAT_HPP_
#define _PASSENGER_CACHED_FILE_STAT_HPP_

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include <cerrno>
#include <cassert>
#include <string>
#include <boost/shared_ptr.hpp>
#include <oxt/system_calls.hpp>

#include "SystemTime.h"

namespace Passenger {

using namespace std;
using namespace oxt;
using namespace boost;

/**
 * CachedFileStat allows one to stat() files at a throttled rate, in order
 * to minimize stress on the filesystem. It does this by caching the old stat
 * data for a specified amount of time.
 *
 * The cache has a maximum size, which may be altered during runtime. If a
 * file that wasn't in the cache is being stat()ed, and the cache is full,
 * then the oldest cache entry will be removed.
 *
 * This class is fully thread-safe.
 */
class CachedFileStat {
public:
	/** Represents a cached file stat entry. */
	class Entry {
	private:
		/** The last return value of stat(). */
		int last_result;
		
		/** The errno set by the last stat() call. */
		int last_errno;
		
		/** The last time a stat() was performed. */
		time_t last_time;
		
		/**
		 * Checks whether <em>interval</em> seconds have elapsed since <em>begin</em>
		 * The current time is returned via the <tt>currentTime</tt> argument,
		 * so that the caller doesn't have to call time() again if it needs the current
		 * time.
		 *
		 * @pre begin <= time(NULL)
		 * @return Whether <tt>interval</tt> seconds have elapsed since <tt>begin</tt>.
		 * @throws TimeRetrievalException Something went wrong while retrieving the time.
		 * @throws boost::thread_interrupted
		 */
		bool expired(time_t begin, unsigned int interval, time_t &currentTime) {
			currentTime = SystemTime::get();
			return (unsigned int) (currentTime - begin) >= interval;
		}
		
	public:
		/** The cached stat info. */
		struct stat info;
		
		/** This entry's filename. */
		string filename;
		
		/**
		 * Creates a new Entry object. The file will not be
		 * stat()ted until you call refresh().
		 *
		 * @param filename The file to stat.
		 */
		Entry(const string &filename) {
			memset(&info, 0, sizeof(struct stat));
			this->filename = filename;
			last_result = -1;
			last_errno = 0;
			last_time = 0;
		}
		
		/**
		 * Re-stat() the file, if necessary. If <tt>throttleRate</tt> seconds have
		 * passed since the last time stat() was called, then the file will be
		 * re-stat()ted.
		 *
		 * The stat information, which may either be the result of a new stat() call
		 * or just the old cached information, is be available in the <tt>info</tt>
		 * member.
		 *
		 * @return 0 if the stat() call succeeded or if no stat() was performed,
		 *         -1 if something went wrong while statting the file. In the latter
		 *         case, <tt>errno</tt> will be populated with an appropriate error code.
		 * @throws TimeRetrievalException Something went wrong while retrieving the
		 *         system time.
		 * @throws boost::thread_interrupted
		 */
		int refresh(unsigned int throttleRate) {
			time_t currentTime;

			if (expired(last_time, throttleRate, currentTime)) {
				last_result = syscalls::stat(filename.c_str(), &info);
				last_errno = errno;
				last_time = currentTime;
				return last_result;
			} else {
				errno = last_errno;
				return last_result;
			}
		}
	};
	
	typedef shared_ptr<Entry> EntryPtr;
	typedef list<EntryPtr> EntryList;
	typedef map<string, EntryList::iterator> EntryMap;
	
	unsigned int maxSize;
	EntryList entries;
	EntryMap cache;
	mutable boost::mutex lock;
	
	/**
	 * Creates a new CachedFileStat object.
	 *
	 * @param maxSize The maximum cache size. A size of 0 means unlimited.
	 */
	CachedFileStat(unsigned int maxSize = 0) {
		this->maxSize = maxSize;
	}
	
	/**
	 * Stats the given file. If <tt>throttleRate</tt> seconds have passed since
	 * the last time stat() was called on this file, then the file will be
	 * re-stat()ted, otherwise the cached stat information will be returned.
	 *
	 * @param filename The file to stat.
	 * @param stat A pointer to a stat struct; the retrieved stat information
	 *             will be stored here.
	 * @param throttleRate Tells this CachedFileStat that the file may only
	 *        be statted at most every <tt>throttleRate</tt> seconds.
	 * @return 0 if the stat() call succeeded or if the cached stat information was used;
	 *         -1 if something went wrong while statting the file. In the latter
	 *         case, <tt>errno</tt> will be populated with an appropriate error code.
	 * @throws SystemException Something went wrong while retrieving the
	 *         system time. stat() errors will <em>not</em> result in
	 *         SystemException being thrown.
	 * @throws boost::thread_interrupted
	 */
	int stat(const string &filename, struct stat *buf, unsigned int throttleRate = 0) {
		boost::unique_lock<boost::mutex> l(lock);
		EntryMap::iterator it(cache.find(filename));
		EntryPtr entry;
		int ret;
		
		if (it == cache.end()) {
			// Filename not in cache.
			// If cache is full, remove the least recently used
			// cache entry.
			if (maxSize != 0 && cache.size() == maxSize) {
				EntryList::iterator listEnd(entries.end());
				listEnd--;
				string filename((*listEnd)->filename);
				entries.pop_back();
				cache.erase(filename);
			}
			
			// Add to cache as most recently used.
			entry = EntryPtr(new Entry(filename));
			entries.push_front(entry);
			cache[filename] = entries.begin();
		} else {
			// Cache hit.
			entry = *it->second;
			
			// Mark this cache item as most recently used.
			entries.erase(it->second);
			entries.push_front(entry);
			cache[filename] = entries.begin();
		}
		ret = entry->refresh(throttleRate);
		*buf = entry->info;
		return ret;
	}
	
	/**
	 * Change the maximum size of the cache. If the new size is larger
	 * than the old size, then the oldest entries in the cache are
	 * removed.
	 *
	 * A size of 0 means unlimited.
	 */
	void setMaxSize(unsigned int maxSize) {
		boost::unique_lock<boost::mutex> l(lock);
		if (maxSize != 0) {
			int toRemove = cache.size() - maxSize;
			for (int i = 0; i < toRemove; i++) {
				string filename(entries.back()->filename);
				entries.pop_back();
				cache.erase(filename);
			}
		}
		this->maxSize = maxSize;
	}
	
	/**
	 * Returns whether <tt>filename</tt> is in the cache.
	 */
	bool knows(const string &filename) const {
		boost::unique_lock<boost::mutex> l(lock);
		return cache.find(filename) != cache.end();
	}
};

} // namespace Passenger

#endif /* _PASSENGER_CACHED_FILE_STAT_HPP_ */
