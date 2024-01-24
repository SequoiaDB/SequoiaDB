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

   Source File Name = rtnBackup.cpp

   Descriptive Name = Runtime Insert

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime code for insert
   request.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "barBkupLogger.hpp"
#include "rtn.hpp"
#include "rtnContextDump.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"

using namespace bson ;

namespace engine
{

   static string _rtnMakeDateDirName()
   {
      CHAR tmpBuff[ 20 ] = {0} ;
      time_t tmpTime = time( NULL ) ;
      struct tm localTm ;
      ossLocalTime( tmpTime, localTm ) ;

      ossSnprintf( tmpBuff, sizeof(tmpBuff) - 1,
                   "%04d%02d%02d",
                   localTm.tm_year+1900,            // 1) Year (UINT32)
                   localTm.tm_mon+1,                // 2) Month (UINT32)
                   localTm.tm_mday                  // 3) Day (UINT32)
                  ) ;
      return (string)tmpBuff ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNBACKUP, "rtnBackup" )
   INT32 rtnBackup( pmdEDUCB *cb, const CHAR *path, const CHAR *backupName,
                    BOOLEAN ensureInc, BOOLEAN rewrite, const CHAR *desp,
                    const BSONObj &option )
   {
      INT32 rc  = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNBACKUP ) ;
      string bkpath ;
      INT32 maxDataFileSize = 0 ;

      barBKOfflineLogger logger ;

      BOOLEAN isSubDir        = FALSE ;
      BOOLEAN enableDateDir   = FALSE ;
      BOOLEAN backupLog       = FALSE ;
      const CHAR *prefix      = NULL ;
      BOOLEAN compressed      = TRUE ;
      const CHAR *pCompType   = VALUE_NAME_SNAPPY ;
      UTIL_COMPRESSOR_TYPE compType = UTIL_COMPRESSOR_INVALID ;

      // option config
      rc = rtnGetBooleanElement( option, FIELD_NAME_ISSUBDIR, isSubDir ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDWARNING, "Failed to get field[%s], rc: %d",
                   FIELD_NAME_ISSUBDIR, rc ) ;

      rc = rtnGetIntElement( option, FIELD_NAME_MAX_DATAFILE_SIZE,
                             maxDataFileSize ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDWARNING, "Failed to get field[%s], rc: %d",
                   FIELD_NAME_MAX_DATAFILE_SIZE, rc ) ;

      rc = rtnGetBooleanElement( option, FIELD_NAME_ENABLE_DATEDIR,
                                 enableDateDir ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDWARNING, "Failed to get field[%s], rc: %d",
                   FIELD_NAME_ENABLE_DATEDIR, rc ) ;

      rc = rtnGetStringElement( option, FIELD_NAME_PREFIX, &prefix ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDWARNING, "Failed to get field[%s], rc: %d",
                   FIELD_NAME_PREFIX, rc ) ;

      rc = rtnGetBooleanElement( option, FIELD_NAME_BACKUP_LOG, backupLog ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDWARNING, "Failed to get field[%s], rc: %d",
                   FIELD_NAME_BACKUP_LOG, rc ) ;

      rc = rtnGetBooleanElement( option, FIELD_NAME_COMPRESSED, compressed ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDWARNING, "Failed to get field[%s], rc: %d",
                   FIELD_NAME_COMPRESSED, rc ) ;

      rc = rtnGetStringElement( option, FIELD_NAME_COMPRESSIONTYPE,
                                &pCompType ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDWARNING, "Failed to get field[%s], rc: %d",
                   FIELD_NAME_COMPRESSIONTYPE, rc ) ;

