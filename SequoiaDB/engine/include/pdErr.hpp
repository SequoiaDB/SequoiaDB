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

