/*******************************************************************************
   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************************/
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