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

   Source File Name = pdErr.hpp

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
#ifndef PD_ERR_HPP__
#define PD_ERR_HPP__

#include "core.h"
#include "oss.h"
#include <string>
#include <stdlib.h>

/*
   Get the thread-local last error no
*/
INT32    pdGetLastError() ;

/*
   Set the thread-local last error no
*/
void     pdSetLastError( INT32 err ) ;

/*
   _pdGeneralException define
*/
class _pdGeneralException : public std::exception
{
   public:
      _pdGeneralException( const std::string &str ) ;
      _pdGeneralException( INT32 err ) ;
      _pdGeneralException( INT32 err, const std::string &str ) ;
      _pdGeneralException( const std::string &str, INT32 err ) ;
      ~_pdGeneralException() throw() {}

      virtual const CHAR *what() const throw()
      {
         return _s.c_str() ;
      }

      /*
         Get errno use pdGetLastError() function
      */

   private:
      INT32       _err ;
      std::string _s ;
} ;
typedef class _pdGeneralException pdGeneralException ;

#endif // PD_ERR_HPP__

