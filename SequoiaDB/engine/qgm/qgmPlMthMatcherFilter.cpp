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

   Source File Name = qgmPIMthMatcherFilter.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/1/2014  ly  Initial Draft

   Last Changed =

*******************************************************************************/

#include "qgmPlMthMatcherFilter.hpp"
#include "pdTrace.hpp"
#include "qgmTrace.hpp"

using namespace bson;

namespace engine
{
   qgmPlMthMatcherFilter::qgmPlMthMatcherFilter( const qgmOPFieldVec &selector,
                                                INT64 numSkip,
                                                INT64 numReturn,
                                                const qgmField &alias )
   :_qgmPlFilter( selector, NULL, numSkip, numReturn, alias )
   {
   }

   INT32 qgmPlMthMatcherFilter::loadPattern( bson::BSONObj matcher )
   {
      return _matcher.loadPattern( matcher );
   }

   //PD_TRACE_DECLARE_FUNCTION( SDB__QGMPLMTHMATCHERFILTER__FETCHNEXT, "qgmPlMthMatcherFilter::_fetchNext" )
   INT32 qgmPlMthMatcherFilter::_fetchNext( qgmFetchOut & next )
   {
      PD_TRACE_ENTRY( SDB__QGMPLMTHMATCHERFILTER__FETCHNEXT ) ;
      INT32 rc = SDB_OK ;
      qgmFetchOut fetch ;
      _qgmPlan *in = input( 0 ) ;

      if ( 0 <= _return && _return <= _currentReturn )
      {
         close() ;
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      while ( TRUE )
      {
         rc = in->fetchNext( fetch ) ;
         if ( SDB_OK != rc && SDB_DMS_EOC != rc )
         {
            goto error ;
         }
         else if ( SDB_DMS_EOC == rc )
         {
            break ;
         }
         else
         {
         }

         BOOLEAN r = FALSE ;
         rc = _matcher.matches( fetch.obj, r );
         if ( rc != SDB_OK )
         {
            goto error;
         }
         else if ( !r )
         {
            continue;
         }
         else
         {
         }

         if ( 0 < _skip && ++_currentSkip <= _skip )
         {
            continue ;
         }

         if ( !_selector.empty() )
         {
            rc = _selector.select( fetch,
                                   next.obj ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
         }
         else
         {
            next.obj = fetch.mergedObj() ;
         }

         if ( !_merge )
         {
            next.alias = _alias.empty()?
                         fetch.alias : _alias ;
         }

         ++_currentReturn ;
         break ;
      }
   done:
      PD_TRACE_EXITRC( SDB__QGMPLMTHMATCHERFILTER__FETCHNEXT, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}
