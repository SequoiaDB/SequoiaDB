/*******************************************************************************


   Copyright (C) 2023-present SequoiaDB Ltd.

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

   Source File Name = utilAccessData.hpp

   Descriptive Name =

   When/how to use: parse Data util

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/30/2013  JW  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef UTIL_MODE_ACCESS_DATA_HPP_
#define UTIL_MODE_ACCESS_DATA_HPP_

#include "core.hpp"
#include "ossIO.hpp"
#include "ossDynamicLoad.hpp"

class _utilAccessData : public SDBObject
{
public:
   virtual ~_utilAccessData()
   {
   }
   virtual INT32 initialize ( void *pParamet ) = 0 ;
   virtual INT32 readNextBuffer ( CHAR *pBuffer, UINT32 &size ) = 0 ;
} ;
typedef class _utilAccessData utilAccessData ;


/*              This is model                                      
                local IO
*/

struct utilAccessParametLocalIO
{
   const CHAR *pFileName ;
   utilAccessParametLocalIO() : pFileName(NULL)
   {
   }
} ;

class _utilAccessDataLocalIO : public _utilAccessData
{
private:
   OSSFILE _fileIO ;
public:
   _utilAccessDataLocalIO() ;
   virtual ~_utilAccessDataLocalIO() ;
   virtual INT32 initialize ( void *pParamet ) ;
   virtual INT32 readNextBuffer ( CHAR *pBuffer, UINT32 &size ) ;
} ;
typedef class _utilAccessDataLocalIO utilAccessDataLocalIO ;

#endif