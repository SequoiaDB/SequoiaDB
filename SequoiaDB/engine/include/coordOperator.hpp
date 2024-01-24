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

   Source File Name = coordOperator.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          13/04/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef COORD_OPERATOR_HPP__
#define COORD_OPERATOR_HPP__

#include "pmdEDU.hpp"
#include "coordGroupHandle.hpp"
#include "coordRemoteHandle.hpp"
#include "rtnContextBuff.hpp"
#include "utilResult.hpp"

#include "../bson/bson.h"

using namespace bson ;
using namespace std ;

namespace engine
{

   /*
      COORD_REPLY_PROCESS_TYPE define
   */
   enum COORD_REPLY_PROCESS_TYPE
   {
      COORD_PROCESS_OK              = 0,
      COORD_PROCESS_IGNORE,
      COORD_PROCESS_NOK
   } ;

   /*
      coordSendMsgIn define
   */
   struct coordSendMsgIn
   {
      MsgHeader                  *_pMsg ;
      GROUP_2_IOVEC              _datas ;

      coordSendMsgIn( MsgHeader *msg )
      {
         _pMsg    = msg ;
      }

      MsgHeader *msg()
      {
         return _pMsg ;
      }
      GROUP_2_IOVEC *data()
      {
         return &_datas ;
      }
      BOOLEAN hasData() const
      {
         if ( _datas.size() > 0 )
         {
            return TRUE ;
         }
         return FALSE ;
      }
      void resetHeader( pmdEDUCB *cb )
      {
         _pMsg->routeID.value = MSG_INVALID_ROUTEID ;
         _pMsg->TID = cb->getTID() ;
      }
      INT32 opCode()
      {
         return _pMsg->opCode ;
      }
   } ;

   /*
      coordSendOptions define
   */
   struct coordSendOptions
   {
      BOOLEAN                    _primary ;
      MSG_ROUTE_SERVICE_TYPE     _svcType ;

      CoordGroupList             _groupLst ;
      BOOLEAN                    _useSpecialGrp ;  // use the specila group of _groupLst

      SET_RC                     *_pIgnoreRC ;

      coordSendOptions( BOOLEAN primary = FALSE,
                        MSG_ROUTE_SERVICE_TYPE type = MSG_ROUTE_SHARD_SERVCIE )
      {
         _primary    = primary ;
         _svcType    = type ;
         _pIgnoreRC  = NULL ;
         _useSpecialGrp = FALSE ;
      }

      BOOLEAN isIgnored( INT32 rc )
      {
         if ( _pIgnoreRC && _pIgnoreRC->find( rc ) != _pIgnoreRC->end() )
         {
            return TRUE ;
         }
         return FALSE ;
      }
   } ;

   /*
      coordProcessResult define
   */
   struct coordProcessResult
   {
      CoordGroupList             _sucGroupLst ;
      ROUTE_RC_MAP               *_pNokRC ;
      ROUTE_RC_MAP               *_pIgnoreRC ;
      ROUTE_RC_MAP               *_pOkRC ;

      ROUTE_REPLY_MAP            *_pNokReply ;
      ROUTE_REPLY_MAP            *_pIgnoreReply ;
      ROUTE_REPLY_MAP            *_pOkReply ;

      coordProcessResult()
      {
         _pNokRC           = NULL ;
         _pIgnoreRC        = NULL ;
         _pOkRC            = NULL ;

         _pNokReply        = NULL ;
         _pIgnoreReply     = NULL ;
         _pOkReply         = NULL ;
      }

      BOOLEAN pushNokRC( UINT64 id, const MsgOpReply *pReply )
      {
         if ( _pNokRC )
         {
            (*_pNokRC)[ id ] = coordErrorInfo( pReply ) ;
            return TRUE ;
         }
         return FALSE ;
      }
      BOOLEAN pushOkRC( UINT64 id, INT32 rc )
      {
         if ( _pOkRC )
         {
            (*_pOkRC)[ id ] = rc ;
            return TRUE ;
         }
         return FALSE ;
      }
      BOOLEAN pushIgnoreRC( UINT64 id, INT32 rc )
      {
         if ( _pIgnoreRC )
         {
            (*_pIgnoreRC)[ id ] = rc ;
            return TRUE ;
         }
         return FALSE ;
      }
      BOOLEAN pushNokReply( UINT64 id, const pmdEDUEvent &event )
      {
         if ( _pNokReply )
         {
            return _pNokReply->insert( std::make_pair( id, event ) ).second ;
         }
         return FALSE ;
      }
      BOOLEAN pushIgnoreReply( UINT64 id, const pmdEDUEvent &event )
      {
         if ( _pIgnoreReply )
         {
            return _pIgnoreReply->insert( std::make_pair( id, event ) ).second ;
         }
         return FALSE ;
      }
      BOOLEAN pushOkReply( UINT64 id, const pmdEDUEvent &event )
      {
         if ( _pOkReply )
         {
            return _pOkReply->insert( std::make_pair( id, event ) ).second ;
         }
         return FALSE ;
      }
      BOOLEAN pushReply( const pmdEDUEvent &event, INT32 processType )
      {
         MsgHeader *pMsg = ( MsgHeader* )event._Data ;
         if ( COORD_PROCESS_OK == processType )
         {
            return pushOkReply( pMsg->routeID.value, event ) ;
         }
         else if ( COORD_PROCESS_IGNORE == processType )
         {
            return pushIgnoreReply( pMsg->routeID.value, event ) ;
         }
         else if ( COORD_PROCESS_NOK == processType )
         {
            return pushNokReply( pMsg->routeID.value, event ) ;
         }
         return FALSE ;
      }
      UINT32 ignoreSize() const
      {
         return _pIgnoreRC ? (UINT32)_pIgnoreRC->size() : 0 ;
      }
      UINT32 nokSize() const
      {
         return _pNokRC ? (UINT32)_pNokRC->size() : 0 ;
      }
      UINT32 okSize() const
      {
         return _pOkRC ? (UINT32)_pOkRC->size() : 0 ;
      }
      BOOLEAN isAllOK() const
      {
         if ( 0 != nokSize() || 0 != ignoreSize() )
         {
            return FALSE ;
         }
         return TRUE ;
      }
      void clearError() ;
      void clear() ;
   } ;

