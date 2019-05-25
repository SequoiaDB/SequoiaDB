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

   Source File Name = coordQueryOperator.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/17/2017  XJH  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef COORD_QUERY_OPERATOR_HPP__
#define COORD_QUERY_OPERATOR_HPP__

#include "coordOperator.hpp"
#include "coordContext.hpp"

using namespace bson ;

namespace engine
{

   /*
      coordQueryConf define
   */
   struct coordQueryConf
   {
      string      _realCLName ;        // for command as 'drop cl' and so on
      BOOLEAN     _updateAndGetCata ;  // update catalog before first get version

      BOOLEAN     _openEmptyContext ;  // open context without sel & orderby ...
      BOOLEAN     _allCataGroups ;     // send to all catalog info groups,

      coordQueryConf()
      {
         _updateAndGetCata = FALSE ;
         _openEmptyContext = FALSE ;
         _allCataGroups    = FALSE ;
      }
   } ;

   /*
      _coordQueryOperator define
   */
   class _coordQueryOperator : public _coordOperator
   {
      public:
         _coordQueryOperator( BOOLEAN readOnly = TRUE ) ;
         virtual ~_coordQueryOperator() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;

      public:

         INT32                queryOrDoOnCL( MsgHeader *pMsg,
                                             pmdEDUCB *cb,
                                             rtnContextCoord **pContext,
                                             coordSendOptions &sendOpt,
                                             coordQueryConf *pQueryConf = NULL,
                                             rtnContextBuf *buf = NULL ) ;

         INT32                queryOrDoOnCL( MsgHeader *pMsg,
                                             pmdEDUCB *cb,
                                             rtnContextCoord **pContext,
                                             coordSendOptions &sendOpt,
                                             CoordGroupList &sucGrpLst,
                                             coordQueryConf *pQueryConf = NULL,
                                             rtnContextBuf *buf = NULL ) ;

      protected:
         INT32                _queryOrDoOnCL( MsgHeader *pMsg,
                                              pmdEDUCB *cb,
                                              rtnContextCoord **pContext,
                                              coordSendOptions &sendOpt,
                                              CoordGroupList *pSucGrpLst = NULL,
                                              coordQueryConf *pQueryConf = NULL,
                                              rtnContextBuf *buf = NULL ) ;

      private:

         INT32    _buildNewMsg( const CHAR *msg,
                                const BSONObj *newSelector,
                                const BSONObj *newHint,
                                CHAR *&newMsg,
                                INT32 &buffSize,
                                IExecutor *cb ) ;

         BSONObj  _buildNewQuery( const BSONObj &query,
                                  const CoordSubCLlist &subCLList ) ;

         INT32    _checkQueryModify( coordSendMsgIn &inMsg,
                                     coordSendOptions &options,
                                     CoordGroupSubCLMap *grpSubCl ) ;

         void     _optimize( coordSendMsgIn &inMsg,
                             coordSendOptions &options,
                             coordProcessResult &result ) ;

         BOOLEAN  _isUpdate( const BSONObj &hint, INT32 flags ) ;

         INT32    _generateNewHint( const CoordCataInfoPtr &cataInfo,
                                    const BSONObj &matcher,
                                    const BSONObj &hint,
                                    BSONObj &newHint,
                                    BOOLEAN &isChanged,
                                    BOOLEAN &isEmpty,
                                    pmdEDUCB *cb,
                                    BOOLEAN keepShardingKey ) ;

         void     _clearBlock( pmdEDUCB *cb ) ;

      protected:
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

      private:
         rtnContextCoord            *_pContext ;
         INT32                      _processRet ;
         vector<CHAR*>              _vecBlock ;
         BOOLEAN                    _readOnly ;

   } ;
   typedef _coordQueryOperator coordQueryOperator ;

}

#endif // COORD_QUERY_OPERATOR_HPP__

