// Copyright (c) 2020, University of Washington All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice, this list
// of conditions and the following disclaimer.
//
// Redistributions in binary form must reproduce the above copyright notice, this
// list of conditions and the following disclaimer in the documentation and/or
// other materials provided with the distribution.
//
// Neither the name of the copyright holder nor the names of its contributors may
// be used to endorse or promote products derived from this software without
// specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
// ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// This is a slightly modified copy of the argp library from Newlib

/* Real definitions for extern inline functions in argp-fmtstream.h
   Copyright (C) 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Written by Miles Bader <miles@gnu.ai.mit.edu>.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define ARGP_FS_EI
#undef __OPTIMIZE__
#define __OPTIMIZE__ 1
#include "argp-fmtstream.h"

/* Add weak aliases.  */
#if _LIBC - 0 && !defined (ARGP_FMTSTREAM_USE_LINEWRAP) && defined (weak_alias)

weak_alias (__argp_fmtstream_putc, argp_fmtstream_putc)
weak_alias (__argp_fmtstream_puts, argp_fmtstream_puts)
weak_alias (__argp_fmtstream_write, argp_fmtstream_write)
weak_alias (__argp_fmtstream_set_lmargin, argp_fmtstream_set_lmargin)
weak_alias (__argp_fmtstream_set_rmargin, argp_fmtstream_set_rmargin)
weak_alias (__argp_fmtstream_set_wmargin, argp_fmtstream_set_wmargin)
weak_alias (__argp_fmtstream_point, argp_fmtstream_point)

#endif
