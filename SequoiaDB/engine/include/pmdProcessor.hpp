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

   Source File Name = pmdProcessor.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/12/2014  Lin Youbin  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef PMD_PROCESSOR_HPP_
#define PMD_PROCESSOR_HPP_

#include "pmd.hpp"
#include "rtnCB.hpp"
#include "dmsCB.hpp"
#include "dpsLogWrapper.hpp"
#include "pmdSession.hpp"

namespace engine
{
   /*
      _pmdDataProcessor define
   */
   class _pmdDataProcessor : public _pmdProcessorBase
   {
      public:
         _pmdDataProcessor() ;
         virtual ~_pmdDataProcessor() ;

      public:
         virtual INT32           processMsg( MsgHeader *msg,
                                             rtnContextBuf &contextBuff,
                                             INT64 &contextID,
                                             BOOLEAN &needReply,
                                             BOOLEAN &needRollback,
                                             BSONObjBuilder &builder ) ;

         virtual INT32           doRollback() ;
         virtual INT32           doCommit() ;

         virtual const CHAR*           processorName() const ;
         virtual SDB_PROCESSOR_TYPE    processorType() const ;

      protected:
         virtual void                  _onAttach () ;
         virtual void                  _onDetach () ;

      protected:
         INT32                   _onMsgReqMsg( MsgHeader * msg ) ;
         INT32                   _onUpdateReqMsg( MsgHeader * msg,
                                                  SDB_DPSCB *dpsCB,
                                                  utilUpdateResult &upResult ) ;
         INT32                   _onInsertReqMsg( MsgHeader * msg,
                                                  rtnContextBuf &buff,
                                                  utilInsertResult &inResult ) ;
         INT32                   _onQueryReqMsg( MsgHeader * msg,
                                                 SDB_DPSCB *dpsCB,
                                                 _rtnContextBuf &buffObj,
                                                 INT64 &contextID,
                                                 BOOLEAN &needRollback,
                                                 BSONObjBuilder &builder ) ;
         INT32                   _onDelReqMsg( MsgHeader * msg,
                                               SDB_DPSCB *dpsCB,
                                               utilDeleteResult &delResult ) ;
         INT32                   _onGetMoreReqMsg( MsgHeader * msg,
                                                   rtnContextBuf &buffObj,
                                                   INT64 &contextID,
                                                   BOOLEAN &needRollback ) ;
         INT32                   _onAdvanceReqMsg( MsgHeader * msg,
                                                   rtnContextBuf &buffObj,
                                                   INT64 &contextID,
                                                   BOOLEAN &needRollback ) ;
         INT32                   _onKillContextsReqMsg( MsgHeader *msg ) ;
         INT32                   _onSQLMsg( MsgHeader *msg,
                                            INT64 &contextID,
                                            SDB_DPSCB *dpsCB,
                                            BOOLEAN &needRollback,
                                            BSONObjBuilder &builder ) ;
         INT32                   _onTransBeginMsg () ;
         INT32                   _onTransCommitMsg ( SDB_DPSCB *dpsCB ) ;
         INT32                   _onTransRollbackMsg ( SDB_DPSCB *dpsCB ) ;
         INT32                   _onAggrReqMsg( MsgHeader *msg,
                                                INT64 &contextID ) ;
         INT32                   _onOpenLobMsg( MsgHeader *msg,
                                                SDB_DPSCB *dpsCB,
                                                SINT64 &contextID,
                                                rtnContextBuf &buffObj ) ;
         INT32                   _onWriteLobMsg( MsgHeader *msg ) ;
         INT32                   _onReadLobMsg( MsgHeader *msg,
                                                rtnContextBuf &buffObj ) ;
         INT32                   _onLockLobMsg( MsgHeader *msg ) ;
         INT32                   _onCloseLobMsg( MsgHeader *msg,
                                                 rtnContextBuf &buffObj ) ;
         INT32                   _onRemoveLobMsg( MsgHeader *msg,
                                                  SDB_DPSCB *dpsCB ) ;
         INT32                   _onTruncateLobMsg( MsgHeader *msg,
                                                    SDB_DPSCB *dpsCB ) ;
         INT32                   _onGetLobRTDetailMsg( MsgHeader *msg,
                                                     rtnContextBuf &buffObj ) ;
         INT32                   _onCreateLobIDMsg( MsgHeader *msg,
                                                    rtnContextBuf &buffObj ) ;
         INT32                   _onInterruptMsg( MsgHeader *msg,
                                                  SDB_DPSCB *dpsCB ) ;
         INT32                   _onInterruptSelfMsg() ;
         INT32                   _onDisconnectMsg() ;

         INT32                   _updateVCS( const CHAR *fullName,
                                             const BSONObj &updator ) ;

         INT32                   _insertVCS( const CHAR *fullName,
                                             const BSONObj &insertor ) ;

         INT32                   _deleteVCS( const CHAR *fullName,
                                             const BSONObj &deletor ) ;

      protected:
         INT32                   _checkTransOperator( BOOLEAN checkDps = TRUE,
                                                      BOOLEAN autoCommit = FALSE ) ;
         INT32                   _checkTransAutoCommit( const MsgHeader *msg,
                                                        BOOLEAN checkDps = TRUE ) ;

      protected:
         virtual INT32           _beginTrans( BOOLEAN isAutoCommit ) ;

      protected:
         _SDB_KRCB *             _pKrcb ;
         _SDB_DMSCB *            _pDMSCB ;
         _SDB_RTNCB *            _pRTNCB ;
   } ;
   typedef _pmdDataProcessor pmdDataProcessor ;

   /*
      _pmdCoordProcessor define
   */
   class _pmdCoordProcessor : public _pmdDataProcessor
   {
      public:
         _pmdCoordProcessor() ;
         virtual ~_pmdCoordProcessor() ;

      public:
         virtual INT32           processMsg( MsgHeader *msg,
                                             rtnContextBuf &contextBuff,
                                             INT64 &contextID,
                                             BOOLEAN &needReply,
                                             BOOLEAN &needRollback,
                                             BSONObjBuilder &builder ) ;

         virtual INT32           doRollback() ;
         virtual INT32           doCommit() ;

         virtual const CHAR*           processorName() const ;
         virtual SDB_PROCESSOR_TYPE    processorType() const ;

      protected:
         virtual INT32           _beginTrans( BOOLEAN isAutoCommit ) ;

      protected:
         virtual void                  _onAttach () ;
         virtual void                  _onDetach () ;

         INT32                   _onQueryReqMsg( MsgHeader *msg,
                                                 _rtnContextBuf &buffObj,
                                                 INT64 &contextID,
                                                 BOOLEAN &needRollback ) ;
      private:
         INT32                   _processCoordMsg( MsgHeader *msg,
                                                   INT64 &contextID,
                                                   rtnContextBuf &contextBuff,
                                                   BOOLEAN &needReply,
                                                   BOOLEAN &needRollback ) ;
   } ;

   typedef _pmdCoordProcessor pmdCoordProcessor ;
}

#endif  /*PMD_PROCESSOR_HPP_*/

