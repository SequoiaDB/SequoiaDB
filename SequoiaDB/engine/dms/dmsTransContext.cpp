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

   Source File Name = dmsTransContext.cpp

   Descriptive Name = Data Management Service Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   dms Reccord ID (RID).

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/14/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsTransContext.hpp"
#include "dmsStorageDataCommon.hpp"
#include "rtnIXScanner.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"

namespace engine
{

   /*
      _dmsScanTransContext implement
   */
   _dmsScanTransContext::_dmsScanTransContext( dmsMBContext *pMBContext,
                                               rtnScanner *pScanner,
                                               DMS_ACCESS_TYPE accessType )
   : _pMBContext( pMBContext ),
     _pScanner( pScanner ),
     _accessType( accessType ),
     _isCursorSame( TRUE )
   {
      SDB_ASSERT( pMBContext, "MB Context can't be NULL" ) ;
   }

   _dmsScanTransContext::~_dmsScanTransContext()
   {
   }

   INT32 _dmsScanTransContext::_checkAccess()
   {
      INT32 rc = SDB_OK ;

      if ( !dmsAccessAndFlagCompatiblity ( _pMBContext->mb()->_flag,
                                           _accessType ) )
      {
         PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                  _pMBContext->mb()->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
      }

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSCANTRANSCONTEXT_PAUSE, "_dmsScanTransContext::pause" )
   INT32 _dmsScanTransContext::pause()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSCANTRANSCONTEXT_PAUSE ) ;

      if ( _pScanner )
      {
         rc = _pScanner->pauseScan() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to pause scanner, rc: %d", rc ) ;
      }

      rc = _pMBContext->pause() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to pause mb context [%s], rc: %d",
                   _pMBContext->toString().c_str(), rc ) ;

   done:
      PD_TRACE_EXITRC ( SDB__DMSSCANTRANSCONTEXT_PAUSE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSCANTRANSCONTEXT_RESUME, "_dmsScanTransContext::resume" )
   INT32 _dmsScanTransContext::resume()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSCANTRANSCONTEXT_RESUME ) ;

      rc = _pMBContext->resume() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to resume mb context [%s], rc: %d",
                   _pMBContext->toString().c_str(), rc ) ;

      rc = _checkAccess() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check access, rc: %d", rc ) ;

      if ( _pScanner )
      {
         rc = _pScanner->resumeScan( _isCursorSame ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to resume scanner, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__DMSSCANTRANSCONTEXT_RESUME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}


