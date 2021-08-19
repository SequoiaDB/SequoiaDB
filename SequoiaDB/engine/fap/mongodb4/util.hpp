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

   Source File Name = util.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== ==============================================
          01/03/2020  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef _SDB_MSG_CONVERTER_UTIL_HPP_
#define _SDB_MSG_CONVERTER_UTIL_HPP_

#include "oss.hpp"
#include "iostream"
#include "msgBuffer.hpp"
#include "../../bson/bson.hpp"
#include <string>
#include "../../bson/lib/md5.hpp"

class _baseConverter : public SDBObject
{
public:
   _baseConverter() : _msglen( 0 ), _msgdata( NULL )
   {}

   virtual ~_baseConverter()
   {
      if ( NULL != _msgdata )
      {
         _msgdata = NULL ;
      }
   }

   void loadFrom( CHAR *msg, const INT32 len )
   {
      if ( NULL == msg )
      {
         return ;
      }

      _msgdata = msg ;
      _msglen  = len ;
   }

   virtual INT32 convert( msgBuffer &out )
   {
      return SDB_OK ;
   }

protected:
   INT32   _msglen ;
   CHAR   *_msgdata ;
} ;
typedef _baseConverter baseConverter ;

///< check big endian machine
inline BOOLEAN checkBigEndian()
{
   BOOLEAN bigEndian = FALSE ;
   union
   {
      unsigned int i ;
      unsigned char s[4] ;
   } c ;

   c.i = 0x12345678 ;
   if ( 0x12 == c.s[0] )
   {
      bigEndian = TRUE ;
   }

   return bigEndian ;
}

#endif
