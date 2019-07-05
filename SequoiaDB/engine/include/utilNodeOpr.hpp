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

   Source File Name = utilNodeOpr.hpp

   Descriptive Name =

   When/how to use: node operation

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/08/2014  XJH Initial Draft

   Last Changed =

******************************************************************************/

#ifndef UTIL_NODE_OPR_HPP__
#define UTIL_NODE_OPR_HPP__

#include "core.hpp"
#include <string>
#include <vector>
#include "ossNPipe.hpp"

using namespace std ;

namespace engine
{

   /*
      _utilNodePipe define
   */
   class _utilNodePipe
   {
      public:
         _utilNodePipe() ;
         ~_utilNodePipe() ;

         INT32    createPipe( const CHAR *svcname ) ;
         INT32    deletePipe() ;

         /*
            In windows, pid is not used
         */
         INT32    openPipe( const CHAR *svcname, OSSPID pid ) ;
         INT32    closePipe() ;

         INT32    readPipe( CHAR *pBuff, INT32 readSize, INT32 &hasRead ) ;
         INT32    writePipe( const CHAR *pBuff, INT32 size ) ;

         INT32    autoRelease() ;

         BOOLEAN  isConnectError() const { return _connectError ; }

         const CHAR* getReadPipeName() const { return _pipeRName ; }
         const CHAR* getWritePipeName() const { return _pipeWName ; }

         INT32    connectPipe() ;
         void     disconnectPipe() ;

      private:
         BOOLEAN        _isCreate ;
         BOOLEAN        _isConnect ;
         BOOLEAN        _isOpen ;
         BOOLEAN        _connectError ;

         OSSNPIPE       _pipeRHandle ;
         OSSNPIPE       _pipeWHandle ;
         CHAR           _pipeRName[ OSS_NPIPE_MAX_NAME_LEN + 1 ] ;
         CHAR           _pipeWName[ OSS_NPIPE_MAX_NAME_LEN + 1 ] ;

   } ;
   typedef _utilNodePipe utilNodePipe ;

   /*
      _utilNodeInfo define
   */
   struct _utilNodeInfo
   {
      string   _orgname ;
      string   _svcname ;
      INT32    _type ;
      INT32    _role ;
      OSSPID   _pid ;
      INT32    _groupID ;
      INT32    _nodeID ;
      INT32    _primary ;
      INT32    _isAlone ;
      string   _groupName ;
      string   _dbPath ;
      UINT64   _startTime ;

      _utilNodeInfo()
      {
         _pid        = OSS_INVALID_PID ;
         _type       = 0 ;
         _role       = 0 ;
         _groupID    = 0 ;
         _nodeID     = 0 ;
         _primary    = -1 ;
         _isAlone    = 0 ;
         _startTime  = 0 ;
      }
   } ;
   typedef _utilNodeInfo utilNodeInfo ;

   typedef vector< utilNodeInfo >   UTIL_VEC_NODES ;

   /*
      list nodes
   */
   INT32    utilListNodes( UTIL_VEC_NODES &nodes,
                           INT32 typeFilter = -1,
                           const CHAR *svcnameFilter = NULL,
                           OSSPID pidFilter = OSS_INVALID_PID,
                           INT32 roleFilter = -1,
                           BOOLEAN allowAloneCM = FALSE ) ;

   /*
      send command to node pipe and read result from node pipe
   */
   INT32 utilWriteReadPipe( const CHAR *pSvcName, OSSPID pid,
                            const CHAR *pWriteBuf, INT32 writeLen,
                            CHAR *pReadBuf, INT32 readLen,
                            BOOLEAN checkLen ) ;

   /*
      get node group id/name, node id, dbpath info
   */
   INT32    utilGetNodeExtraInfo( utilNodeInfo &info ) ;

   #define UTIL_WAIT_NODE_TIMEOUT         ( 15 * 60 ) // second

   /*
      enum nodes
   */
   INT32    utilEnumNodes( const string &localPath,
                           UTIL_VEC_NODES &nodes,
                           INT32 typeFilter = -1,
                           const CHAR *svcnameFilter = NULL,
                           INT32 roleFilter = -1 ) ;

   /*
      wait node bussiness ok
   */
   INT32    utilWaitNodeOK( utilNodeInfo &node,
                            const CHAR *svcname,
                            OSSPID pid = OSS_INVALID_PID,
                            INT32 typeFilter = -1,
                            INT32 timeout = UTIL_WAIT_NODE_TIMEOUT,
                            BOOLEAN allowAloneCM = FALSE ) ;

   #define UTIL_STOP_NODE_TIMEOUT         ( 5 * 60 ) // second

   /*
      stop node
   */
   INT32    utilStopNode ( utilNodeInfo &node,
                           INT32 timeout = UTIL_STOP_NODE_TIMEOUT,
                           BOOLEAN force = FALSE,
                           BOOLEAN skipKill = FALSE ) ;

   INT32    utilAsyncStopNode( utilNodeInfo &node ) ;

   /*
      Notiry the sdb node to end pipe dup
   */
   INT32    utilEndNodePipeDup( const CHAR *svcname, OSSPID pid ) ;

   struct _utilNodeVerInfo
   {
      INT32    _version ;
      INT32    _subVersion ;
      INT32    _fixVersion ;
      INT32    _release ;
      string   _buildStr ;
   } ;
   typedef _utilNodeVerInfo utilNodeVerInfo ;

   /*
      get node version info
   */
   INT32    utilGetNodeVerInfo( const CHAR* pCommand,
                                utilNodeVerInfo &verInfo ) ;

}

#endif // UTIL_NODE_OPR_HPP__

