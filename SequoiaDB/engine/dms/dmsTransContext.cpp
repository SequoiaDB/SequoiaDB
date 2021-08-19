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
      _dmsTBTransContext implement
   */
   _dmsTBTransContext::_dmsTBTransContext( _dmsMBContext *pMBContext,
                                           DMS_ACCESS_TYPE accessType )
   {
      SDB_ASSERT( pMBContext, "MB Context can't be NULL" ) ;
      _pMBContext    = pMBContext ;
      _accessType    = accessType ;
   }

   _dmsTBTransContext::~_dmsTBTransContext()
   {
   }

   INT32 _dmsTBTransContext::_checkAccess()
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

   INT32 _dmsTBTransContext::pause()
   {
      return _pMBContext->pause() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSTBTRANSCONTEXT_RESUME, "_dmsTBTransContext::resume" )
   INT32 _dmsTBTransContext::resume()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSTBTRANSCONTEXT_RESUME ) ;

      rc = _pMBContext->resume() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Resume dms mblock[%s] failed, rc: %d",
                 _pMBContext->toString().c_str(), rc ) ;
         goto error ;
      }

      rc = _checkAccess() ;
      if ( rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__DMSTBTRANSCONTEXT_RESUME, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      _dmsIXTransContext implement
   */
   _dmsIXTransContext::_dmsIXTransContext( _dmsMBContext *pMBContext,
                                           DMS_ACCESS_TYPE accessType,
                                           _rtnIXScanner *pScanner )
   :_dmsTBTransContext( pMBContext, accessType )
   {
      SDB_ASSERT( pScanner, "Scanner can't be NULL" ) ;

      _pScanner      = pScanner ;
      _isSame        = TRUE ;
   }

   _dmsIXTransContext::~_dmsIXTransContext()
   {
   }

   INT32 _dmsIXTransContext::pause()
   {
      INT32 rc = SDB_OK ;

      _isSame = TRUE ;

      rc = _pScanner->pauseScan() ;
      if ( SDB_OK == rc )
      {
         rc = _dmsTBTransContext::pause() ;
      }

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSIXTRANSCONTEXT_RESUME, "_dmsIXTransContext::resume" )
   INT32 _dmsIXTransContext::resume()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSIXTRANSCONTEXT_RESUME ) ;

      /// first resume base
      rc = _dmsTBTransContext::resume() ;
      if ( rc )
      {
         goto error ;
      }

      /// then resume scanner
      rc = _pScanner->resumeScan( &_isSame ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Resume index scanner failed, rc: %d",
                 rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__DMSIXTRANSCONTEXT_RESUME, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _dmsIXTransContext::isCursorSame() const
   {
      return _isSame ;
   }

}


