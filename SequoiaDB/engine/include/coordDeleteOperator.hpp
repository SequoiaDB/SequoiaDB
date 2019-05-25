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

      private:
         BSONObj        _buildNewDeletor( const BSONObj &deletor,
                                          const CoordSubCLlist &subCLList ) ;

         void           _clearBlock( pmdEDUCB *cb ) ;

      private:
         vector<CHAR*>           _vecBlock ;

   } ;
   typedef _coordDeleteOperator coordDeleteOperator ;

}

#endif // COORD_DELETE_OPERATOR_HPP__

