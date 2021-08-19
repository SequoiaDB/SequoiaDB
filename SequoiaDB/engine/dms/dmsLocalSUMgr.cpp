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

   Source File Name = dmsLocalSUMgr.cpp

   Descriptive Name = DMS Local Storage Unit Management

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains code logic for
   temporary table creation and release.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsLocalSUMgr.hpp"
#include "dmsStorageUnit.hpp"
#include "dmsCB.hpp"
#include "rtn.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"

using namespace bson ;

namespace engine
{

   /*
      _dmsLocalSUMgr implement
   */
   _dmsLocalSUMgr::_dmsLocalSUMgr( _SDB_DMSCB *dmsCB )
   :_dmsSysSUMgr( dmsCB )
   {
   }

   _dmsLocalSUMgr::~_dmsLocalSUMgr()
   {
      fini() ;
   }

   INT32 _dmsLocalSUMgr::init()
   {
      INT32 rc = SDB_OK ;
      _pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      dmsStorageUnitID suID = DMS_INVALID_CS ;

      SDB_ASSERT ( _dmsCB, "dmsCB can't be NULL" ) ;
      SDB_ASSERT ( cb, "cb can't be NULL" ) ;

      if ( SDB_ROLE_DATA != pmdGetDBRole() )
      {
         goto done ;
      }

      // first to load collection space
      rc = rtnCollectionSpaceLock( DMS_SYSLOCAL_CS_NAME, _dmsCB, TRUE,
                                   &_su, suID ) ;
      if ( SDB_DMS_CS_NOTEXIST == rc )
      {
         // create new SYSSTAT collection space
         rc = rtnCreateCollectionSpaceCommand ( DMS_SYSLOCAL_CS_NAME, NULL,
                                                _dmsCB, NULL,
                                                UTIL_UNIQUEID_NULL,
                                                DMS_PAGE_SIZE_MAX,
                                                DMS_DO_NOT_CREATE_LOB,
                                                DMS_STORAGE_NORMAL, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to create %s collection "
                      "space, rc: %d", DMS_SYSLOCAL_CS_NAME, rc ) ;
   
         rc = rtnCollectionSpaceLock ( DMS_SYSLOCAL_CS_NAME, _dmsCB, TRUE,
                                       &_su, suID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to lock %s collection space, "
                      "rc: %d", DMS_SYSLOCAL_CS_NAME, rc ) ;
      }
      else if ( SDB_OK != rc )
      {
         PD_RC_CHECK( rc, PDERROR, "Failed to lock collection space [%s], "
                      "rc: %d", DMS_SYSLOCAL_CS_NAME, rc ) ;
      }

      _su->data()->setTransSupport( FALSE ) ;

      _dmsCB->suUnlock( suID ) ;
      suID = DMS_INVALID_CS ;

      rc = _ensureMetadata( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to ensure meta-data, rc: %d", rc ) ;

   done :
      if ( DMS_INVALID_CS != suID )
      {
         _dmsCB->suUnlock ( suID ) ;
      }
      return rc ;
   error :
      goto done ;
   }

   INT32 _dmsLocalSUMgr::_ensureMetadata ( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      rc = rtnTestAndCreateCL( DMS_SYSLOCALTASK_CL_NAME, cb, _dmsCB, NULL,
                               TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create collection [%s], rc: %d",
                   DMS_SYSLOCALTASK_CL_NAME, rc ) ;

   done :
      return rc ;
   error :
      goto done ;
   }

   void _dmsLocalSUMgr::fini()
   {
   }

   INT32 _dmsLocalSUMgr::addTask( const BSONObj &obj,
                                  _pmdEDUCB *cb,
                                  _dpsLogWrapper *dpsCB,
                                  INT16 w,
                                  BSONObj &retIDObj )
   {
      INT32 rc = SDB_OK ;
      utilInsertResult insertResult ;

      insertResult.enableReturnIDInfo() ;

      rc = rtnInsert( DMS_SYSLOCALTASK_CL_NAME, obj, 1, 0, cb,
                      _dmsCB, dpsCB, w, &insertResult ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Insert object(%s) to %s failed, rc: %d",
                 obj.toString().c_str(),
                 DMS_SYSLOCALTASK_CL_NAME, rc ) ;
         goto error ;
      }

      retIDObj = insertResult.getReturnIDObj() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsLocalSUMgr::removeTask( const BSONObj &matcher,
                                     _pmdEDUCB *cb,
                                     _dpsLogWrapper *dpsCB,
                                     INT16 w )
   {
      INT32 rc = SDB_OK ;

      rc = rtnDelete( DMS_SYSLOCALTASK_CL_NAME, matcher, BSONObj(), 0,
                      cb, _dmsCB, dpsCB, w, NULL ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Delete object(%s) from %s failed, rc: %d",
                 matcher.toString().c_str(),
                 DMS_SYSLOCALTASK_CL_NAME, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsLocalSUMgr::queryTask( const BSONObj &matcher,
                                    ossPoolVector< BSONObj > &vecObj,
                                    _SDB_RTNCB *rtnCB,
                                    _pmdEDUCB *cb,
                                    INT64 limit )
   {
      INT32 rc = SDB_OK ;
      INT64 contextID = -1 ;

      rc = rtnQuery( DMS_SYSLOCALTASK_CL_NAME,
                   /* selector  matcher  orderby    hint */
                     BSONObj(), matcher, BSONObj(), BSONObj(),
                   /* flag  skip limit */
                     0, cb, 0, limit,
                     _dmsCB, rtnCB,
                     contextID,
                     NULL, FALSE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Query object(%s) from %s failed, rc: %d",
                 matcher.toString().c_str(),
                 DMS_SYSLOCALTASK_CL_NAME, rc ) ;
         goto error ;
      }

      /// get more
      try
      {
         while( TRUE )
         {
            rtnContextBuf buffObj ;
            rc = rtnGetMore( contextID, 1, buffObj, cb, rtnCB ) ;
            if ( SDB_DMS_EOC == rc )
            {
               /// hint end
               contextID = -1 ;
               rc = SDB_OK ;
               break ;
            }
            else if ( rc )
            {
               contextID = -1 ;
               PD_LOG( PDERROR, "Getmore object(%s) from %s failed, rc: %d",
                       matcher.toString().c_str(),
                       DMS_SYSLOCALTASK_CL_NAME, rc ) ;
               goto error ;
            }

            vecObj.push_back( BSONObj( buffObj.data() ).getOwned() ) ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_OOM ;
         goto error ;
      }

   done:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete( contextID, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsLocalSUMgr::countTask( const BSONObj &matcher,
                                    _SDB_RTNCB *rtnCB,
                                    _pmdEDUCB *cb,
                                    INT64 &count )
   {
      INT32 rc = SDB_OK ;
      rtnQueryOptions options ;
      options._query = matcher ;
      options.setCLFullName( DMS_SYSLOCALTASK_CL_NAME ) ;

      rc = rtnGetCount( options, _dmsCB, cb, rtnCB, &count ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Count object(%s) from %s failed, rc: %d",
                 matcher.toString().c_str(),
                 DMS_SYSLOCALTASK_CL_NAME, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _dmsLocalSUMgr::getLocalTaskCLName() const
   {
      return DMS_SYSLOCALTASK_CL_NAME ;
   }

}

