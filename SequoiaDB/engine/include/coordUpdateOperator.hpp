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

   Source File Name = coordUpdateOperator.hpp

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

#ifndef COORD_UPDATE_OPERATOR_HPP__
#define COORD_UPDATE_OPERATOR_HPP__

#include "coordTransOperator.hpp"

using namespace bson ;

namespace engine
{
   /*
      _coordUpdateOperator define
   */
   class _coordUpdateOperator : public _coordTransOperator
   {
      public:
         _coordUpdateOperator() ;
         virtual ~_coordUpdateOperator() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;

         virtual BOOLEAN      isReadOnly() const ;

         UINT32               getUpdatedNum() const ;
         UINT32               getInsertedNum() const ;
         void                 clearStat() ;

      protected:
         virtual void               _prepareForTrans( pmdEDUCB *cb,
                                                      MsgHeader *pMsg ) ;

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

         BSONObj  _buildNewSelector( const BSONObj &selector,
                                     const CoordSubCLlist &subCLList ) ;

         INT32    _upsert( const CHAR *pCollectionName,
                           const BSONObj &matcher,
                           const BSONObj &updator,
                           const BSONObj &hint,
                           BOOLEAN strictDataMode,
                           pmdEDUCB *cb,
                           UINT32 &insertNum,
                           INT64 &contextID,
                           rtnContextBuf *buf ) ;

         void     _clearBlock( pmdEDUCB *cb ) ;

      private:
         UINT32               _insertedNum ;
         vector< CHAR* >      _vecBlock ;

   } ;
   typedef _coordUpdateOperator coordUpdateOperator ;

}

#endif // COORD_UPDATE_OPERATOR_HPP__


