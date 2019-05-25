/*******************************************************************************


   Copyright (C) 2011-2017 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

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

