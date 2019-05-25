/*******************************************************************************

   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = rtnLobTuple.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/31/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_LOBTUPLE_HPP
#define RTN_LOBTUPLE_HPP

#include "msg.h"
#include <list>

namespace engine
{
   struct _rtnLobTuple
   {
      MsgLobTuple tuple ;
      const CHAR *data ;

      _rtnLobTuple( UINT32 len,
                    UINT32 sequence,
                    SINT64 offset,
                    const CHAR *d )
      :data( d )
     {
        tuple.columns.len = len ;
        tuple.columns.sequence = sequence ;
        tuple.columns.offset = offset ; 
     }

     _rtnLobTuple()
      :data( NULL )
     {
        tuple.columns.len = 0 ;
        tuple.columns.sequence = 0 ;
        tuple.columns.offset = -1 ;
     }

     BOOLEAN empty() const
     {
        return 0 == tuple.columns.len ;
     }

     void clear()
     {
        tuple.columns.len = 0 ;
        tuple.columns.sequence = 0 ;
        tuple.columns.offset = -1 ;
        data = NULL ;
     }
   } ;

   typedef std::list<_rtnLobTuple> RTN_LOB_TUPLES ;
}

#endif

