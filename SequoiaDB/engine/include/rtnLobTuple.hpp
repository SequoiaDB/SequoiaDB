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
#include "ossMemPool.hpp"

namespace engine
{
   struct _rtnLobTuple
   {
      // tuple must be defined in the header. see _coordLobStream::_shardData
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

   typedef ossPoolList<_rtnLobTuple> RTN_LOB_TUPLES ;
}

#endif

