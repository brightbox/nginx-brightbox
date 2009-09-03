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
#include "CachedFileStat.h"
#include "CachedFileStat.hpp"

extern "C" {

struct CachedFileStat {
	Passenger::CachedFileStat cfs;
	
	CachedFileStat(unsigned int maxSize): cfs(maxSize) { }
};

CachedFileStat *
cached_file_stat_new(unsigned int max_size) {
	return new CachedFileStat(max_size);
}

void
cached_file_stat_free(CachedFileStat *cstat) {
	delete cstat;
}

int
cached_file_stat_perform(CachedFileStat *cstat,
                         const char *filename,
                         struct stat *buf,
                         unsigned int throttle_rate) {
	try {
		return cstat->cfs.stat(filename, buf, throttle_rate);
	} catch (const Passenger::TimeRetrievalException &e) {
		errno = e.code();
		return -1;
	} catch (const boost::thread_interrupted &) {
		errno = EINTR;
		return -1;
	}
}

} // extern "C"
