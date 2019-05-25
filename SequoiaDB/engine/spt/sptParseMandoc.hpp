/*   $Id: main.h,v 1.15 2011/10/06 22:29:12 kristaps Exp $ */
/*
 * Copyright (c) 2009, 2010, 2011 Kristaps Dzonsons <kristaps@bsd.lv>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef   SPT_PARSE_MANDOC_HPP__
#define   SPT_PARSE_MANDOC_HPP__
#include <core.hpp>

class _sptParseMandoc
{
public:
   static _sptParseMandoc& getInstance() ;
   INT32 parse( const CHAR* filename ) ;
private:
   _sptParseMandoc() {} ;
   ~_sptParseMandoc() {} ;
   _sptParseMandoc( const _sptParseMandoc& ) ;
   _sptParseMandoc& operator=( const _sptParseMandoc& ) ;
} ;
typedef _sptParseMandoc sptParseMandoc ;

#endif // SPT_PARSE_MANDOC_HPP__