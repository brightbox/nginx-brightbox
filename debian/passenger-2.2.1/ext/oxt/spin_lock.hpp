/*
 * OXT - OS eXtensions for boosT
 * Provides important functionality necessary for writing robust server software.
 *
 * Copyright (c) 2008 Phusion
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef _OXT_SPIN_LOCK_HPP_
#define _OXT_SPIN_LOCK_HPP_

// At the time of writing (July 22, 2008), these operating systems don't
// support pthread spin locks:
// - OpenBSD 4.3
// - Solaris 9
// - MacOS X
#if defined(__OpenBSD__) || defined(__SOLARIS9__) || defined(__APPLE__)
	#define OXT_NO_PTHREAD_SPINLOCKS
#endif

#ifndef GCC_VERSION
	#define GCC_VERSION (__GNUC__ * 10000 \
	                     + __GNUC_MINOR__ * 100 \
	                     + __GNUC_PATCH_LEVEL__)
#endif

#if (GCC_VERSION > 40100 && defined(__i386__)) || defined(IN_DOXYGEN)
	// GCC 4.0 doesn't support __sync instructions while GCC 4.2
	// does. I'm not sure whether support for it started in 4.1 or
	// 4.2, so the above version check may have to be changed later.
	#include "detail/spin_lock_gcc_x86.hpp"
#elif !defined(WIN32) && !defined(OXT_NO_PTHREAD_SPINLOCKS)
	#include "detail/spin_lock_pthreads.hpp"
#else
	#include "detail/spin_lock_portable.hpp"
#endif

#endif /* _OXT_SPIN_LOCK_HPP_ */

