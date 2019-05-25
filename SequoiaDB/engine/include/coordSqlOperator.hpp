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

