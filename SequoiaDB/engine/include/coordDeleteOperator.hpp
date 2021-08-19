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

   Source File Name = coordDeleteOperator.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/14/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef COORD_DELETE_OPERATOR_HPP__
#define COORD_DELETE_OPERATOR_HPP__

#include "coordTransOperator.hpp"
#include "utilInsertResult.hpp"

using namespace bson ;

namespace engine
{
   /*
      coordDeleteOperator define
   */
   class _coordDeleteOperator : public _coordTransOperator
   {
      public:
         _coordDeleteOperator() ;
         virtual ~_coordDeleteOperator() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;

         virtual BOOLEAN      isReadOnly() const ;

         UINT32      getDeletedNum() const ;
         void        clearStat() ;

      protected:
         virtual void   _prepareForTrans( pmdEDUCB *cb,
                                          MsgHeader *pMsg ) ;

         INT32          _prepareCLOp( coordCataSel &cataSel,
                                      coordSendMsgIn &inMsg,
                                      coordSendOptions &options,
                                      pmdEDUCB *cb,
                                      coordProcessResult &result ) ;

         virtual INT32  _prepareMainCLOp( coordCataSel &cataSel,
                                          coordSendMsgIn &inMsg,
                                          coordSendOptions &options,
                                          pmdEDUCB *cb,
                                          coordProcessResult &result ) ;

         virtual void   _doneMainCLOp( coordCataSel &cataSel,
                                       coordSendMsgIn &inMsg,
                                       coordSendOptions &options,
                                       pmdEDUCB *cb,
                                       coordProcessResult &result ) ;

         virtual void   _onNodeReply( INT32 processType,
                                      MsgOpReply *pReply,
                                      pmdEDUCB *cb,
                                      coordSendMsgIn &inMsg ) ;

      private:
         BSONObj        _buildNewDeletor( const BSONObj &deletor,
                                          const CoordSubCLlist &subCLList ) ;

         void           _clearBlock( pmdEDUCB *cb ) ;

         INT32          _checkDeleteOne( coordSendMsgIn &inMsg,
                                         coordSendOptions &options,
                                         CoordGroupSubCLMap *grpSubCl ) ;

      private:
         vector<CHAR*>           _vecBlock ;
         utilDeleteResult        _delResult ;

   } ;
   typedef _coordDeleteOperator coordDeleteOperator ;

}

#endif // COORD_DELETE_OPERATOR_HPP__