   #define COORD_RET_BUILDER_DFT_SIZE           ( 128 )

   /*
      _coordOperator define
   */
   class _coordOperator : public SDBObject
   {
      friend class _coordCommandFactory ;

      public:
         _coordOperator() ;
         virtual ~_coordOperator() ;

         INT32                init( coordResource *pResource,
                                    _pmdEDUCB *cb,
                                    INT64 timeout = 0 ) ;

         INT64                getTimeout() const ;

         CoordCataInfoPtr     getCataPtr() { return _cataPtr ; }

      public:
         virtual BOOLEAN      isReadOnly() const ;
         virtual const CHAR*  getName() const ;
         virtual BOOLEAN      needRollback() const ;

         virtual utilWriteResult*  getWriteResult() { return NULL ; }

      public:
         virtual INT32        execute( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       INT64 &contextID,
                                       rtnContextBuf *buf ) = 0 ;

         virtual INT32        rollback( MsgHeader *pMsg,
                                        pmdEDUCB *cb,
                                        INT64 &contextID,
                                        rtnContextBuf *buf ) ;

         virtual INT32        doOnGroups( coordSendMsgIn &inMsg,
                                          coordSendOptions &options,
                                          pmdEDUCB *cb,
                                          coordProcessResult &result ) ;

         virtual INT32        doOpOnCL( coordCataSel &cataSel,
                                        const BSONObj &objMatch,
                                        coordSendMsgIn &inMsg,
                                        coordSendOptions &options,
                                        pmdEDUCB *cb,
                                        coordProcessResult &result ) ;

      protected:
         virtual BOOLEAN            _isTrans( pmdEDUCB *cb,
                                              MsgHeader *pMsg ) ;

         virtual void               _onNodeReply( INT32 processType,
                                                  MsgOpReply *pReply,
                                                  pmdEDUCB *cb,
                                                  coordSendMsgIn &inMsg ) ;

         virtual INT32              _prepareCLOp( coordCataSel &cataSel,
                                                  coordSendMsgIn &inMsg,
                                                  coordSendOptions &options,
                                                  pmdEDUCB *cb,
                                                  coordProcessResult &result ) ;

         virtual void               _doneCLOp( coordCataSel &cataSel,
                                               coordSendMsgIn &inMsg,
                                               coordSendOptions &options,
                                               pmdEDUCB *cb,
                                               coordProcessResult &result ) ;

         virtual INT32              _prepareMainCLOp( coordCataSel &cataSel,
                                                      coordSendMsgIn &inMsg,
                                                      coordSendOptions &options,
                                                      pmdEDUCB *cb,
                                                      coordProcessResult &result ) ;

         virtual void               _doneMainCLOp( coordCataSel &cataSel,
                                                   coordSendMsgIn &inMsg,
                                                   coordSendOptions &options,
                                                   pmdEDUCB *cb,
                                                   coordProcessResult &result ) ;

         /*
            Whether or not to interrupted all other sub sessions
            when a sub session failed( send failed or returned failed )
         */
         virtual BOOLEAN            _interruptWhenFailed() const ;

      protected:
         /*
            Check retry for collection operation
         */
         BOOLEAN              checkRetryForCLOpr( INT32 rc,
                                                  ROUTE_RC_MAP *pRC,
                                                  coordCataSel &cataSel,
                                                  MsgHeader *pSrcMsg,
                                                  pmdEDUCB *cb,
                                                  INT32 &errRC,
                                                  MsgRouteID *pNodeID = NULL,
                                                  BOOLEAN canUpdate = TRUE ) ;

         INT32                checkCatVersion(pmdEDUCB *cb,
                                              const char*pCollectionName,
                                              INT32 clientVer,
                                              coordCataSel& cataSel ) ;

      protected:
         void                 setReadOnly( BOOLEAN isReadOnly ) ;

      private:
         BOOLEAN              _isReadOnly ;

      protected:
         coordResource              *_pResource ;
         coordGroupSession          _groupSession ;
         coordGroupHandler          _groupHandler ;
         coordRemoteHandler         _remoteHandler ;
         CoordCataInfoPtr           _cataPtr ;

   } ;
   typedef _coordOperator coordOperator ;

}

#endif //COORD_OPERATOR_HPP__

