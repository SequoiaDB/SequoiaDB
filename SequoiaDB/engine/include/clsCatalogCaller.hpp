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

   Source File Name = clsCatalogCaller.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Replication component. This file contains structure for
   replication control block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLSCATALOGCALLER_HPP_
#define CLSCATALOGCALLER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "msg.hpp"

#include "ossMemPool.hpp"

using namespace std ;
namespace engine
{
   class _netRouteAgent ;

   struct _clsCataCallerMeta
   {
      MsgHeader *header ;
      UINT32 bufLen ;
      INT32  timeout ;
      UINT32 sendTimes ;

      _clsCataCallerMeta()
      :header(NULL),
       bufLen(0),
       timeout(-1),
       sendTimes(1)
      {
      }

      ~_clsCataCallerMeta()
      {
         if ( NULL != header )
         {
            SDB_OSS_FREE( header ) ;
            header = NULL ;
         }
         bufLen = 0 ;
         timeout = -1 ;
         sendTimes = 0 ;
      }
   } ;
   typedef ossPoolMap<UINT32, _clsCataCallerMeta>  callerMeta ;

   class _clsCatalogCaller : public SDBObject
   {
   public:
      _clsCatalogCaller() ;
      ~_clsCatalogCaller() ;

   public:
      INT32 call( MsgHeader *header, UINT32 times = 1 ) ;

      void remove( const MsgHeader *header, INT32 result ) ;
      void remove( INT32 opCode ) ;

      void handleTimeout( const UINT32 &millisec ) ;

   private:
      callerMeta _meta ;

   } ;

   typedef class _clsCatalogCaller clsCatalogCaller ;
}

#endif

