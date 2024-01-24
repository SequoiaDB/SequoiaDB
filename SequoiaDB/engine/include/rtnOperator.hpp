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

   Source File Name = rtnOperator.hpp

   Descriptive Name = Base class for rtn operators.

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          14/04/2017  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_OPERATOR_HPP__
#define RTN_OPERATOR_HPP__

#include "oss.hpp"
#include "pmdEDU.hpp"
#include "rtnCB.hpp"
#include "rtnQueryOptions.hpp"

namespace engine
{
   // Base class for rtn Operators.
   class _rtnOperator : public SDBObject
   {
      public:
         _rtnOperator() ;
         virtual ~_rtnOperator() ;
      public:
         virtual INT32 run( const rtnQueryOptions &options,
                            pmdEDUCB *eduCB,
                            SDB_RTNCB *rtnCB,
                            INT64 &contextID,
                            rtnContextBase **ppContext,
                            BOOLEAN enablePrefetch ) = 0 ;
   } ;
   typedef _rtnOperator rtnOperator ;
}

#endif /* RTN_OPERATOR_HPP__ */