      compType = utilString2CompressType( pCompType ) ;
      if ( UTIL_COMPRESSOR_INVALID == compType ||
           UTIL_COMPRESSOR_LZW == compType )
      {
         PD_LOG_MSG( PDERROR, "Field[%s]'s value[%s] is invalid, only support: %s",
                     FIELD_NAME_COMPRESSIONTYPE, pCompType,
                     "snappy/lz4/zlib" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( maxDataFileSize < BAR_MIN_DATAFILE_SIZE ||
           maxDataFileSize > BAR_MAX_DATAFILE_SIZE )
      {
         maxDataFileSize = BAR_DFT_DATAFILE_SIZE ;
      }

      // make path
      if ( isSubDir && path )
      {
         bkpath = rtnFullPathName( pmdGetOptionCB()->getBkupPath(), path ) ;
      }
      else if ( path && 0 != path[0] )
      {
         bkpath = path ;
      }
      else
      {
         bkpath = pmdGetOptionCB()->getBkupPath() ;
      }

      if ( enableDateDir )
      {
         bkpath = rtnFullPathName( bkpath, _rtnMakeDateDirName() ) ;
      }

      rc = logger.init( bkpath.c_str(), backupName, maxDataFileSize, prefix,
                        ensureInc ? BAR_BACKUP_OP_TYPE_INC :
                        BAR_BACKUP_OP_TYPE_FULL, rewrite, desp ) ;
      PD_RC_CHECK( rc, PDERROR, "Init off line backup logger failed, rc: %d",
                   rc ) ;
      logger.setBackupLog( backupLog ) ;
      logger.enableCompress( compressed, compType ) ;

      rc = logger.backup( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Off line backup failed, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC ( SDB_RTNBACKUP, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 rtnDumpBackups( const BSONObj &hint, rtnContextDump *context )
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *krcb = pmdGetKRCB () ;
      const CHAR *pPath = NULL ;
      const CHAR *backupName = NULL ;
      BOOLEAN isSubDir        = FALSE ;
      const CHAR *prefix      = NULL ;
      string bkpath ;
      BOOLEAN detail = FALSE ;
      barBackupMgr bkMgr( krcb->getGroupName() ) ;
      vector < BSONObj >  vecBackup ;
      UINT32 index = 0 ;

      rc = rtnGetStringElement( hint, FIELD_NAME_PATH, &pPath ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDWARNING, "Failed to get field[%s], rc: %d",
                   FIELD_NAME_PATH, rc ) ;

      rc = rtnGetStringElement( hint, FIELD_NAME_NAME, &backupName ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDWARNING, "Failed to get field[%s], rc: %d",
                   FIELD_NAME_NAME, rc ) ;
      rc = rtnGetBooleanElement( hint, FIELD_NAME_DETAIL, detail ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDWARNING, "Failed to get field[%s], rc: %d",
                   FIELD_NAME_DETAIL, rc ) ;

      // option config
      rc = rtnGetBooleanElement( hint, FIELD_NAME_ISSUBDIR, isSubDir ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDWARNING, "Failed to get field[%s], rc: %d",
                   FIELD_NAME_ISSUBDIR, rc ) ;

      rc = rtnGetStringElement( hint, FIELD_NAME_PREFIX, &prefix ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDWARNING, "Failed to get field[%s], rc: %d",
                   FIELD_NAME_PREFIX, rc ) ;

      // make path
      if ( isSubDir && pPath )
      {
         bkpath = rtnFullPathName( pmdGetOptionCB()->getBkupPath(), pPath ) ;
      }
      else if ( pPath && 0 != pPath[0] )
      {
         bkpath = pPath ;
      }
      else
      {
         bkpath = pmdGetOptionCB()->getBkupPath() ;
      }

      rc = bkMgr.init( bkpath.c_str(), backupName, prefix ) ;
      PD_RC_CHECK( rc, PDWARNING, "Init backup manager failed, rc: %d", rc ) ;

      // list
      rc = bkMgr.list( vecBackup, detail ) ;
      PD_RC_CHECK( rc, PDWARNING, "List backup failed, rc: %d", rc ) ;

      while ( index < vecBackup.size() )
      {
         rc = context->monAppend( vecBackup[index] ) ;
         PD_RC_CHECK( rc, PDERROR, "Add to obj[%s] to context failed, "
                      "rc: %d", vecBackup[index].toString().c_str(), rc ) ;
         ++index ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rtnRemoveBackup( pmdEDUCB *cb, const CHAR *path,
                          const CHAR *backupName,
                          const BSONObj &option )
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *krcb = pmdGetKRCB () ;

      BOOLEAN isSubDir        = FALSE ;
      const CHAR *prefix      = NULL ;
      string bkpath ;
      INT32 incID             = -1 ;

      barBackupMgr bkMgr( krcb->getGroupName() ) ;

      // option config
      rc = rtnGetBooleanElement( option, FIELD_NAME_ISSUBDIR, isSubDir ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDWARNING, "Failed to get field[%s], rc: %d",
                   FIELD_NAME_ISSUBDIR, rc ) ;

      rc = rtnGetStringElement( option, FIELD_NAME_PREFIX, &prefix ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDWARNING, "Failed to get field[%s], rc: %d",
                   FIELD_NAME_PREFIX, rc ) ;

      rc = rtnGetIntElement( option, FIELD_NAME_ID, incID ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDWARNING, "Failed to get field[%s], rc: %d",
                   FIELD_NAME_ID, rc ) ;

      // make path
      if ( isSubDir && path )
      {
         bkpath = rtnFullPathName( pmdGetOptionCB()->getBkupPath(), path ) ;
      }
      else if ( path && 0 != path[0] )
      {
         bkpath = path ;
      }
      else
      {
         bkpath = pmdGetOptionCB()->getBkupPath() ;
      }

      rc = bkMgr.init( bkpath.c_str(), backupName, prefix ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init backup manager, rc: %d", rc ) ;

      rc = bkMgr.drop( incID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to drop backup[%s], ID:%d, rc: %d",
                   bkMgr.backupName(), incID, rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN rtnIsInBackup ()
   {
      return SDB_DB_OFFLINE_BK == PMD_DB_STATUS() ? TRUE : FALSE ;
   }

}

