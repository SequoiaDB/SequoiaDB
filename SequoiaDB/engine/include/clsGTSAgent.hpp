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

   Source File Name = clsGTSAgent.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Replication component. This file contains structure for
   replication control block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/05/2012  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLS_GTS_AGENT_HPP__
#define CLS_GTS_AGENT_HPP__

#include "oss.hpp"
#include "dpsDef.hpp"
#include "ossMemPool.hpp"
#include "dpsTransDef.hpp"
#include "sdbInterface.hpp"

namespace engine
{
   class _clsShardMgr ;
   /*
      _clsGTSAgent define
   */
   class _clsGTSAgent : public SDBObject, public _dpsTransEvent
   {
      public:
         _clsGTSAgent( _clsShardMgr *pShardMgr ) ;
         ~_clsGTSAgent() ;

      public:
         virtual INT32  onRollbackAll() ;

      public:

         INT32       checkTransStatus( DPS_TRANS_ID transID,
                                       UINT32 nodeNum,
                                       const UINT64 *pNodes,
                                       IExecutor *cb,
                                       DPS_TRANS_STATUS &status ) ;

      protected:

         INT32       _checkTransStatus( DPS_TRANS_ID transID,
                                        UINT32 group,
                                        IExecutor *cb,
                                        DPS_TRANS_STATUS &status ) ;

         INT32       _syncCheckTransStatus( DPS_TRANS_ID transID,
                                            DPS_LSN_OFFSET curLsn,
                                            DPS_TRANS_STATUS &status ) ;

         INT32       _commitTrans( DPS_TRANS_ID transID,
                                   DPS_LSN_OFFSET lastLsn,
                                   DPS_LSN_OFFSET &curLsn ) ;

      private:
         _clsShardMgr         *_pShardMgr ;

   } ;
   typedef _clsGTSAgent clsGTSAgent ;

}


#endif // CLS_GTS_AGENT_HPP__

