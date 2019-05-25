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

   Source File Name = coordCommand2Phase.hpp

   Descriptive Name = Coord Commands

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2017  XJH Init
   Last Changed =

*******************************************************************************/

#ifndef COORD_COMMAND_2PHASE_HPP__
#define COORD_COMMAND_2PHASE_HPP__

#include "coordCommandBase.hpp"
#include "coordFactory.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordCMDArguments define
   */
   class _coordCMDArguments : public SDBObject
   {
      public :
         _coordCMDArguments () { _pBuf = NULL ; }

         virtual ~_coordCMDArguments () {}

         /* A copy of the query object */
         BSONObj _boQuery ;

         /* Name of the catalog target to be updated */
         string _targetName ;

         /* ignore error return codes */
         SET_RC _ignoreRCList ;

         /* the return context buf pointer */
         rtnContextBuf *_pBuf ;
   } ;
   typedef _coordCMDArguments coordCMDArguments ;

   /*
    * _coordCMD2Phase define
    */
   class _coordCMD2Phase : public _coordCommandBase
   {
      public:
         _coordCMD2Phase() ;
         virtual ~_coordCMD2Phase() ;

      public:
         virtual INT32 execute ( MsgHeader *pMsg,
                                 pmdEDUCB *cb,
                                 INT64 &contextID,
                                 rtnContextBuf *buf ) ;
      protected :
         virtual coordCMDArguments *_getArguments () ;

         virtual INT32 _extractMsg ( MsgHeader *pMsg,
                                     coordCMDArguments *pArgs ) ;

         virtual INT32 _parseMsg ( MsgHeader *pMsg,
                                   coordCMDArguments *pArgs ) = 0 ;

         virtual INT32 _generateCataMsg ( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          coordCMDArguments *pArgs,
                                          CHAR **ppMsgBuf,
                                          INT32 *pBufSize ) = 0 ;

         virtual void  _releaseCataMsg( CHAR *pMsgBuf,
                                        INT32 bufSize,
                                        pmdEDUCB *cb ) = 0 ;

         virtual INT32 _generateDataMsg ( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          coordCMDArguments *pArgs,
                                          const vector<BSONObj> &cataObjs,
                                          CHAR **ppMsgBuf,
                                          INT32 *pBufSize ) = 0 ;

         virtual void  _releaseDataMsg( CHAR *pMsgBuf,
                                        INT32 bufSize,
                                        pmdEDUCB *cb ) = 0 ;

         virtual INT32 _generateRollbackDataMsg ( MsgHeader *pMsg,
                                                  pmdEDUCB *cb,
                                                  coordCMDArguments *pArgs,
                                                  CHAR **ppMsgBuf,
                                                  INT32 *pBufSize ) = 0 ;

         virtual void  _releaseRollbackDataMsg( CHAR *pMsgBuf,
                                                INT32 bufSize,
                                                pmdEDUCB *cb ) = 0 ;

      protected :
         virtual INT32 _doOnCataGroup ( MsgHeader *pMsg,
                                        pmdEDUCB *cb,
                                        rtnContextCoord **ppContext,
                                        coordCMDArguments *pArgs,
                                        CoordGroupList *pGroupLst,
                                        vector<BSONObj> *pReplyObjs ) ;

         virtual INT32 _doOnDataGroup ( MsgHeader *pMsg,
                                        pmdEDUCB *cb,
                                        rtnContextCoord **ppContext,
                                        coordCMDArguments *pArgs,
                                        const CoordGroupList &groupLst,
                                        const vector<BSONObj> &cataObjs,
                                        CoordGroupList &sucGroupLst ) = 0 ;

         virtual INT32 _doOnCataGroupP2 ( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          rtnContextCoord **ppContext,
                                          coordCMDArguments *pArgs,
                                          const CoordGroupList &pGroupLst ) ;

         virtual INT32 _doOnDataGroupP2 ( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          rtnContextCoord **ppContext,
                                          coordCMDArguments *pArgs,
                                          const CoordGroupList &groupLst,
                                          const vector<BSONObj> &cataObjs ) ;

         virtual INT32 _rollbackOnDataGroup ( MsgHeader *pMsg,
                                              pmdEDUCB *cb,
                                              coordCMDArguments *pArgs,
                                              const CoordGroupList &groupLst ) ;

         virtual INT32 _doCommit ( MsgHeader *pMsg,
                                   pmdEDUCB * cb,
                                   rtnContextCoord **ppContext,
                                   coordCMDArguments *pArgs ) ;

         virtual INT32 _doComplete ( MsgHeader *pMsg,
                                     pmdEDUCB * cb,
                                     coordCMDArguments *pArgs ) ;

         virtual INT32 _doAudit ( coordCMDArguments *pArgs, INT32 rc ) = 0 ;

         virtual INT32 _processContext ( pmdEDUCB *cb,
                                         rtnContextCoord **ppContext,
                                         SINT32 maxNumSteps ) ;

      protected :
         /*
            Disable preRead in Coord context (send GetMore in advanced) 
            to control the sub-context from Catalog or Data step by step
         */
         virtual BOOLEAN _flagCoordCtxPreRead () { return FALSE ; }

         /*
            Get list of Data Groups from Catalog with the P1 catalog command
         */
         virtual BOOLEAN _flagGetGrpLstFromCata () { return FALSE ; }

         /*
            Commit on Catalog when rollback on Data groups failed
         */
         virtual BOOLEAN _flagCommitOnRollbackFailed () { return FALSE ; }

         /*
            Rollback on Catalog before rollback on Data groups
         */
         virtual BOOLEAN _flagRollbackCataBeforeData () { return FALSE ; }

      protected:
         coordCMDArguments             _args ;

   } ;
   typedef _coordCMD2Phase coordCMD2Phase ;

}

#endif // COORD_COMMAND_2PHASE_HPP__
