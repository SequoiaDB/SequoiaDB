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

   Source File Name = dmsTempSUMgr.cpp

   Descriptive Name = DMS Temp Storage Unit Management

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

#include "dmsTempSUMgr.hpp"
#include "../bson/bson.h"
#include "dmsStorageUnit.hpp"
#include "rtn.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "pmd.hpp"
#include "dmsTmpBlkUnit.hpp"

#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

using namespace bson ;
namespace fs = boost::filesystem ;

namespace engine
{
   _dmsTempSUMgr::_dmsTempSUMgr ( SDB_DMSCB *dmsCB ) : _dmsSysSUMgr( dmsCB )
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSTMPSUMGR_INIT, "_dmsTempSUMgr::init" )
   INT32 _dmsTempSUMgr::init ()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSTMPSUMGR_INIT ) ;
      dmsStorageUnitID suID = DMS_INVALID_CS ;

      SDB_ASSERT ( _dmsCB, "dmsCB can't be NULL" ) ;

      CHAR tempName[15] = {0} ;
      UINT16 collectionID = DMS_INVALID_MBID ;

      DMSSYSSUMGR_XLOCK

      rc = rtnCollectionSpaceLock( SDB_DMSTEMP_NAME, _dmsCB, TRUE,
                                   &_su, suID ) ;
      if ( SDB_OK == rc )
      {
         _dmsCB->suUnlock( suID ) ;
         suID = DMS_INVALID_CS ;
         _su = NULL ;
         _dmsCB->dropCollectionSpace( SDB_DMSTEMP_NAME, NULL, NULL ) ;
      }
      else if ( SDB_DMS_CS_NOTEXIST != rc )
      {
         PD_LOG( PDERROR, "Lock temp collection space failed, rc: %d", rc ) ;
      }

      rc = rtnCreateCollectionSpaceCommand ( SDB_DMSTEMP_NAME, NULL, _dmsCB,
                                             NULL, DMS_PAGE_SIZE_MAX,
                                             DMS_DO_NOT_CREATE_LOB,
                                             DMS_STORAGE_NORMAL, TRUE ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to create temp collectionspace, rc: %d",
                  rc ) ;
         goto error ;
      }

      rc = rtnCollectionSpaceLock ( SDB_DMSTEMP_NAME, _dmsCB, TRUE,
                                    &_su, suID ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to get collection space and lock for %s, "
                  "rc: %d", SDB_DMSTEMP_NAME, rc ) ;
         goto error ;
      }

      for ( INT16 i = 0 ; i < DMS_MME_SLOTS ; ++i )
      {
         ossSnprintf ( tempName, sizeof(tempName), DMS_TEMP_NAME_PATTERN,
                       SDB_DMSTEMP_NAME, i ) ;
         rc = _su->data()->addCollection ( tempName, &collectionID,
                                           DMS_MB_ATTR_NOIDINDEX, NULL,
                                           NULL, 0, TRUE ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add temp collection %s, rc: %d",
                     tempName, rc ) ;
            goto error ;
         }
         SDB_ASSERT ( collectionID == i, "Invalid collectionID" ) ;
         _freeCollections.push( collectionID ) ;
      }

      rc = _initTmpPath() ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done :
      if ( DMS_INVALID_CS != suID )
      {
         _dmsCB->suUnlock ( suID ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSTMPSUMGR_INIT, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSTMPSUMGR_RELEASE, "_dmsTempSUMgr::release" )
   INT32 _dmsTempSUMgr::release ( dmsMBContext *&context )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSTMPSUMGR_RELEASE ) ;
      INT32 num = 0 ;

      _mutex.get() ;
      num = _occupiedCollections.erase ( context->mbID() ) ;
      if ( num )
      {
         _mutex.release() ;
         _su->_resetCollection ( context ) ;
         _mutex.get() ;
         _freeCollections.push ( context->mbID() ) ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
      }
      _mutex.release() ;

      _su->data()->releaseMBContext( context ) ;

      PD_TRACE_EXITRC ( SDB__DMSTMPSUMGR_RELEASE, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSTMPSUMGR_RESERVE, "_dmsTempSUMgr::reserve" )
   INT32 _dmsTempSUMgr::reserve ( dmsMBContext **ppContext, UINT64 eduID )
   {
      INT32 rc = SDB_OK ;
      UINT16 mbID = DMS_INVALID_MBID ;
      PD_TRACE_ENTRY ( SDB__DMSTMPSUMGR_RESERVE );
      if ( !_su )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      {
         DMSSYSSUMGR_XLOCK
         if ( 0 == _freeCollections.size() )
         {
            rc = SDB_DMS_NO_MORE_TEMP ;
            goto error ;
         }
         mbID = _freeCollections.front() ;

         rc = _su->data()->getMBContext( ppContext, mbID,
                                         DMS_INVALID_CLID, DMS_INVALID_CLID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get dms mb context for "
                      "collection[%d], rc: %d", mbID, rc ) ;
         _occupiedCollections[mbID] = eduID ;
         _freeCollections.pop() ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__DMSTMPSUMGR_RESERVE, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _dmsTempSUMgr::_initTmpPath()
   {
      INT32 rc = SDB_OK ;
      const CHAR *path = pmdGetOptionCB()->getTmpPath() ;

      try
      {
         fs::path dbDir( path ) ;
         fs::directory_iterator end_iter ;
         if ( fs::exists(dbDir) && fs::is_directory(dbDir) )
         {
            for ( fs::directory_iterator dir_iter(dbDir);
                  dir_iter != end_iter; ++dir_iter )
            {
/*               if ( !fs::remove_all( *dir_iter ))
               {
                  PD_LOG( PDERROR, "failed to remove tmp file." ) ;
                  rc = SDB_SYS ;
                  goto error ;

               }
*/
               if ( !fs::is_directory(dir_iter->path() ))
               {
                  std::string filename = dir_iter->path().filename().string() ;
                  UINT32 minSize = ossStrlen( DMS_TMP_BLK_FILE_BEGIN ) ;
                  if ( minSize < filename.size() &&
                       0 ==ossStrncmp( DMS_TMP_BLK_FILE_BEGIN,
                                       filename.c_str(),
                                       minSize ))
                  {
                     fs::remove( dir_iter->path()) ;
                  }
               }
            }
         }
         else
         {
            PD_LOG( PDERROR, "make sure that tmp path[%s] exists.",
                   path ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }
      catch ( std::exception & e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

}

