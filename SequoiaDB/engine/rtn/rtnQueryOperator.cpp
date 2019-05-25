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

   Source File Name = rtnQueryOperator.cpp

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
#include "rtnContextTS.hpp"
#include "rtnQueryOperator.hpp"

namespace engine
{
   _rtnTSQueryOperator::_rtnTSQueryOperator()
   {
   }

   _rtnTSQueryOperator::~_rtnTSQueryOperator()
   {
   }

   INT32 _rtnTSQueryOperator::run( const rtnQueryOptions &options,
                                   pmdEDUCB *eduCB,
                                   SDB_RTNCB *rtnCB,
                                   INT64 &contextID,
                                   rtnContextBase **ppContext,
                                   BOOLEAN enablePrefetch )
   {
      INT32 rc = SDB_OK ;
      rtnContextTS *pContext = NULL ;

      rc = rtnCB->contextNew( RTN_CONTEXT_TS, (rtnContext **)&pContext,
                              contextID, eduCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Create text search context failed[ %d ]",
                   rc ) ;

      rc = pContext->open( options, eduCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Open text search query context failed[ %d ]",
                   rc ) ;

      if ( ppContext )
      {
         *ppContext = pContext ;
      }

      if ( enablePrefetch )
      {
         pContext->enablePrefetch( eduCB ) ;
      }

   done:
      return rc ;
   error:
      if ( pContext )
      {
         rtnCB->contextDelete( contextID, eduCB ) ;
      }
      goto done ;
   }
}

