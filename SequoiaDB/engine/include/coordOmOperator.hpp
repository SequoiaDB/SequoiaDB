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

   Source File Name = coordOmOperator.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          13/03/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef COORD_OM_OPERATOR_HPP__
#define COORD_OM_OPERATOR_HPP__

#include "coordCommandBase.hpp"

namespace engine
{

   /*
      _coordOmOperatorBase define
   */
   class _coordOmOperatorBase : public _coordCommandBase
   {
      public:
         _coordOmOperatorBase() ;
         virtual ~_coordOmOperatorBase() ;

      private:
         virtual INT32        execute( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       INT64 &contextID,
                                       rtnContextBuf *buf ) ;

      public:
         INT32         executeOnOm ( MsgHeader *pMsg,
                                     pmdEDUCB *cb,
                                     BOOLEAN onPrimary,
                                     SET_RC *pIgnoreRC = NULL,
                                     rtnContextCoord::sharePtr *ppContext = NULL,
                                     rtnContextBuf *buf = NULL ) ;

         INT32         executeOnOm ( MsgHeader *pMsg,
                                     pmdEDUCB *cb,
                                     vector<BSONObj> *pReplyObjs,
                                     BOOLEAN onPrimary = TRUE,
                                     SET_RC *pIgnoreRC = NULL,
                                     rtnContextBuf *buf = NULL ) ;

         INT32          queryOnOm( MsgHeader *pMsg,
                                   INT32 requestType,
                                   pmdEDUCB *cb,
                                   INT64 &contextID,
                                   rtnContextBuf *buf ) ;

         INT32          queryOnOm( const rtnQueryOptions &options,
                                   pmdEDUCB *cb,
                                   SINT64 &contextID,
                                   rtnContextBuf *buf ) ;

         INT32          queryOnOmAndPushToVec( const rtnQueryOptions &options,
                                               pmdEDUCB *cb,
                                               vector<BSONObj> &objs,
                                               rtnContextBuf *buf ) ;
   } ;
   typedef _coordOmOperatorBase coordOmOperatorBase ;

}

#endif //COORD_OM_OPERATOR_HPP__

