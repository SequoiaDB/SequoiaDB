/*******************************************************************************

   Copyright (C) 2012-2014 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

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

