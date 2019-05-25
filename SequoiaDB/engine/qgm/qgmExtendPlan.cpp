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

   Source File Name = qgmExtendPlan.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

******************************************************************************/

#include "qgmExtendPlan.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "qgmTrace.hpp"

namespace engine
{
#define CLEAR_WHEN_FAILED\
        do\
        {\
           QGM_EXTEND_TABLE::iterator itr = _table.begin() ;\
           for ( ; itr != _table.end(); ++itr )\
           {\
              if ( _local != itr->second )\
              {\
                 SAFE_OSS_DELETE( itr->second ) ;\
              }\
           }\
        }\
        while ( 0 )

   _qgmExtendPlan::_qgmExtendPlan()
   :_local( NULL ),
    _localID( 0 )
   {

   }

   _qgmExtendPlan::~_qgmExtendPlan()
   {
      _local = NULL ;
      _table.clear() ;
   }

   PD_TRACE_DECLARE_FUNCTION( SDB__QGMEXTENDPLAN_EXTEND, "_qgmExtendPlan::extend" )
   INT32 _qgmExtendPlan::extend( _qgmOptiTreeNode *&extended )
   {
      PD_TRACE_ENTRY( SDB__QGMEXTENDPLAN_EXTEND ) ;
      INT32 rc = SDB_OK ;
      _qgmOptiTreeNode *parent = NULL ;

      if ( _table.empty() || NULL == _local )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "initialize has not been done." ) ;
         goto error ;
      }
      {
      QGM_EXTEND_TABLE::iterator itr = _table.begin() ;
      for ( ; itr != _table.end(); ++itr )
      {
         if ( NULL == itr->second )
         {
            rc = _extend( itr->first, itr->second ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to extend node, id:%d",
                       itr->first ) ;
               goto error ;
            }
         }
         else
         {
            SDB_ASSERT( _aliases.size() > 0 , "impossible" ) ;
            itr->second->_alias = _aliases.front() ;
         }

         if ( NULL != parent )
         {
            if ( parent->hasChildren() )
            {
               SDB_ASSERT( 1 == parent->getSubNodeCount(), "impossible" ) ;
               _qgmOptiTreeNode *child = parent->getSubNode( 0 ) ;
               parent->_children.at( 0 ) = itr->second ;
               itr->second->_children.push_back( child ) ;
            }
            else
            {
               parent->_children.push_back( itr->second ) ;
            }
         }

         parent = itr->second ;
      }

      extended = _table.begin()->second ;
      }
   done:
      PD_TRACE_EXITRC( SDB__QGMEXTENDPLAN_EXTEND, rc ) ;
      return rc ;
   error:
      CLEAR_WHEN_FAILED ;
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION( SDB__QGMEXTENDPLAN_INSERTPLAN, "_qgmExtendPlan::insertPlan" )
   INT32 _qgmExtendPlan::insertPlan( UINT32 id, qgmOptiTreeNode *ex )
   {
      PD_TRACE_ENTRY( SDB__QGMEXTENDPLAN_INSERTPLAN ) ;
      INT32 rc = SDB_OK ;

      if ( !(_table.insert(std::make_pair(id, ex) ).second) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( NULL != ex )
      {
         SDB_ASSERT( NULL == _local, "impossible" ) ;
         _local = ex ;
         _localID = id ;
      }
   done:
      PD_TRACE_EXITRC( SDB__QGMEXTENDPLAN_INSERTPLAN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

}
