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

   Source File Name = coordSqlOperator.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/02/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef COORD_SQL_OPERATOR_HPP_
#define COORD_SQL_OPERATOR_HPP_

#include "coordOperator.hpp"

namespace engine
{
   /*
      _coordSqlOperator define
   */
   class _coordSqlOperator : public _coordOperator
   {
      public:
         _coordSqlOperator() ;
         virtual ~_coordSqlOperator() ;

         virtual const CHAR* getName() const ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;

         virtual BOOLEAN      needRollback() const ;

      public:
         BOOLEAN        _needRollback ;

   } ;
   typedef _coordSqlOperator coordSqlOperator ;

}

#endif // COORD_SQL_OPERATOR_HPP_

