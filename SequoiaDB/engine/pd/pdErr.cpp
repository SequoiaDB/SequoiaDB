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

   Source File Name = pdErr.cpp

   Descriptive Name = Problem Determination Header

   When/how to use: this program may be used on binary and text-formatted
   versions of PD component. This file contains declare of PD functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/04/2016  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "pdErr.hpp"

static OSS_THREAD_LOCAL INT32 s_errno = 0 ;

INT32 pdGetLastError()
{
   return s_errno ;
}

void pdSetLastError( INT32 err )
{
   s_errno = err ;
}

/*
   _pdGeneralException implement
*/
_pdGeneralException::_pdGeneralException( const std::string &str )
{
   _err = 0 ;
   _s = str ;
   pdSetLastError( _err ) ;
}

_pdGeneralException::_pdGeneralException( INT32 err )
{
   _err = err ;
   pdSetLastError( _err ) ;
}

_pdGeneralException::_pdGeneralException( INT32 err, const std::string &str )
{
   _err = err ;
   _s = str ;
   pdSetLastError( _err ) ;
}

_pdGeneralException::_pdGeneralException( const std::string &str, INT32 err )
{
   _err = err ;
   _s = str ;
   pdSetLastError( _err ) ;
}

