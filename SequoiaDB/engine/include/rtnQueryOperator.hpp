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

   Source File Name = rtnQueryOperator.hpp

   Descriptive Name = rtn query operator.

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
#ifndef RTN_QUERY_OPERATOR__
#define RTN_QUERY_OPERATOR__

#include "rtnOperator.hpp"

namespace engine
{
   // Operator for query with text search.
   class _rtnTSQueryOperator : public _rtnOperator
   {
      public:
         _rtnTSQueryOperator() ;
         virtual ~_rtnTSQueryOperator() ;

         INT32 run( const rtnQueryOptions &options,
                    pmdEDUCB *eduCB,
                    SDB_RTNCB *rtnCB,
                    INT64 &contextID,
                    rtnContextBase **ppContext,
                    BOOLEAN enablePrefetch ) ;
   } ;
   typedef _rtnTSQueryOperator rtnTSQueryOperator ;
}

#endif /* RTN_QUERY_OPERATOR__ */

