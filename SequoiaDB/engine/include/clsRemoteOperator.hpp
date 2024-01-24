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

   Source File Name = clsRemoteOperator.hpp

   Descriptive Name = Remote operator Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          3/10/2020   LYB Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLS_REMOTE_OPERATOR_HPP_
#define CLS_REMOTE_OPERATOR_HPP_

#include "sdbRemoteOperator.hpp"
#include "rtnContextBuff.hpp"
#include "msgMessage.hpp"

using namespace bson ;

namespace engine
{

   class _pmdEDUCB ;
   class _pmdSessionBase ;
   class _pmdProcessorBase ;

   class _clsRemoteOperator : public _IRemoteOperator
   {
   public:
      _clsRemoteOperator() ;
      virtual ~_clsRemoteOperator() ;

   public:
      virtual INT32           transBegin() ;

      virtual INT32           transCommit() ;

      virtual INT32           transRollback() ;

      virtual INT32           insert( const CHAR *clName,
                                      const BSONObj &insertor,
                                      INT32 flags,
                                      utilInsertResult *pResult = NULL ) ;

      virtual INT32           remove( const CHAR *clName,
                                      const BSONObj &matcher,
                                      const BSONObj &hint, INT32 flags,
                                      utilDeleteResult *pResult = NULL ) ;

      virtual INT32           update( const CHAR *clName,
                                      const BSONObj &matcher,
                                      const BSONObj &updator,
                                      const BSONObj &hint,
                                      INT32 flags,
                                      utilUpdateResult *pResult = NULL ) ;

      virtual INT32           truncateCL( const CHAR *clName ) ;

      virtual INT32           snapshot( INT64 &contextID,
                                        const CHAR *pCommand,
                                        const BSONObj &matcher = BSONObj(),
                                        const BSONObj &selector = BSONObj(),
                                        const BSONObj &orderBy = BSONObj(),
                                        const BSONObj &hint = BSONObj(),
                                        INT64 numToSkip = 0,
                                        INT64 numToReturn = -1,
                                        INT32 flag = 0 ) ;

      virtual INT32           snapshotIndexes( INT64 &contextID,
                                               const CHAR *clName,
                                               const CHAR *indexName,
                                               BOOLEAN rawData = FALSE ) ;

      virtual INT32           list( INT64 &contextID,
                                    const CHAR *pCommand,
                                    const BSONObj &matcher = BSONObj(),
                                    const BSONObj &selector = BSONObj(),
                                    const BSONObj &orderBy = BSONObj(),
                                    const BSONObj &hint = BSONObj(),
                                    INT64 numToSkip = 0,
                                    INT64 numToReturn = -1,
                                    INT32 flag = 0 ) ;

      virtual INT32           listCSIndexes( INT64 &contextID,
                                             utilCSUniqueID csUniqID ) ;

      virtual INT32           stopCriticalMode( const UINT32 &groupID ) ;

      virtual INT32           stopMaintenanceMode( const UINT32 &groupID,
                                                   const CHAR *pNodeName ) ;

      virtual UINT64          getSucCount() ;

      virtual UINT64          getFailureCount() ;

      virtual _sdbRemoteOpCtrl*  getController() ;

   public:
      INT32                   init( _pmdEDUCB *cb ) ;

   private:
      void                    _clear() ;
      void                    _generateErrorInfo( INT32 rc,
                                                  rtnContextBuf &contextBuff,
                                                  BSONObjBuilder &retBuilder ) ;
      void                    _getResponse( INT32 rc,
                                            rtnContextBuf &contextBuff,
                                            BSONObjBuilder &retBuilder,
                                            const CHAR **ppResData = NULL,
                                            INT32 *pResLen = NULL,
                                            INT32 *pResNum = NULL ) ;
      INT32                   _processMsg( MsgHeader* msg ) ;
      INT32                   _processMsg( MsgHeader* msg,
                                           rtnContextBuf& contextBuff,
                                           INT64& contextID ) ;
      void                    _increaseCount( INT32 rc ) ;

   private:
      _pmdProcessorBase *_processor ;

      _pmdSessionBase *_session ;
      _pmdEDUCB *_cb ;

      UINT64 _sucCount ;
      UINT64 _failureCount ;
      _sdbRemoteOpCtrl *_ctrl ;

      // to keep the bson's data point avaliable
      BSONObj _errorInfo ;
   };

   typedef _clsRemoteOperator clsRemoteOperator ;

}

#endif //CLS_REMOTE_OPERATOR_HPP_

