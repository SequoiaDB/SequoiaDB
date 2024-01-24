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

   Source File Name = rtnScannerChecker.cpp

   Descriptive Name = RunTime Scanner Checker

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/26/2022  HGM first init

   Last Changed =

*******************************************************************************/

#include "rtnScannerChecker.hpp"
#include "rtnCB.hpp"
#include "rtnContextData.hpp"

namespace engine
{

   /*
      _rtnScannerChecker implement
    */
   _rtnScannerChecker::_rtnScannerChecker( pmdEDUCB * cb )
   : _eduCB( cb ),
     _contextID( -1 )
   {
   }

   _rtnScannerChecker::~_rtnScannerChecker()
   {
      _release() ;
   }

   INT32 _rtnScannerChecker::open( UINT32 suLID,
                                   UINT32 mbLID,
                                   const CHAR *csName,
                                   const CHAR *clShortName,
                                   const CHAR *optrDesc )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( -1 == _contextID, "should be not opened" ) ;

      _release() ;

      rc = _open( suLID, mbLID, csName, clShortName, optrDesc ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open checker for "
                   "collection [%s.%s], rc: %d", csName, clShortName, rc ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

   void _rtnScannerChecker::release()
   {
      _release() ;
   }

   BOOLEAN _rtnScannerChecker::needInterrupt()
   {
      BOOLEAN result = FALSE ;

      if ( NULL != _eduCB )
      {
         _eduCB->checkUrgentEvents() ;
      }

      // check if context still exists ( check EDU thread cache is enough )
      if ( ( -1 != _contextID ) &&
           ( !( _eduCB->contextFind( _contextID ) ) ) )
      {
         result = TRUE ;
         _contextID = -1 ;
      }

      return result ;
   }

   INT32 _rtnScannerChecker::_open( UINT32 suLID,
                                    UINT32 mbLID,
                                    const CHAR *csName,
                                    const CHAR *clShortName,
                                    const CHAR *optrDesc )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( DMS_INVALID_LOGICCSID != suLID,
                  "storage unit logical ID is invalid" ) ;
      SDB_ASSERT( DMS_INVALID_LOGICCLID != mbLID,
                  "metadata block logical ID is invalid" ) ;
      SDB_ASSERT( NULL != csName, "collection space name is invalid" ) ;
      SDB_ASSERT( NULL != clShortName, "collection short name is invalid" ) ;

      rtnContextTemp::sharePtr context ;
      rc = sdbGetRTNCB()->contextNew( RTN_CONTEXT_TEMP, context,
                                      _contextID, _eduCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create temp context for collection "
                   "[%s.%s], rc: %d", csName, clShortName, rc ) ;


      rc = context->open( suLID, mbLID, csName, clShortName, optrDesc ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open temp context for collection "
                   "[%s.%s], rc: %d", csName, clShortName, rc ) ;

   done:
      return rc ;

   error:
      _release() ;
      goto done ;
   }

   void _rtnScannerChecker::_release()
   {
      if ( -1 != _contextID )
      {
         sdbGetRTNCB()->contextDelete( _contextID, _eduCB ) ;
         _contextID = -1 ;
      }
   }

   /*
      _rtnScannerCheckerCreator implement
    */
   INT32 _rtnScannerCheckerCreator::createChecker( UINT32 suLID,
                                                   UINT32 mbLID,
                                                   const CHAR *csName,
                                                   const CHAR *clShortName,
                                                   const CHAR *optrDesc,
                                                   pmdEDUCB *cb,
                                                   IDmsScannerChecker **ppChecker )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( NULL != ppChecker, "checker pointer should be valid" ) ;

      rtnScannerChecker *checker = SDB_OSS_NEW rtnScannerChecker( cb ) ;
      PD_CHECK( NULL != checker, SDB_OOM, error, PDERROR,
                "Failed to allocate scanner checker" ) ;

      rc = checker->open( suLID, mbLID, csName, clShortName, optrDesc ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open scanner checker, rc: %d",
                   rc ) ;

      *ppChecker = checker ;

   done:
      return rc ;

   error:
      SAFE_OSS_DELETE( checker ) ;
      goto done ;
   }

   void _rtnScannerCheckerCreator::releaseChecker( IDmsScannerChecker *pChecker )
   {
      if ( NULL != pChecker )
      {
         rtnScannerChecker *tmpChecker =
                           dynamic_cast< rtnScannerChecker * >( pChecker ) ;
         if ( NULL != tmpChecker )
         {
            tmpChecker->release() ;
         }
         else
         {
            SDB_ASSERT( FALSE, "should be a runtime scanner checker" ) ;
         }
         SDB_OSS_DEL pChecker ;
      }
   }

}
