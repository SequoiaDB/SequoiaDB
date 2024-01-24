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

   Source File Name = mthSAction.cpp

   Descriptive Name = mth selector action

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "mthSAction.hpp"
#include "mthTrace.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"

namespace engine
{
   _mthSAction::_mthSAction()
   : _mthMatchTreeHolder(),
     _buildFunc( NULL ),
     _getFunc( NULL ),
     _name( NULL ),
     _attribute( MTH_S_ATTR_NONE ),
     _strictDataMode( FALSE )
   {
      _matchTargetBob.reset() ;
   }

   _mthSAction::~_mthSAction()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSACTION_BUILD, "_mthSAction::build" )
   INT32 _mthSAction::build( const CHAR *fieldName,
                             const bson::BSONElement &e,
                             bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSACTION_BUILD ) ;
      if ( NULL == _buildFunc )
      {
         goto done ;
      }

      rc = ( *_buildFunc )( fieldName, e, this, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to build column:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHSACTION_BUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSACTION_GET, "_mthSAction::get" )
   INT32 _mthSAction::get( const CHAR *fieldName,
                           const bson::BSONElement &in,
                           bson::BSONElement &out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSACTION_GET ) ;
      if ( NULL == _getFunc )
      {
         goto done ;
      }

      rc = ( *_getFunc )( fieldName, in, this, out ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get column:%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHSACTION_GET, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

