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

   Source File Name = barBkupLogger.cpp

   Descriptive Name = backup and recovery

   When/how to use: this program may be used on backup or restore db data.
   You can specfiy some options from parameters.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "barBkupLogger.hpp"
#include "pd.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "utilCommon.hpp"
#include "rtn.hpp"
#include "dmsStorageUnit.hpp"
#include "monDump.hpp"
#include "clsReplayer.hpp"
#include "ossPath.hpp"
#include "pmdStartup.hpp"
#include "utilCompressor.hpp"
#include "pdTrace.hpp"
#include "barTrace.hpp"

#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

namespace fs = boost::filesystem ;
using namespace bson ;


namespace engine
{

   /*
      back up extent meta fields define
   */
   #define BAR_SU_NAME                    "suName"
   #define BAR_SU_SEQUENCE                "sequence"
   #define BAR_SU_FILE_NAME               "suFileName"
   #define BAR_SU_FILE_OFFSET             "offset"
   #define BAR_SU_FILE_TYPE               "type"

   /*
      back up extent meta fields values
   */
   #define BAR_SU_FILE_TYPE_DATA          "data"
   #define BAR_SU_FILE_TYPE_INDEX         "index"
   #define BAR_SU_FILE_TYPE_LOBM          "lobm"
   #define BAR_SU_FILE_TYPE_LOBD          "lobd"

   #define BAR_MAX_EXTENT_DATA_SIZE       (67108864)           // 64MB
   #define BAR_THINCOPY_THRESHOLD_SIZE    (16)                 // MB
   #define BAR_THINCOPY_THRESHOLD_RATIO   (0.1)

   #define BAR_COMPRESS_MIN_SIZE          (4096)               // 4K
   #define BAR_COMPRSSS_MAX_SIZE          BAR_MAX_EXTENT_DATA_SIZE
   #define BAR_COMPRESS_RATIO_THRESHOLD   (0.8)                // 80%

   /*
      _barBaseLogger implement
   */
   _barBaseLogger::_barBaseLogger ()
   {
      _pDMSCB        = NULL ;
      _pDPSCB        = NULL ;
      _pTransCB      = NULL ;
      _pOptCB        = NULL ;
      _pClsCB        = NULL ;

      _pDataExtent   = NULL ;
      _pCompressor   = NULL ;

      _pCompressBuff = NULL ;
      _buffSize      = 0 ;
   }

   _barBaseLogger::~_barBaseLogger ()
   {
      if ( _pDataExtent )
      {
         SDB_OSS_DEL _pDataExtent ;
         _pDataExtent = NULL ;
      }
      if ( _pCompressBuff )
      {
         SDB_OSS_FREE( _pCompressBuff ) ;
         _pCompressBuff = NULL ;
      }
      _buffSize = 0 ;
      _pDMSCB        = NULL ;
      _pDPSCB        = NULL ;
      _pTransCB      = NULL ;
      _pOptCB        = NULL ;
      _pClsCB        = NULL ;
      _pCompressor   = NULL ;
   }

   string _barBaseLogger::_replaceWildcard( const CHAR * source )
   {
      if ( NULL == source )
      {
         return "" ;
      }
      pmdKRCB *krcb = pmdGetKRCB() ;
      string destStr ;
      UINT32 index = 0 ;
      BOOLEAN isFormat = FALSE ;

      while ( source[index] )
      {
         if ( isFormat )
         {
            isFormat = FALSE ;

            if ( 'G' == source[index] || 'g' == source[index] )
            {
               destStr += krcb->getGroupName() ;
            }
            else if ( 'H' == source[index] || 'h' == source[index] )
            {
               destStr += krcb->getHostName() ;
            }
            else if ( 'S' == source[index] || 's' == source[index] )
            {
               destStr += pmdGetOptionCB()->getServiceAddr() ;
            }
            else
            {
               destStr += source[index] ;
            }
         }
         else if ( '%' == source[index] )
         {
            isFormat = TRUE ;
            ++index ;
            continue ;
         }
         else
         {
            destStr += source[index] ;
         }
         ++index ;
      }
      return destStr ;
   }

   string _barBaseLogger::getIncFileName( UINT32 sequence )
   {
      return getIncFileName( _path, _backupName, sequence ) ;
   }

   string _barBaseLogger::getIncFileName( const string &backupName,
                                          UINT32 sequence )
   {
      return getIncFileName( _path, backupName, sequence ) ;
   }

   string _barBaseLogger::getIncFileName( const string &path,
                                          const string &backupName,
                                          UINT32 sequence )
   {
      CHAR tmp[ 15 ] = {0} ;
      ossSnprintf( tmp, sizeof(tmp)-1, ".%u", sequence ) ;
      string fileName = rtnFullPathName( path, backupName ) ;
      fileName += BAR_BACKUP_META_FILE_EXT ;
      if ( sequence > 0 )
      {
         fileName += tmp ;
      }
      return fileName ;
   }

   string _barBaseLogger::getMainFileName()
   {
      string fileName = rtnFullPathName( _path, _backupName ) ;
      fileName += BAR_BACKUP_META_FILE_EXT ;
      return fileName ;
   }

   BOOLEAN _barBaseLogger::parseDateFile( const string &fileName,
                                          string &backupName,
                                          UINT32 &seq )
   {
      const CHAR *pFileName = fileName.c_str() ;
      const CHAR *pSeq = ossStrrchr( pFileName, '.' ) ;

      if ( !pSeq || pSeq == pFileName ||
           FALSE == _isDigital( pSeq + 1, &seq ) )
      {
         return FALSE ;
      }
      backupName = fileName.substr( 0, pSeq - pFileName ) ;
      return TRUE ;
   }

   BOOLEAN _barBaseLogger::parseMainFile( const string &fileName,
                                          string &backupName )
   {
      const CHAR *pFileName = fileName.c_str() ;
      const CHAR *pExt = ossStrrchr( pFileName , '.' ) ;

      if ( !pExt || pExt == pFileName ||
           0 != ossStrcmp( pExt, BAR_BACKUP_META_FILE_EXT ) )
      {
         return FALSE ;
      }

      backupName = fileName.substr( 0, pExt - pFileName ) ;
      return TRUE ;
   }

   BOOLEAN _barBaseLogger::parseIncFile( const string &fileName,
                                         string &backupName,
                                         UINT32 &incID )
   {
      const CHAR *pFileName = fileName.c_str() ;
      const CHAR *pInc = ossStrrchr( pFileName, '.' ) ;

      if ( !pInc || pInc == pFileName ||
           FALSE == _isDigital( pInc + 1, &incID ) )
      {
         return FALSE ;
      }

      string leftStr = fileName.substr( 0, pInc - pFileName ) ;
      return parseMainFile( leftStr, backupName ) ;
   }

   BOOLEAN _barBaseLogger::parseMetaFile( const string &fileName,
                                          string &backupName,
                                          UINT32 &incID )
   {
      if ( parseIncFile( fileName, backupName, incID ) )
      {
         return TRUE ;
      }
      else if ( parseMainFile( fileName, backupName ) )
      {
         incID = 0 ;
         return TRUE ;
      }
      return FALSE ;
   }

   BOOLEAN _barBaseLogger::_isDigital( const CHAR *pStr,
                                       UINT32 *pNum )
   {
      const CHAR *pTmp = pStr ;
      if ( !pStr || !*pStr )
      {
         return FALSE ;
      }
      while( *pTmp )
      {
         if ( *pTmp < '0' || *pTmp > '9' )
         {
            return FALSE ;
         }
         ++pTmp ;
      }
      if ( pNum )
      {
         *pNum = (UINT32)ossAtoi( pStr ) ;
      }
      return TRUE ;
   }

   string _barBaseLogger::getDataFileName( UINT32 sequence )
   {
      return getDataFileName( _backupName, sequence ) ;
   }

   string _barBaseLogger::getDataFileName( const string &backupName,
                                           UINT32 sequence )
   {
      SDB_ASSERT( sequence > 0, "sequence must > 0" ) ;

      CHAR tmp[ 15 ] = {0} ;
      ossSnprintf( tmp, sizeof(tmp)-1, ".%u", sequence ) ;
      string fileName = rtnFullPathName( _path, backupName ) ;
      fileName += tmp ;
      return fileName ;
   }

   UINT32 _barBaseLogger::_ensureMetaFileSeq ( set< UINT32 > *pSetSeq )
   {
      return _ensureMetaFileSeq( _path, _backupName, pSetSeq ) ;
   }

   UINT32 _barBaseLogger::_ensureMetaFileSeq( const string &backupName,
                                              set< UINT32 > *pSetSeq )
   {
      return _ensureMetaFileSeq( _path, backupName, pSetSeq ) ;
   }

   UINT32 _barBaseLogger::_ensureMetaFileSeq( const string &path,
                                              const string &backupName,
                                              set< UINT32 > *pSetSeq )
   {
      UINT32 metaFileSeq = 0 ;
      INT32 rc = SDB_OK ;
      string filter = backupName + BAR_BACKUP_META_FILE_EXT ;
      multimap < string, string > mapFiles ;
      multimap < string, string >::iterator it ;

      string tmpName ;
      UINT32 tmpID = 0 ;

      rc = ossEnumFiles2( path, mapFiles, filter.c_str(),
                          OSS_MATCH_LEFT, 1 ) ;
      if ( rc )
      {
         goto done ;
      }

      it = mapFiles.begin() ;
      while( it != mapFiles.end() )
      {
         const string &fileName = it->first ;

         if ( parseMetaFile( fileName, tmpName, tmpID ) &&
              backupName == tmpName )
         {
            if ( pSetSeq )
            {
               pSetSeq->insert( tmpID ) ;
            }
            if ( tmpID + 1 > metaFileSeq )
            {
               metaFileSeq = tmpID + 1 ;
            }
         }
         ++it ;
      }

   done:
      return metaFileSeq ;
   }

   INT32 _barBaseLogger::_initInner ( const CHAR *path, const CHAR *backupName,
                                      const CHAR *prefix )
   {
      INT32 rc = SDB_OK ;

      pmdKRCB *krcb = pmdGetKRCB() ;
      _pDMSCB = krcb->getDMSCB() ;
      _pDPSCB = krcb->getDPSCB() ;
      _pTransCB = krcb->getTransCB() ;
      _pOptCB = krcb->getOptionCB() ;
      _pClsCB = krcb->getClsCB() ;

      if ( !_pDataExtent )
      {
         _pDataExtent = SDB_OSS_NEW barBackupExtentHeader ;
         if ( !_pDataExtent )
         {
            PD_LOG( PDERROR, "Failed to alloc memory for extent header" ) ;
            rc = SDB_OOM ;
            goto error ;
         }
      }

      if ( !path || 0 == ossStrlen( path ) )
      {
         PD_LOG( PDWARNING, "Path can't be empty" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // init meta header
      _metaHeader.makeBeginTime() ;
      if ( !backupName || 0 == ossStrlen( backupName ) )
      {
         _backupName = _metaHeader._startTimeStr ;
      }
      else
      {
         _backupName = backupName ;
      }
      _path          = _replaceWildcard ( path ) ;

      if ( prefix )
      {
         string tmpName = _replaceWildcard( prefix ) ;
         tmpName += "_" ;
         tmpName += _backupName ;
         _backupName = tmpName ;
      }

      _metaHeader.setPath( _path.c_str() ) ;
      _metaHeader.setName( _backupName.c_str(), NULL ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _barBaseLogger::_read( OSSFILE & file, CHAR * buf, SINT64 len )
   {
      INT32 rc = SDB_OK ;
      SINT64 read = 0 ;
      SINT64 needRead = len ;
      SINT64 bufOffset = 0 ;

      while ( 0 < needRead )
      {
         rc = ossRead( &file, buf + bufOffset, needRead, &read );
         if ( rc )
         {
            if ( SDB_INTERRUPT != rc && SDB_EOF != rc )
            {
               PD_LOG( PDWARNING, "Failed to read data, rc: %d", rc ) ;
            }
            goto error ;
         }
         needRead -= read ;
         bufOffset += read ;

         rc = SDB_OK ;
      }

   done:
      return rc ;
   error:
      goto done;
   }

   INT32 _barBaseLogger::_flush( OSSFILE &file, const CHAR *buf,
                                 SINT64 len )
   {
      INT32 rc = SDB_OK;
      SINT64 written = 0;
      SINT64 needWrite = len;
      SINT64 bufOffset = 0;

      while ( 0 < needWrite )
      {
         rc = ossWrite( &file, buf + bufOffset, needWrite, &written );
         if ( rc && SDB_INTERRUPT != rc )
         {
            PD_LOG( PDWARNING, "Failed to write data, rc: %d", rc ) ;
            goto error ;
         }
         needWrite -= written ;
         bufOffset += written ;

         rc = SDB_OK ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _barBaseLogger::_close( OSSFILE &file, BOOLEAN fsync )
   {
      INT32 rc = SDB_OK ;
      if ( fsync )
      {
         INT32 rc2 = ossFsync( &file ) ;
         // ignore SDB_INVALIDARG - it means the filesystem doesn't need fsync
         if ( SDB_INVALIDARG != rc2 )
         {
            rc = rc2 ;
         }
      }
      // close the file descriptor
      INT32 rc3 = ossClose( file ) ;
      if ( rc3 )
      {
         if ( SDB_OK == rc )
         {
            // only return the new rc if there were no previous errors
            rc = rc3 ;
         }
         PD_LOG( PDERROR, "Error closing file" ) ;
      }
      return rc ;
   }

   INT32 _barBaseLogger::_readMetaHeader( UINT32 incID,
                                          barBackupHeader *pHeader,
                                          BOOLEAN check,
                                          UINT64 secretValue )
   {
      INT32 rc = SDB_OK ;
      OSSFILE file ;
      BOOLEAN isOpened = FALSE ;
      string fileName = getIncFileName( incID ) ;

      rc = ossOpen( fileName.c_str(), OSS_READONLY, OSS_RU|OSS_WU|OSS_RG,
                    file ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open file[%s], rc: %d",
                   fileName.c_str(), rc ) ;
      isOpened = TRUE ;

      rc = _read( file, (CHAR*)pHeader, BAR_BACKUP_HEADER_SIZE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to read header from file[%s], rc: %d",
                   fileName.c_str(), rc ) ;

      if ( FALSE == check )
      {
         goto done ;
      }

      // check
      if ( 0 != ossStrncmp( pHeader->_eyeCatcher, BAR_BACKUP_META_EYECATCHER,
                            BAR_BACKUP_HEADER_EYECATCHER_LEN ) )
      {
         PD_LOG( PDWARNING, "Invalid eyecatcher[%s]", pHeader->_eyeCatcher ) ;
         rc = SDB_BAR_DAMAGED_BK_FILE ;
         goto error ;
      }
      else if ( 0 != ossStrncmp( pHeader->_name, backupName(),
                                 BAR_BACKUP_NAME_LEN ) )
      {
         PD_LOG( PDWARNING, "Invalid backup name[%s], should be: %s",
                 pHeader->_name, backupName() ) ;
         rc = SDB_BAR_DAMAGED_BK_FILE ;
         goto error ;
      }
      else if ( 0 != secretValue && secretValue != pHeader->_secretValue )
      {
         PD_LOG( PDERROR, "File[%s] header's secret value[%lld] is not the "
                 "same with [%lld]", fileName.c_str(),
                 pHeader->_secretValue, secretValue ) ;
         rc = SDB_BAR_DAMAGED_BK_FILE ;
         goto error ;
      }

   done:
      if ( isOpened )
      {
         INT32 rc2 = _close( file, FALSE ) ;
         if ( SDB_OK == rc )
         {
            rc = rc2 ;
         }
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _barBaseLogger::_readDataHeader( OSSFILE &file,
                                          const string &fileName,
                                          barBackupDataHeader *pHeader,
                                          BOOLEAN check, UINT64 secretValue,
                                          UINT32 sequence )
   {
      INT32 rc = SDB_OK ;

      rc = _read( file, (CHAR*)pHeader, BAR_BACKUPDATA_HEADER_SIZE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to read data header from file[%s], "
                   "rc: %d", fileName.c_str(), rc ) ;

      if ( FALSE == check )
      {
         goto done ;
      }

      // check
      if ( 0 != ossStrncmp( pHeader->_eyeCatcher, BAR_BACKUP_DATA_EYECATCHER,
                            BAR_BACKUP_HEADER_EYECATCHER_LEN ) )
      {
         PD_LOG( PDERROR, "Invalid eyecatcher[%s] in file[%s]",
                 pHeader->_eyeCatcher, fileName.c_str() ) ;
         rc = SDB_BAR_DAMAGED_BK_FILE ;
         goto error ;
      }
      else if ( 0 != secretValue && secretValue != pHeader->_secretValue )
      {
         PD_LOG( PDERROR, "Secret value[%llu] is not expect[%llu] in file[%s]",
                 pHeader->_secretValue, secretValue, fileName.c_str() ) ;
         rc = SDB_BAR_DAMAGED_BK_FILE ;
         goto error ;
      }
      else if ( 0 != sequence && sequence != pHeader->_sequence )
      {
         PD_LOG( PDERROR, "Sequence value[%d] is not expect[%d] in file[%s]",
                 pHeader->_sequence, sequence, fileName.c_str() ) ;
         rc = SDB_BAR_DAMAGED_BK_FILE ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _barBaseLogger::_readDataHeader( UINT32 sequence,
                                          barBackupDataHeader *pHeader,
                                          BOOLEAN check,
                                          UINT64 secretValue )
   {
      INT32 rc = SDB_OK ;
      string fileName = getDataFileName( sequence ) ;
      OSSFILE file ;
      BOOLEAN isOpened = FALSE ;

      rc = ossOpen( fileName.c_str(), OSS_READONLY, OSS_RU|OSS_WU|OSS_RG,
                    file ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open file[%s], rc: %d",
                   fileName.c_str(), rc ) ;
      isOpened = TRUE ;

      rc = _readDataHeader( file, fileName, pHeader, check, secretValue,
                            sequence ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      if ( isOpened )
      {
         INT32 rc2 = _close( file, FALSE ) ;
         if ( SDB_OK == rc )
         {
            rc = rc2 ;
         }
      }
      return rc ;
   error:
      goto done ;
   }

   CHAR* _barBaseLogger::_allocCompressBuff( UINT64 buffSize )
   {
      CHAR *pBuff = NULL ;

      if ( _buffSize >= buffSize )
      {
         pBuff = _pCompressBuff ;
      }
      else
      {
         if ( _pCompressBuff )
         {
            SDB_OSS_FREE( _pCompressBuff ) ;
            _pCompressBuff = NULL ;
            _buffSize = 0 ;
         }

         _pCompressBuff = (CHAR*) SDB_OSS_MALLOC( buffSize ) ;
         if ( _pCompressBuff )
         {
            _buffSize = buffSize ;
            pBuff = _pCompressBuff ;
            ossMemset( pBuff, 0, buffSize ) ;
         }
      }

      return pBuff ;
   }

   /*
      _barBkupBaseLogger implement
   */
   _barBkupBaseLogger::_barBkupBaseLogger ()
   {
      _rewrite       = TRUE ;
      _metaFileSeq   = 0 ;
      _curFileSize   = 0 ;
      _isOpened      = FALSE ;
      _isOpenedWriter = FALSE ;

      _lastLSN       = ~0 ;
      _lastLSNCode   = 0 ;

      _needBackupLog = FALSE ;
      _compressed    = TRUE ;
      _metaHeader._compressionType = UTIL_COMPRESSOR_SNAPPY ;
   }

   _barBkupBaseLogger::~_barBkupBaseLogger ()
   {
      // ignore the rc - this is a destructor and can't handle failure
      _closeCurFile() ;
   }

   INT32 _barBkupBaseLogger::_closeCurFile ()
   {
      INT32 rc = SDB_OK ;
      if ( _isOpened )
      {
         rc = _close( _curFile, _isOpenedWriter ) ;
         _isOpened = FALSE ;
         _isOpenedWriter = FALSE ;
      }
      return rc ;
   }

   void _barBkupBaseLogger::setBackupLog( BOOLEAN backupLog )
   {
      _needBackupLog = backupLog ;
   }

   void _barBkupBaseLogger::enableCompress( BOOLEAN compressed,
                                            UTIL_COMPRESSOR_TYPE compType )
   {
      _compressed = compressed ;

      if ( _compressed )
      {
         if ( UTIL_COMPRESSOR_INVALID == compType ||
              UTIL_COMPRESSOR_LZW == compType )
         {
            compType = UTIL_COMPRESSOR_SNAPPY ;
         }
         _metaHeader._compressionType = compType ;
      }
      else
      {
         _metaHeader._compressionType = UTIL_COMPRESSOR_INVALID ;
         _pCompressor = NULL ;
      }
   }

   INT32 _barBkupBaseLogger::init( const CHAR *path,
                                   const CHAR *backupName,
                                   INT32 maxDataFileSize,
                                   const CHAR *prefix,
                                   UINT32 opType,
                                   BOOLEAN rewrite,
                                   const CHAR *backupDesp )
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *krcb = pmdGetKRCB() ;

      rc = _initInner( path, backupName, prefix ) ;
      PD_RC_CHECK( rc, PDERROR, "Init inner failed, rc: %d", rc ) ;

      if ( BAR_BACKUP_OP_TYPE_FULL != opType &&
           BAR_BACKUP_OP_TYPE_INC != opType )
      {
         PD_LOG( PDWARNING, "Invalid backup op type: %d", opType ) ;
         rc = SDB_INVALIDARG ;
      }

      // 1. init meta header
      _metaHeader._type                = _getBackupType() ;
      _metaHeader._opType              = opType ;
      _metaHeader._maxDataFileSize     = (UINT64)maxDataFileSize << 20 ;
      ossStrncpy( _metaHeader._groupName, krcb->getGroupName(),
                  BAR_BACKUP_GROUPNAME_LEN - 1 ) ;
      ossStrncpy( _metaHeader._hostName, krcb->getHostName(),
                  BAR_BACKUP_HOSTNAME_LEN - 1 ) ;
      ossStrncpy( _metaHeader._svcName, pmdGetOptionCB()->getServiceAddr(),
                  BAR_BACKUP_SVCNAME_LEN - 1 ) ;
      _metaHeader._nodeID              = pmdGetNodeID().value ;
      if ( backupDesp )
      {
         _metaHeader.setDesp( backupDesp ) ;
      }
      _rewrite                         = rewrite ;

      // 2. ensure meta file seq
      _metaFileSeq = _ensureMetaFileSeq () ;

      // 3. init check and prepare for backup
      rc = _initCheckAndPrepare() ;
      PD_RC_CHECK( rc, PDWARNING, "Prepare check for backup failed, rc: %d",
                   rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _barBkupBaseLogger::backup ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN hasPrepared = FALSE ;
      BOOLEAN isEmpty = FALSE ;

      PD_LOG( PDEVENT, "Begin to backup[%s]...", backupName() ) ;

      // init compressor
      if ( _compressed )
      {
         _pCompressor = getCompressorByType(
            (UTIL_COMPRESSOR_TYPE)_metaHeader._compressionType ) ;
      }

      try
      {
         // 1. prepare for backup
         rc = _prepareBackup( cb, isEmpty ) ;
         PD_RC_CHECK( rc, PDERROR, "Prepare for backup failed, rc: %d", rc ) ;

         hasPrepared = TRUE ;

         if ( !isEmpty )
         {
            // 2. backup config
            rc = _backupConfig() ;
            PD_RC_CHECK( rc, PDERROR, "Failed to backup config, rc: %d", rc ) ;

            // 3. do backup data
            rc = _doBackup ( cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to do backup, rc: %d", rc ) ;

            // 4. write meta file
            rc = _writeMetaFile () ;
            PD_RC_CHECK( rc, PDERROR, "Failed to write meta file, rc: %d",
                         rc ) ;
         }
         else
         {
            PD_LOG( PDWARNING, "Backup[%s] is empty, will ignored",
                    backupName() ) ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to run backup [%s], occur exception %s",
                 backupName(), e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      // 5. clean up after backup
      rc = _afterBackup ( cb ) ;
      hasPrepared = FALSE ;
      PD_RC_CHECK( rc, PDERROR, "Failed to cleanup after backup, rc: %d", rc ) ;

      PD_LOG( PDEVENT, "Complete backup[%s]", backupName() ) ;

   done:
      return rc ;
   error:
      {
         INT32 tempRC = dropCurBackup() ;
         if ( tempRC )
         {
            PD_LOG( PDWARNING, "Rollback bakcup failed, rc: %d", tempRC ) ;
         }

         if ( hasPrepared )
         {
            tempRC = _afterBackup ( cb ) ;
            hasPrepared = FALSE ;
            if ( tempRC )
            {
               PD_LOG( PDWARNING, "Failed to cleanup after backup, rc: %d",
                       tempRC ) ;
            }
         }
      }
      goto done ;
   }


   INT32 _barBkupBaseLogger::writeData( const CHAR *data, UINT32 len )
   {
      return _flush( _curFile, data, len ) ;
   }

   INT32 _barBkupBaseLogger::_initCheckAndPrepare ()
   {
      INT32 rc = SDB_OK ;

      // 1. ensure path valid
      // Attempt to make the directory
      rc = ossMkdir( _metaHeader._path ) ;
      if ( SDB_PERM == rc )
      {
         // ossMkdir may return SDB_PERM if mkdir succeeded but chmod failed.
         // This can happen when there are userid issues on the system. A
         // common scenario for this issue is when a mounted network disk
         // userids are wrong.
         PD_LOG( PDWARNING, "Failed to set permissions on backup dir[%s]",
                 _metaHeader._path ) ;
      }
      // SDB_FE means the dir exists, which is ok
      else if ( rc && SDB_FE != rc )
      {
         PD_LOG( PDERROR, "Create backup dir[%s] failed, rc: %d",
                 _metaHeader._path, rc ) ;
         goto error ;
      }
      // Verify read/write access to the backup directory
      rc = ossAccess( _metaHeader._path, OSS_MODE_READWRITE ) ;
      if ( rc )
      {
         if ( SDB_PERM == rc )
         {
            PD_LOG( PDERROR, "No read/write privileges on the backup dir[%s]",
                    _metaHeader._path ) ;
         }
         else if ( SDB_FNE == rc )
         {
            PD_LOG( PDERROR, "Failed to create backup dir[%s]",
                    _metaHeader._path ) ;
         }
         else
         {
            PD_LOG( PDERROR, "Checking access to backup dir[%s] failed, rc: %d",
                    _metaHeader._path, rc ) ;
         }
         goto error ;
      }
      rc = SDB_OK ;

      // 2. backup exist check
      if ( BAR_BACKUP_OP_TYPE_INC == _metaHeader._opType &&
           0 == _metaFileSeq )
      {
         PD_LOG( PDERROR, "Full backup[%s] not exist", backupName() ) ;
         rc = SDB_BAR_BACKUP_NOTEXIST ;
         goto error ;
      }
      else if ( BAR_BACKUP_OP_TYPE_FULL == _metaHeader._opType &&
                _metaFileSeq != 0  )
      {
         if ( !_rewrite )
         {
            PD_LOG( PDERROR, "Backup[%s] already exist", backupName() ) ;
            rc = SDB_BAR_BACKUP_EXIST ;
         }
         else
         {
            rc = dropAllBackup () ;
            PD_RC_CHECK( rc, PDERROR, "Failed to drop backup[%s], rc: %d",
                         backupName(), rc ) ;
         }
      }

      // 3. read last info for INC backup
      if ( BAR_BACKUP_OP_TYPE_INC == _metaHeader._opType )
      {
         _metaHeader._secretValue = 0 ;

         rc = _updateFromMetaFile( _metaFileSeq - 1 ) ;
         PD_RC_CHECK( rc, PDERROR, "Update from meta file[%d] failed, rc: %d",
                      _metaFileSeq - 1, rc ) ;

         PD_LOG( PDDEBUG, "Backup info: lastFileSequence: %lld, "
                 "lastExtentID: %lld, beginLSN: %llu, secretValue: %llu",
                 _metaHeader._lastDataSequence, _metaHeader._lastExtentID,
                 _metaHeader._beginLSNOffset, _metaHeader._secretValue ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _barBkupBaseLogger::_updateFromMetaFile( UINT32 incID )
   {
      INT32 rc = SDB_OK ;
      barBackupHeader *pHeader = NULL ;

      pHeader = SDB_OSS_NEW barBackupHeader ;
      if ( !pHeader )
      {
         PD_LOG( PDERROR, "Failed to alloc memory for backup header" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = _readMetaHeader( incID, pHeader, TRUE, _metaHeader._secretValue ) ;
      if ( rc )
      {
         goto error ;
      }

      // update info
      _metaHeader._lastDataSequence = pHeader->_lastDataSequence ;
      _metaHeader._beginLSNOffset   = pHeader->_endLSNOffset ;
      _metaHeader._lastExtentID     = pHeader->_lastExtentID ;
      if ( 0 == _metaHeader._secretValue )
      {
         _metaHeader._secretValue      = pHeader->_secretValue ;
      }

      _lastLSN = pHeader->_lastLSN ;
      _lastLSNCode = pHeader->_lastLSNCode ;

   done:
      if ( pHeader )
      {
         SDB_OSS_DEL pHeader ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _barBkupBaseLogger::_writeData( const CHAR * buf, INT64 len,
                                         BOOLEAN isExtHeader )
   {
      INT32 rc = SDB_OK ;

      if ( isExtHeader )
      {
         barBackupExtentHeader *pHeader = (barBackupExtentHeader*)buf ;
         UINT32 dataLen = pHeader->_dataSize ;
         if ( 0 != pHeader->_thinCopy )
         {
            _metaHeader._thinDataSize += dataLen ;
            dataLen = 0 ;
         }
         if ( _curFileSize + len + dataLen > _metaHeader._maxDataFileSize )
         {
            rc = _closeCurFile() ;
            if ( rc )
            {
               // backup() will clean up the bad backup's files
               goto error ;
            }
         }
      }

      if ( !_isOpened )
      {
         rc = _openDataFile() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open data file, rc: %d", rc ) ;
      }

      rc = _flush( _curFile, buf, len ) ;
      if ( rc )
      {
         goto error ;
      }
      _curFileSize += len ;
      _metaHeader._dataSize += len ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _barBkupBaseLogger::_writeExtent( barBackupExtentHeader *pHeader,
                                           const CHAR *pData )
   {
      INT32 rc = SDB_OK ;
      UINT64 srcDataSize = pHeader->_dataSize ;
      UINT8 srcCompressed = pHeader->_compressed ;

      if ( _compressed && _pCompressor &&
           0 == pHeader->_thinCopy &&
           0 == srcCompressed &&
           srcDataSize >= BAR_COMPRESS_MIN_SIZE &&
           srcDataSize <= BAR_COMPRSSS_MAX_SIZE )
      {
         UINT32 maxCompSize = 0 ;
         UINT32 destLen = 0 ;
         CHAR *pBuff = NULL ;

         rc = _pCompressor->compressBound( srcDataSize, maxCompSize, NULL ) ;
         if ( SDB_OK == rc )
         {
            if ( maxCompSize > srcDataSize )
            {
               destLen = maxCompSize ;
            }
            else
            {
               destLen = srcDataSize ;
            }
            pBuff = _allocCompressBuff( srcDataSize ) ;
         }

         if ( pBuff )
         {
            rc = _pCompressor->compress( pData, srcDataSize, pBuff,
                                         destLen, NULL, NULL ) ;
            if ( SDB_OK == rc &&
                 ( (FLOAT64)destLen / srcDataSize ) <=
                 BAR_COMPRESS_RATIO_THRESHOLD  )
            {
               _metaHeader._compressDataSize += ( srcDataSize - destLen ) ;

               pHeader->_compressed = 1 ;
               pHeader->_dataSize = destLen ;
               pData = pBuff ;
            }
         } // if ( pBuff )
      }

      /// write header
      rc = _writeData( (const CHAR*)pHeader, BAR_BACKUP_EXTENT_HEADER_SIZE,
                       TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to write extent header, rc: %d",
                   rc ) ;

      /// write data
      if ( 0 == pHeader->_thinCopy )
      {
         rc = _writeData( pData, pHeader->_dataSize, FALSE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to write extent data, rc: %d",
                      rc ) ;
      }

   done:
      /// restore
      pHeader->_dataSize = srcDataSize ;
      pHeader->_compressed = srcCompressed ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _barBkupBaseLogger::_openDataFile ()
   {
      if ( _isOpened )
      {
         return SDB_OK ;
      }

      ++_metaHeader._lastDataSequence ;

      barBackupDataHeader *pHeader = NULL ;
      string fileName = getDataFileName( _metaHeader._lastDataSequence ) ;
      INT32 rc = ossOpen( fileName.c_str(), OSS_REPLACE | OSS_READWRITE,
                          OSS_RU | OSS_WU | OSS_RG, _curFile ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to open file[%s], rc: %d", fileName.c_str(),
                 rc ) ;
         goto error ;
      }
      _isOpened = TRUE ;
      _isOpenedWriter = TRUE ; // must set this because we are writing

      // write header
      pHeader = SDB_OSS_NEW barBackupDataHeader ;
      if ( !pHeader )
      {
         PD_LOG( PDERROR, "Failed to alloc memory for backup data header" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      pHeader->_secretValue   = _metaHeader._secretValue ;
      pHeader->_sequence      = _metaHeader._lastDataSequence ;
      pHeader->_compressionType = _metaHeader._compressionType ;

      rc = _flush( _curFile, (const CHAR *)pHeader,
                   BAR_BACKUPDATA_HEADER_SIZE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to write header to file[%s], rc: %d",
                   fileName.c_str(), rc ) ;

      _curFileSize = BAR_BACKUPDATA_HEADER_SIZE ;
      _metaHeader._dataSize += BAR_BACKUPDATA_HEADER_SIZE ;
      _metaHeader._dataFileNum++ ;

   done:
      if ( pHeader )
      {
         SDB_OSS_DEL pHeader ;
      }
      return rc ;
   error:
      goto done ;
   }

   barBackupExtentHeader* _barBkupBaseLogger::_nextDataExtent( UINT32 dataType )
   {
      if ( !_pDataExtent )
      {
         return NULL ;
      }
      _pDataExtent->init() ;
      _pDataExtent->_dataType = dataType ;
      _pDataExtent->_extentID = ++_metaHeader._lastExtentID ;

      return _pDataExtent ;
   }

   INT32 _barBkupBaseLogger::_backupConfig ()
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;
      BSONObj tmpData, cfgData ;
      barBackupExtentHeader *pHeader = _nextDataExtent( BAR_DATA_TYPE_CONFIG ) ;
      CHAR tmpBuff[4] = {0} ;
      UINT32 tmpSize = 0 ;

      rc = _pOptCB->toBSON( tmpData, PMD_CFG_MASK_SKIP_UNFIELD ) ;
      PD_RC_CHECK( rc, PDERROR, "Config to bson failed, rc: %d", rc ) ;

      try
      {
         BSONObjIterator it( tmpData ) ;
         while( it.more() )
         {
            BSONElement e = it.next() ;
            if ( 0 == ossStrcmp( e.fieldName(), PMD_OPTION_CONFPATH ) )
            {
               continue ;
            }
            builder.append( e ) ;
         }
         builder.append( PMD_OPTION_CONFPATH, _pOptCB->getConfPath() ) ;
         cfgData = builder.obj() ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      tmpSize = ossAlign4( (UINT32)cfgData.objsize() ) - cfgData.objsize() ;
      pHeader->_dataSize = ossAlign4( (UINT32)cfgData.objsize () ) ;

      // write extent header
      rc = _writeData( (const CHAR*)pHeader, BAR_BACKUP_EXTENT_HEADER_SIZE,
                       TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to write config extent header, rc: %d",
                   rc ) ;
      // wirte config
      rc = _writeData( cfgData.objdata(), cfgData.objsize(), FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to write config, rc: %d", rc ) ;

      if ( 0 != tmpSize )
      {
         // wirte align data
         rc = _writeData( (const CHAR*)tmpBuff, tmpSize, FALSE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to write config align data, rc: %d",
                      rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _barBkupBaseLogger::_writeMetaFile ()
   {
      INT32 rc = SDB_OK ;
      string fileName ;

      rc = _onWriteMetaFile() ;
      PD_RC_CHECK( rc, PDERROR, "On write meta file failed, rc: %d", rc ) ;

      rc = _closeCurFile() ;
      if ( rc )
      {
         // backup() will clean up the bad backup's files
         goto error ;
      }

      fileName = getIncFileName( _metaFileSeq ) ;
      rc = ossOpen( fileName.c_str(), OSS_REPLACE | OSS_READWRITE,
                    OSS_RU | OSS_WU | OSS_RG, _curFile ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open file[%s], rc: %d",
                   fileName.c_str(), rc ) ;

      _isOpened = TRUE ;
      _isOpenedWriter = TRUE ; // must set this because we are writing
      _metaHeader.makeEndTime() ;

      rc = _flush( _curFile, (const CHAR *)&_metaHeader,
                   BAR_BACKUP_HEADER_SIZE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to write backup header to meta "
                   "file[%s], rc: %d", fileName.c_str(), rc ) ;

      rc = _closeCurFile() ;
      if ( rc )
      {
         // backup() will clean up the bad backup's files
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _barBkupBaseLogger::dropCurBackup ()
   {
      INT32 rc = SDB_OK ;
      string fileName ;

      // ignore the rc - this is the failure cleanup anyway
      _closeCurFile() ;

      // 1. remove data files
      while ( _metaHeader._lastDataSequence > 0 &&
              _metaHeader._dataFileNum > 0 )
      {
         fileName = getDataFileName( _metaHeader._lastDataSequence ) ;
         if ( SDB_OK == ossAccess( fileName.c_str() ) )
         {
            rc = ossDelete( fileName.c_str() ) ;
            PD_RC_CHECK( rc, PDWARNING, "Failed to delete file[%s], rc: %d",
                         fileName.c_str(), rc ) ;
         }

         --_metaHeader._dataFileNum ;
         --_metaHeader._lastDataSequence ;
      }

      // 2. remove meta file
      fileName = getIncFileName( _metaFileSeq ) ;
      if ( SDB_OK == ossAccess( fileName.c_str() ) )
      {
         rc = ossDelete( fileName.c_str() ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to delete file[%s], rc: %d",
                      fileName.c_str(), rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _barBkupBaseLogger::dropAllBackup ()
   {
      INT32 rc = SDB_OK ;
      barBackupMgr bkMgr ;

      rc = bkMgr.init( path(), backupName() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init backup mgr, rc: %d", rc ) ;

      rc = bkMgr.drop() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to drop backup[%s], rc: %d",
                   backupName(), rc ) ;

      _metaFileSeq = 0 ;

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _barBKOfflineLogger implement
   */
   _barBKOfflineLogger::_barBKOfflineLogger ()
   {
      _curDataType = BAR_DATA_TYPE_RAW_DATA ;
      _curOffset   = 0 ;
      _curSequence = 0 ;
      _blockSync   = FALSE ;
      _hasRegBackup = FALSE ;
      _pExtentBuff = NULL ;
   }

   _barBKOfflineLogger::~_barBKOfflineLogger ()
   {
      if ( _pExtentBuff )
      {
         SDB_OSS_FREE( _pExtentBuff ) ;
         _pExtentBuff = NULL ;
      }
   }

   UINT32 _barBKOfflineLogger::_getBackupType () const
   {
      return BAR_BACKUP_TYPE_OFFLINE ;
   }

   INT32 _barBKOfflineLogger::_prepareBackup ( _pmdEDUCB *cb, BOOLEAN &isEmpty )
   {
      pmdKRCB *krcb = pmdGetKRCB() ;
      INT32 rc = SDB_OK ;
      DPS_LSN beginlsn, expectlsn, currentLSN ;
      DPS_LSN_OFFSET transLSN = DPS_INVALID_LSN_OFFSET ;

      isEmpty = FALSE ;

      if ( BAR_BACKUP_OP_TYPE_FULL == _metaHeader._opType )
      {
         if ( SDB_ROLE_STANDALONE != krcb->getDBRole() )
         {
            _pClsCB->getReplCB()->syncMgr()->disableSync() ;
            _blockSync = TRUE ;

            _pClsCB->getReplCB()->getSyncEmptyEvent()->wait() ;

            // wait all log complete
            while ( TRUE )
            {
               if ( cb->isInterrupted() )
               {
                  rc = SDB_APP_INTERRUPT ;
                  goto error ;
               }
               if ( SDB_OK == _pClsCB->getReplCB()->getBucket()->waitEmpty(
                              OSS_ONE_SEC ) )
               {
                  break ;
               }
            }
         }

         rc = _pDMSCB->registerBackup( cb, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to register backup, rc: %d", rc ) ;

         pmdGetKRCB()->getFTMgr()->holdStatus( PMD_FT_MASK_DEADSYNC ) ;
         _hasRegBackup = TRUE ;

         /// sync
         rtnSyncDB( cb, -1, NULL, FALSE ) ;
      }
      else
      {
         rc = _pDMSCB->registerBackup( cb, FALSE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to register backup, rc: %d", rc ) ;
         _hasRegBackup = TRUE ;
      }

      // if increase backup, need to check lsn
      beginlsn = _pDPSCB->getStartLsn( FALSE ) ;
      expectlsn = _pDPSCB->expectLsn() ;
      currentLSN = _pDPSCB->getCurrentLsn() ;
      transLSN = _pTransCB->getOldestBeginLsn() ;

      if ( BAR_BACKUP_OP_TYPE_INC == _metaHeader._opType )
      {
         if ( beginlsn.compareOffset( _metaHeader._beginLSNOffset ) > 0 )
         {
            PD_LOG( PDERROR, "Begin lsn[%lld] is smaller than log's begin "
                    "lsn[%u,%lld]", _metaHeader._beginLSNOffset,
                    beginlsn.version, beginlsn.offset ) ;
            rc = SDB_DPS_LOG_NOT_IN_BUF ;
            goto error ;
         }
         else if ( expectlsn.compareOffset( _metaHeader._beginLSNOffset ) <= 0 )
         {
            isEmpty = TRUE ;
            goto done ;
         }

         /// when need to check last lsn's hash code
         if ( 0 != _lastLSNCode && (UINT64)~0 != _lastLSN )
         {
            dpsMessageBlock mb( DPS_MSG_BLOCK_DEF_LEN ) ;
            DPS_LSN searchLsn ;
            searchLsn.offset = _lastLSN ;
            UINT32 hashValue = 0 ;
            rc = _pDPSCB->search( searchLsn, &mb ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Search last check lsn[%lld] failed, rc: %d",
                       _lastLSN, rc ) ;
               goto error ;
            }
            hashValue = ossHash( mb.offset( 0 ), mb.length() ) ;
            if ( _lastLSNCode != hashValue )
            {
               PD_LOG( PDERROR, "Last lsn[%lld]'s hash value[%u] is not the "
                       "same[%u]", _lastLSN, _lastLSNCode, hashValue ) ;
               rc = SDB_DPS_CORRUPTED_LOG ;
               goto error ;
            }
         }
      }
      else if ( BAR_BACKUP_OP_TYPE_FULL == _metaHeader._opType )
      {
         if ( _needBackupLog )
         {
            _metaHeader._beginLSNOffset = beginlsn.offset ;
         }
         else if ( DPS_INVALID_LSN_OFFSET != transLSN &&
                   transLSN < currentLSN.offset )
         {
            _metaHeader._beginLSNOffset = transLSN ;
         }
         else
         {
            _metaHeader._beginLSNOffset = currentLSN.offset ;
         }
      }

      _metaHeader._endLSNOffset   = expectlsn.offset ;
      _metaHeader._transLSNOffset = transLSN ;

   done:
      return rc ;
   error:
      if ( _hasRegBackup )
      {
         _pDMSCB->backupDown( cb ) ;
         if ( BAR_BACKUP_OP_TYPE_FULL == _metaHeader._opType )
         {
            pmdGetKRCB()->getFTMgr()->unholdStatus( PMD_FT_MASK_DEADSYNC ) ;
         }
         _hasRegBackup = FALSE ;
      }
      if ( _blockSync )
      {
         _pClsCB->getReplCB()->syncMgr()->enableSync() ;
         _blockSync = FALSE ;
      }
      goto done ;
   }

   INT32 _barBKOfflineLogger::_onWriteMetaFile ()
   {
      return SDB_OK ;
   }

   INT32 _barBKOfflineLogger::_afterBackup ( _pmdEDUCB *cb )
   {
      if ( _hasRegBackup )
      {
         _pDMSCB->backupDown( cb ) ;
         if ( BAR_BACKUP_OP_TYPE_FULL == _metaHeader._opType )
         {
            pmdGetKRCB()->getFTMgr()->unholdStatus( PMD_FT_MASK_DEADSYNC ) ;
         }
         _hasRegBackup = FALSE ;
      }
      if ( _blockSync )
      {
         _pClsCB->getReplCB()->syncMgr()->enableSync() ;
         _blockSync = FALSE ;
      }
      return SDB_OK ;
   }

   INT32 _barBKOfflineLogger::_doBackup ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      if ( BAR_BACKUP_OP_TYPE_FULL == _metaHeader._opType )
      {
         dmsStorageUnit *su = NULL ;
         _SDB_DMSCB::CSCB_ITERATOR itr = _pDMSCB->begin() ;
         while ( itr != _pDMSCB->end() )
         {
            if ( NULL == *itr )
            {
               ++itr ;
               continue ;
            }

            su = (*itr)->_su ;

            // skip the SYSTEMP
            if ( su->data()->isTempSU () )
            {
               ++itr ;
               continue ;
            }

            PD_LOG( PDEVENT, "Begin to backup storage: %s", su->CSName() ) ;

            _curSequence = su->CSSequence() ;

            // backup data file
            _curDataType = BAR_DATA_TYPE_RAW_DATA ;
            _curOffset = 0 ;
            rc = _backupSU( su->data(), cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to backup storage unit[%s] data "
                         "su, rc: %d", su->CSName(), rc ) ;

            // backup index file
            _curDataType = BAR_DATA_TYPE_RAW_IDX ;
            _curOffset = 0 ;
            rc = _backupSU ( su->index(), cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to backup storage unit[%s] "
                         "index su, rc: %d", su->CSName(), rc ) ;

            if ( su->lob()->isOpened() )
            {
               // backup lob meta file
               _curDataType = BAR_DATA_TYPE_RAW_LOBM ;
               _curOffset = 0 ;
               rc = _backupSU( su->lob(), cb ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to backup storage unit[%s] "
                            "lob meta su, rc: %d", su->CSName(), rc ) ;

               // backup lob data file
               _curDataType = BAR_DATA_TYPE_RAW_LOBD ;
               _curOffset = 0 ;
               rc = _backupLobData( su->lob(), cb ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to backup storage unit[%s] "
                            "lob data su, rc: %d", su->CSName(), rc ) ;
            }

            PD_LOG( PDEVENT, "Complete backup storage: %s", su->CSName() ) ;

            _metaHeader._csNum++ ;
            ++itr ;
         }

         rc = _pDMSCB->getStorageService()->backup( *this ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to backup storage service, rc: %d", rc ) ;
      }

      // back up repl-log
      PD_LOG( PDEVENT, "Begin to backup repl-log: %lld",
              _metaHeader._beginLSNOffset ) ;
      rc = _backupLog( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to backup log, rc: %d", rc ) ;

      PD_LOG( PDEVENT, "Complete backup repl-log: %lld",
              _metaHeader._endLSNOffset ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   BSONObj _barBKOfflineLogger::_makeExtentMeta( _dmsStorageBase *pSU )
   {
      BSONObjBuilder builder ;
      builder.append( BAR_SU_NAME, pSU->getSuName() ) ;
      builder.append( BAR_SU_FILE_NAME, pSU->getSuFileName() ) ;
      builder.append( BAR_SU_FILE_OFFSET, (long long)_curOffset ) ;
      builder.append( BAR_SU_SEQUENCE, (INT32)_curSequence ) ;
      if ( BAR_DATA_TYPE_RAW_DATA == _curDataType )
      {
         builder.append( BAR_SU_FILE_TYPE, BAR_SU_FILE_TYPE_DATA ) ;
      }
      else if ( BAR_DATA_TYPE_RAW_IDX == _curDataType )
      {
         builder.append( BAR_SU_FILE_TYPE, BAR_SU_FILE_TYPE_INDEX ) ;
      }
      else if ( BAR_DATA_TYPE_RAW_LOBM == _curDataType )
      {
         builder.append( BAR_SU_FILE_TYPE, BAR_SU_FILE_TYPE_LOBM ) ;
      }
      else
      {
         builder.append( BAR_SU_FILE_TYPE, BAR_SU_FILE_TYPE_LOBD ) ;
      }

      return builder.obj() ;
   }

   BSONObj _barBKOfflineLogger::_makeExtentMeta( _dmsStorageLob *pLobSU )
   {
      BSONObjBuilder builder ;
      builder.append( BAR_SU_NAME, pLobSU->getSuName() ) ;
      builder.append( BAR_SU_FILE_NAME,
                      pLobSU->getLobData()->getFileName() ) ;
      builder.append( BAR_SU_FILE_OFFSET, (long long)_curOffset ) ;
      builder.append( BAR_SU_SEQUENCE, (INT32)_curSequence ) ;
      if ( BAR_DATA_TYPE_RAW_DATA == _curDataType )
      {
         builder.append( BAR_SU_FILE_TYPE, BAR_SU_FILE_TYPE_DATA ) ;
      }
      else if ( BAR_DATA_TYPE_RAW_IDX == _curDataType )
      {
         builder.append( BAR_SU_FILE_TYPE, BAR_SU_FILE_TYPE_INDEX ) ;
      }
      else if ( BAR_DATA_TYPE_RAW_LOBM == _curDataType )
      {
         builder.append( BAR_SU_FILE_TYPE, BAR_SU_FILE_TYPE_LOBM ) ;
      }
      else
      {
         builder.append( BAR_SU_FILE_TYPE, BAR_SU_FILE_TYPE_LOBD ) ;
      }

      return builder.obj() ;
   }

   INT32 _barBKOfflineLogger::_nextThinCopyInfo( dmsStorageBase *pSU,
                                                 UINT32 startExtID,
                                                 UINT32 maxExtID,
                                                 UINT32 maxNum,
                                                 UINT32 &num,
                                                 BOOLEAN &used )
   {
      INT32 rc = SDB_OK ;
      const dmsSpaceManagementExtent *pSME = pSU->getSME() ;
      num   = 0 ;
      used  = FALSE ;
      CHAR  flag = DMS_SME_ALLOCATED ;

      if ( startExtID >= pSU->pageNum() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "start extent id[%u] is more than total data "
                 "pages[%u]", startExtID, pSU->pageNum() ) ;
         goto error ;
      }

      if ( startExtID >= maxExtID || num >= maxNum )
      {
         goto done ;
      }

      if ( DMS_SME_ALLOCATED == pSME->getBitMask( startExtID ) )
      {
         used = TRUE ;
      }
      else
      {
         used = FALSE ;
      }
      ++num ;

      while ( num < maxNum && startExtID + num < maxExtID )
      {
         flag = pSME->getBitMask( startExtID + num ) ;

         if ( ( used && DMS_SME_ALLOCATED != flag ) ||
              ( !used && DMS_SME_ALLOCATED == flag ) )
         {
            break ;
         }
         ++num ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _barBKOfflineLogger::_backupSU( _dmsStorageBase *pSU,
                                         _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      BSONObj metaObj ;
      barBackupExtentHeader *pHeader = NULL ;
      ossValuePtr ptr   = 0 ;
      UINT32 length     = 0 ;
      BOOLEAN thinCopy  = FALSE ;

      // thin copy related
      UINT32 segmentID = 0 ;
      UINT32 curExtentID = 0 ;
      UINT32 maxExtNum = BAR_MAX_EXTENT_DATA_SIZE >> pSU->pageSizeSquareRoot() ;

      // sync memory to mmap
      pSU->syncMemToMmap() ;

      // judge need to thin copy
      if ( pSU->dataSize() > ((UINT64)BAR_THINCOPY_THRESHOLD_SIZE << 20 ) )
      {
         FLOAT64 ratio = (FLOAT64)pSU->getSMEMgr()->totalFree() /
                         (FLOAT64)pSU->pageNum() ;
         if ( ratio >= BAR_THINCOPY_THRESHOLD_RATIO )
         {
            thinCopy = TRUE ;
         }
      }

      UINT32 pos = pSU->begin() ;
      ossMmapFile::ossMmapSegment *pSegment = NULL ;
      while ( NULL != ( pSegment = pSU->next( pos ) ) )
      {
         if ( cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         ptr = pSegment->_ptr ;
         length = pSegment->_length ;

         if ( segmentID < pSU->dataStartSegID() || !thinCopy )
         {
            while ( length > 0 )
            {
               pHeader = _nextDataExtent( _curDataType ) ;
               pHeader->_dataSize = length < BAR_MAX_EXTENT_DATA_SIZE ?
                                    length : BAR_MAX_EXTENT_DATA_SIZE ;
               metaObj = _makeExtentMeta( pSU ) ;
               pHeader->setMetaData( metaObj.objdata(), metaObj.objsize() ) ;

               // write extent
               rc = _writeExtent( pHeader, (const CHAR *)ptr ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to write extent, rc: %d",
                            rc ) ;

               length -= pHeader->_dataSize ;
               ptr += pHeader->_dataSize ;
               _curOffset += pHeader->_dataSize ;
            }
         }
         else
         {
            UINT32 num = 0 ;
            BOOLEAN used = FALSE ;
            UINT32 maxExtID = curExtentID + pSU->segmentPages() ;
            while ( curExtentID < maxExtID )
            {
               rc = _nextThinCopyInfo( pSU, curExtentID, maxExtID, maxExtNum,
                                       num, used ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to get next thin copy info, "
                            "rc: %d", rc ) ;

               pHeader = _nextDataExtent( _curDataType ) ;
               pHeader->_dataSize = (UINT64)num << pSU->pageSizeSquareRoot() ;
               pHeader->_thinCopy = used ? 0 : 1 ;
               metaObj = _makeExtentMeta( pSU ) ;
               pHeader->setMetaData( metaObj.objdata(), metaObj.objsize() ) ;

               // write extent
               rc = _writeExtent( pHeader, (const CHAR*)ptr ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to write extent, rc: %d",
                            rc ) ;

               ptr += pHeader->_dataSize ;
               _curOffset += pHeader->_dataSize ;
               curExtentID += num ;
            }
         }

         ++segmentID ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _barBKOfflineLogger::_backupLobData( _dmsStorageLob * pLobSU,
                                              _pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;
      BSONObj metaObj ;
      barBackupExtentHeader *pHeader = NULL ;
      BOOLEAN thinCopy  = FALSE ;
      dmsStorageLobData *pLobData = pLobSU->getLobData() ;
      UINT64 metaLen    = pLobData->getFileSz() - pLobData->getDataSz() ;
      UINT32 readLen    = 0 ;

      // thin copy related
      UINT32 curExtentID = 0 ;
      UINT32 maxExtNum = BAR_MAX_EXTENT_DATA_SIZE >>
                         pLobData->pageSizeSquareRoot() ;

      if ( !_pExtentBuff )
      {
         _pExtentBuff = ( CHAR* )SDB_OSS_MALLOC( BAR_MAX_EXTENT_DATA_SIZE ) ;
         if ( !_pExtentBuff )
         {
            PD_LOG( PDERROR, "Alloc extent buff failed" ) ;
            rc = SDB_OOM ;
            goto error ;
         }
      }

      // judge need to thin copy
      if ( (UINT64)pLobData->getDataSz() >
           ((UINT64)BAR_THINCOPY_THRESHOLD_SIZE << 20 ) )
      {
         FLOAT64 ratio = (FLOAT64)pLobSU->getSMEMgr()->totalFree() /
                         (FLOAT64)pLobSU->pageNum() ;
         if ( ratio >= BAR_THINCOPY_THRESHOLD_RATIO )
         {
            thinCopy = TRUE ;
         }
      }

      while ( _curOffset < (UINT64)pLobData->getFileSz() )
      {
         if ( cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         if ( _curOffset < metaLen || !thinCopy )
         {
            UINT64 onceLen = 0 ;
            if ( _curOffset < metaLen )
            {
               onceLen = metaLen ;
            }
            else
            {
               onceLen = _curOffset + pLobData->getSegmentSize() ;
               if ( onceLen > (UINT64)pLobData->getFileSz() )
               {
                  onceLen = pLobData->getFileSz() ;
               }
            }
            while ( _curOffset < onceLen )
            {
               pHeader = _nextDataExtent( _curDataType ) ;
               pHeader->_dataSize = onceLen - _curOffset <
                                    BAR_MAX_EXTENT_DATA_SIZE ?
                                    onceLen - _curOffset :
                                    BAR_MAX_EXTENT_DATA_SIZE ;
               metaObj = _makeExtentMeta( pLobSU ) ;
               pHeader->setMetaData( metaObj.objdata(), metaObj.objsize() ) ;

               // read data
               rc = pLobData->readRaw( _curOffset, pHeader->_dataSize,
                                       _pExtentBuff, readLen, cb, FALSE ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Read lob file[%s, offset: %lld, len: %lld] "
                          "failed, rc: %d", pLobData->getFileName(),
                          _curOffset, pHeader->_dataSize, rc ) ;
                  goto error ;
               }
               else if ( readLen != pHeader->_dataSize )
               {
                  rc = SDB_SYS ;
                  PD_LOG( PDERROR, "Read lob file[%s, offset: %lld, len: %lld] "
                          "failed[readLen: %d], rc: %d",
                          pLobData->getFileName(), _curOffset,
                          pHeader->_dataSize, readLen, rc ) ;
                  goto error ;
               }

               // write extent
               rc = _writeExtent( pHeader, _pExtentBuff ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to write extent, rc: %d",
                            rc ) ;

               _curOffset += pHeader->_dataSize ;
            }
         }
         else
         {
            UINT32 num = 0 ;
            BOOLEAN used = FALSE ;
            UINT32 maxExtID = curExtentID + pLobSU->dataSegmentPages() ;
            while ( curExtentID < maxExtID )
            {
               rc = _nextThinCopyInfo( pLobSU, curExtentID, maxExtID,
                                       maxExtNum, num, used ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to get next thin copy info, "
                            "rc: %d", rc ) ;

               pHeader = _nextDataExtent( _curDataType ) ;
               pHeader->_dataSize = (UINT64)num << pLobData->pageSizeSquareRoot() ;
               pHeader->_thinCopy = used ? 0 : 1 ;
               metaObj = _makeExtentMeta( pLobSU ) ;
               pHeader->setMetaData( metaObj.objdata(), metaObj.objsize() ) ;

               // read data
               rc = pLobData->readRaw( _curOffset, pHeader->_dataSize,
                                       _pExtentBuff, readLen, cb, FALSE ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Read lob file[%s, offset: %lld, len: %lld] "
                          "failed, rc: %d", pLobData->getFileName(),
                          _curOffset, pHeader->_dataSize, rc ) ;
                  goto error ;
               }
               else if ( readLen != pHeader->_dataSize )
               {
                  rc = SDB_SYS ;
                  PD_LOG( PDERROR, "Read lob file[%s, offset: %lld, len: %lld] "
                          "failed[readLen: %d], rc: %d",
                          pLobData->getFileName(), _curOffset,
                          pHeader->_dataSize, readLen, rc ) ;
                  goto error ;
               }

               // write extent
               rc = _writeExtent( pHeader, _pExtentBuff ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to write extent, rc: %d",
                            rc ) ;

               _curOffset += pHeader->_dataSize ;
               curExtentID += num ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _barBKOfflineLogger::_backupLog( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      DPS_LSN lsn ;
      DPS_LSN_OFFSET oldTransLsn = DPS_INVALID_LSN_OFFSET ;
      barBackupExtentHeader *pHeader = NULL ;
      dpsMessageBlock mb( BAR_MAX_EXTENT_DATA_SIZE ) ;
      BOOLEAN endLoop = FALSE ;
      const dpsLogRecordHeader *pLastLSN = NULL ;

      if ( DPS_INVALID_LSN_OFFSET == _metaHeader._beginLSNOffset )
      {
         goto done ;
      }

      lsn.offset = _metaHeader._beginLSNOffset ;
      while ( !endLoop )
      {
         if ( cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         mb.clear() ;

         while ( mb.length() < BAR_MAX_EXTENT_DATA_SIZE )
         {
            rc = _pDPSCB->search( lsn, &mb ) ;
            if ( SDB_DPS_LSN_OUTOFRANGE == rc )
            {
               rc = SDB_OK ;
               endLoop = TRUE ;
               break ;
            }
            else if ( rc )
            {
               /// Print current dps info
               DPS_LSN fileBegin ;
               DPS_LSN memBegin ;
               DPS_LSN endLsn ;
               DPS_LSN expectLsn ;
               _pDPSCB->getLsnWindow( fileBegin, memBegin, endLsn,
                                      &expectLsn, NULL ) ;
               PD_LOG( PDERROR, "Failed to search lsn[%u,%lld] in "
                       "log[FileBeginLsn:%lld, MemBeginLsn:%lld, EndLsn:%lld,"
                       "ExpectLsn:%lld], rc: %d", lsn.version, lsn.offset,
                       fileBegin.offset, memBegin.offset, endLsn.offset,
                       expectLsn.offset, rc ) ;
               goto error ;
            }
            pLastLSN = (const dpsLogRecordHeader*)mb.readPtr() ;
            mb.readPtr( mb.length() ) ;
            lsn.offset += pLastLSN->_length ;
            lsn.version = pLastLSN->_version ;
         }

         oldTransLsn = _pTransCB->getOldestBeginLsn() ;
         if ( 0 == mb.length() )
         {
            break ;
         }

         pHeader = _nextDataExtent( BAR_DATA_TYPE_REPL_LOG ) ;
         pHeader->_dataSize = mb.length() ;

         /// write extent
         rc = _writeExtent( pHeader, mb.startPtr() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to write extent, rc: %d", rc ) ;
      }

      /// check lsn
      if ( lsn.compareOffset( _metaHeader._endLSNOffset ) < 0 )
      {
         PD_LOG( PDWARNING, "Real end lsn[%u,%llu] is smaller than expect "
                 "end lsn[%llu]", lsn.version, lsn.offset,
                 _metaHeader._endLSNOffset ) ;
         _metaHeader._endLSNOffset = lsn.offset ;
      }
      else if ( lsn.compareOffset( _metaHeader._endLSNOffset ) > 0 )
      {
         PD_LOG( PDINFO, "Real end lsn[%u,%llu] is grater than expect "
                 "end lsn[%llu]", lsn.version, lsn.offset,
                 _metaHeader._endLSNOffset ) ;
         _metaHeader._endLSNOffset = lsn.offset ;
      }
      /// check trans lsn
      if ( oldTransLsn != _metaHeader._transLSNOffset )
      {
         PD_LOG( PDWARNING, "Old trans lsn[%lld] is not the same with "
                 "expect trans lsn[%lld]", oldTransLsn,
                 _metaHeader._transLSNOffset ) ;
         _metaHeader._transLSNOffset = oldTransLsn ;
      }

      if ( pLastLSN )
      {
         _metaHeader._lastLSN = pLastLSN->_lsn ;
         _metaHeader._lastLSNCode = ossHash( (const CHAR*)pLastLSN,
                                             pLastLSN->_length ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _barRSBaseLogger implement
   */
   _barRSBaseLogger::_barRSBaseLogger ()
   {
      _metaFileSeq         = 0 ;
      _secretValue         = 0 ;
      _expectExtID         = 1 ;
      _curDataFileSeq      = 0 ;

      _isOpened            = FALSE ;
      _curOffset           = 0 ;
      _pBuff               = NULL ;
      _buffSize            = 0 ;
      _beginID             = -1 ;

      _isDoRestoring       = FALSE ;
      _skipConf            = FALSE ;
   }

   _barRSBaseLogger::~_barRSBaseLogger ()
   {
      // ignore the rc - this is a destructor and can't handle failure
      _closeCurFile() ;
      if ( _pBuff )
      {
         SDB_OSS_FREE( _pBuff ) ;
         _pBuff = NULL ;
         _buffSize = 0 ;
      }
   }

   void _barRSBaseLogger::_reset ()
   {
      _closeCurFile() ;
      _expectExtID = 1 ;
      _curDataFileSeq = 0 ;
      _curOffset = 0 ;
   }

   INT32 _barRSBaseLogger::_allocBuff( UINT64 buffSize )
   {
      if ( buffSize <= _buffSize )
      {
         return SDB_OK ;
      }
      if ( _pBuff )
      {
         SDB_OSS_FREE( _pBuff ) ;
         _pBuff = NULL ;
         _buffSize = 0 ;
      }
      _pBuff = ( CHAR* )SDB_OSS_MALLOC( buffSize ) ;
      if ( !_pBuff )
      {
         PD_LOG( PDERROR, "Failed to alloc memory, size: %llu", buffSize ) ;
         return SDB_OOM ;
      }
      _buffSize = buffSize ;
      return SDB_OK ;
   }

   INT32 _barRSBaseLogger::_closeCurFile ()
   {
      INT32 rc = SDB_OK ;
      if ( _isOpened )
      {
         rc = _close( _curFile, FALSE ) ;
         _isOpened = FALSE ;
      }
      return rc ;
   }

   INT32 _barRSBaseLogger::_openDataFile ()
   {
      if ( _isOpened )
      {
         return SDB_OK ;
      }
      ++_curDataFileSeq ;
      barBackupDataHeader *pHeader = NULL ;
      string fileName = getDataFileName( _curDataFileSeq ) ;

      if ( _isDoRestoring )
      {
         std::cout << "Begin to restore data file: " << fileName.c_str()
                   << " ..." << std::endl ;
         PD_LOG( PDEVENT, "Begin to restore data file[%s]", fileName.c_str() ) ;
      }

      // is it really necessary to open with OSS_WU?
      INT32 rc = ossOpen( fileName.c_str(), OSS_READONLY,
                          OSS_RU | OSS_WU | OSS_RG, _curFile ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to open file[%s], rc: %d", fileName.c_str(),
                 rc ) ;
         goto error ;
      }
      _isOpened = TRUE ;

      // read header
      pHeader = SDB_OSS_NEW barBackupDataHeader ;
      if ( !pHeader )
      {
         PD_LOG( PDERROR, "Failed to alloc memory for backup data header" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = _readDataHeader( _curFile, fileName, pHeader, TRUE, _secretValue,
                            _curDataFileSeq ) ;
      if ( rc )
      {
         goto error ;
      }
      _curOffset = BAR_BACKUPDATA_HEADER_SIZE ;
      _pCompressor = getCompressorByType(
         (UTIL_COMPRESSOR_TYPE)pHeader->_compressionType ) ;

   done:
      if ( pHeader )
      {
         SDB_OSS_DEL pHeader ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _barRSBaseLogger::init( const CHAR *path, const CHAR *backupName,
                                 const CHAR *prefix, INT32 incID,
                                 INT32 beginID, BOOLEAN skipConf )
   {
      INT32 rc = SDB_OK ;
      set< UINT32 > setSeq ;

      _beginID = beginID ;
      _skipConf = skipConf ;

      rc = _initInner( path, backupName, prefix ) ;
      PD_RC_CHECK( rc, PDWARNING, "Init inner failed, rc: %d", rc ) ;

      if ( !backupName || 0 == ossStrlen( backupName ) )
      {
         PD_LOG( PDWARNING, "Backup name can't be empty" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // 1. ensure meta file seq
      _metaFileSeq = _ensureMetaFileSeq( &setSeq ) ;

      // 2. init check and prepare for restore
      rc = _initCheckAndPrepare ( incID, beginID, setSeq ) ;
      PD_RC_CHECK( rc, PDWARNING, "Prepare check for resotre failed, rc: %d",
                   rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _barRSBaseLogger::_initCheckAndPrepare( INT32 incID,
                                                 INT32 beginID,
                                                 set< UINT32 > &setSeq )
   {
      INT32 rc = SDB_OK ;
      UINT32 tmpID = 0 ;
      set< UINT32 >::iterator it ;
      barBackupHeader *pHeader = NULL ;

      pHeader = SDB_OSS_NEW barBackupHeader ;
      if ( !pHeader )
      {
         PD_LOG( PDERROR, "Alloc backup data header failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      // 1. backup file exist check
      if ( 0 == _metaFileSeq || setSeq.empty() )
      {
         rc = SDB_BAR_BACKUP_NOTEXIST ;
         PD_LOG( PDWARNING, "Full backup[%s] not exist", backupName() ) ;
         goto error ;
      }
      --_metaFileSeq ;

      if ( incID >= 0 )
      {
         if ( (UINT32)incID > _metaFileSeq ||
              0 == setSeq.count( incID ) )
         {
            rc = SDB_BAR_BACKUP_NOTEXIST ;
            PD_LOG( PDWARNING, "Increase backup[%s,%d] does not exist",
                    backupName(), incID ) ;
            goto error ;
         }
         _metaFileSeq = (UINT32)incID ;
      }

      /// check value
      if ( beginID >= 0 )
      {
         if ( (UINT32)beginID > _metaFileSeq )
         {
            PD_LOG( PDERROR, "Backup begin increase id[%d] is grather than "
                    "backup end increase id[%d]", beginID, _metaFileSeq ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         else if ( 0 == setSeq.count( beginID ) )
         {
            rc = SDB_BAR_BACKUP_NOTEXIST ;
            PD_LOG( PDERROR, "Begin increase backup[%s,%d] does not exist",
                    backupName(), beginID ) ;
            goto error ;
         }
         tmpID = beginID ;
      }
      else
      {
         it = setSeq.begin() ;
         tmpID = *it ;
      }

      // 2. read meta file header
      rc = _updateFromMetafile( _metaFileSeq ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to read meta file[%d], rc: %d",
                   _metaFileSeq, rc ) ;

      // 3. init _mapBackupInfo
      while( tmpID < _metaFileSeq )
      {
         rc = _readMetaHeader( tmpID, pHeader, TRUE, _secretValue ) ;
         PD_RC_CHECK( rc, PDERROR, "Read meta header[Name:%s,ID:%d] failed, "
                      "rc: %d", backupName(), tmpID, rc ) ;
         _mapBackupInfo[ tmpID ] = barBackupHeaderSimple( pHeader ) ;

         if ( beginID >= 0 )
         {
            break ;
         }
         else if ( it != setSeq.end() )
         {
            ++it ;
            tmpID = *it ;
         }
         else
         {
            break ;
         }
      }
      _mapBackupInfo[ _metaFileSeq ] = barBackupHeaderSimple( &_metaHeader ) ;

      // 4. load config
      rc = _loadConf() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to load config, rc: %d", rc ) ;

      // 5. reset
      _reset() ;

   done:
      if ( pHeader )
      {
         SDB_OSS_DEL pHeader ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _barRSBaseLogger::_updateFromMetafile( UINT32 incID )
   {
      INT32 rc = SDB_OK ;

      rc = _readMetaHeader( incID, &_metaHeader, TRUE, _secretValue ) ;
      if ( rc )
      {
         goto error ;
      }

      // update info
      if ( 0 == _secretValue )
      {
         _secretValue = _metaHeader._secretValue ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _barRSBaseLogger::_loadConf ()
   {
      INT32 rc = SDB_OK ;
      CHAR *pBuff = NULL ;
      barBackupExtentHeader *pExtHeader = NULL ;

      _curDataFileSeq = _metaHeader._lastDataSequence -
                        _metaHeader._dataFileNum ;
      _expectExtID = 0 ;

      rc = _readData( &pExtHeader, &pBuff ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to read config, rc: %d" ) ;

      if ( !pExtHeader || !pBuff )
      {
         PD_LOG( PDERROR, "Invalid backup data file and meta file" ) ;
         rc = SDB_BAR_DAMAGED_BK_FILE ;
         goto error ;
      }
      else if ( BAR_DATA_TYPE_CONFIG != pExtHeader->_dataType )
      {
         PD_LOG( PDERROR, "Extent data type[%d] error",
                 pExtHeader->_dataType ) ;
         rc = SDB_BAR_DAMAGED_BK_FILE ;
         goto error ;
      }

      try
      {
         _confObj = BSONObj( pBuff ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception when read config: %s", e.what() ) ;
         rc = SDB_BAR_DAMAGED_BK_FILE ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _barRSBaseLogger::restore( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN prepared = FALSE ;
      BOOLEAN isEmpty = FALSE ;

      PD_LOG( PDEVENT, "Begin to restore[%s]...", backupName() ) ;

      // 1. prepare for restore
      rc = _prepareRestore( cb, isEmpty ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to prepare for restore, rc: %d", rc ) ;

      prepared = TRUE ;

      if ( !isEmpty )
      {
         // 2. restore config
         rc = _restoreConfig () ;
         PD_RC_CHECK( rc, PDERROR, "Failed to restore config, rc: %d", rc ) ;

         _isDoRestoring = TRUE ;
         // 3. do restore data
         rc = _doRestore( cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to do restore, rc: %d", rc ) ;
      }

      // 4. after restore
      rc = _afterRestore( cb ) ;
      prepared = FALSE ;
      PD_RC_CHECK( rc, PDERROR, "Failed to clean up after restore", rc ) ;

      pmdGetStartup().ok( TRUE ) ;
      pmdGetStartup().final() ;

      PD_LOG( PDEVENT, "Complete restore[%s]", backupName() ) ;

   done:
      PMD_SHUTDOWN_DB( rc ) ;
      return rc ;
   error:
      {
         INT32 tempRC = SDB_OK ;
         /// only when restore full need to rollback
         if ( 0 == _beginID )
         {
            tempRC = cleanDMSData () ;
            if ( tempRC )
            {
               PD_LOG( PDWARNING, "Rollback restore failed, rc: %d", tempRC ) ;
            }
            _pDPSCB->move( 0, 1 ) ;

            _metaHeader._transLSNOffset = DPS_INVALID_LSN_OFFSET ;
         }

         if ( prepared )
         {
            tempRC = _afterRestore( cb ) ;
            if ( tempRC )
            {
               PD_LOG( PDWARNING, "Failed to clean up after restore", tempRC ) ;
            }
         }
      }
      goto done ;
   }

   INT32 _barRSBaseLogger::cleanDMSData ()
   {
      INT32 rc = SDB_OK ;
      MON_CS_LIST csList ;

      PD_LOG ( PDEVENT, "Clear dms data for restore" ) ;

      //dump all collectionspace
      _pDMSCB->dumpInfo( csList, TRUE ) ;
      MON_CS_LIST::const_iterator it = csList.begin() ;
      while ( it != csList.end() )
      {
         const _monCollectionSpace &cs = *it ;
         rc = rtnDropCollectionSpaceCommand ( cs._name, NULL, _pDMSCB, NULL,
                                              TRUE ) ;
         if ( SDB_OK != rc && SDB_DMS_CS_NOTEXIST != rc )
         {
            PD_LOG ( PDERROR, "Clear collectionspace[%s] failed[rc:%d]",
                     cs._name, rc ) ;
            break ;
         }
         PD_LOG ( PDDEBUG, "Clear collectionspace[%s] succeed", cs._name ) ;
         ++it ;
      }
      return rc ;
   }

   INT32 _barRSBaseLogger::_restoreConfig ()
   {
      INT32 rc = SDB_OK ;

      if ( !_skipConf )
      {
         if ( 0 != ossAccess( _pOptCB->getConfPath() ) )
         {
            rc = ossMkdir( _pOptCB->getConfPath() ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Create config directory[%s] failed, rc: %d",
                       _pOptCB->getConfPath(), rc ) ;
               goto error ;
            }
         }
         rc = _pOptCB->reflush2File() ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Flush config to file failed, rc: %d", rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _barRSBaseLogger::_readData( barBackupExtentHeader **pExtHeader,
                                      CHAR **pBuff )
   {
      INT32 rc = SDB_OK ;
      *pExtHeader = NULL ;
      *pBuff = NULL ;

      // finished
      if ( _expectExtID > _metaHeader._lastExtentID )
      {
         if ( _curDataFileSeq != _metaHeader._lastDataSequence )
         {
            PD_LOG( PDERROR, "Invalid backup file, expect extent id: %llu, "
                    "meta header last extent id: %llu, cur file seq: %d, meta "
                    "header last data seq: %d", _expectExtID,
                    _metaHeader._lastExtentID, _curDataFileSeq,
                    _metaHeader._lastDataSequence ) ;
            rc = SDB_BAR_DAMAGED_BK_FILE ;
            goto error ;
         }
         goto done ;
      }

   retry:
      if ( !_isOpened )
      {
         rc = _openDataFile() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open data file, rc: %d", rc ) ;
      }

      // read extent header
      rc = _read( _curFile, (CHAR*)_pDataExtent,
                  BAR_BACKUP_EXTENT_HEADER_SIZE ) ;
      if ( SDB_EOF == rc )
      {
         if ( _curDataFileSeq < _metaHeader._lastDataSequence )
         {
            _closeCurFile() ;
            goto retry ;
         }
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to read data extent header, offset: "
                   "%llu, expect extent id: %llu, data seq: %d, last extent "
                   "id: %llu, rc: %d", _curOffset, _expectExtID,
                   _curDataFileSeq, _metaHeader._lastExtentID, rc ) ;

      _curOffset += BAR_BACKUP_EXTENT_HEADER_SIZE ;

      // check
      if ( 0 != ossStrncmp( _pDataExtent->_eyeCatcher, BAR_EXTENT_EYECATCH,
                           BAR_BACKUP_HEADER_EYECATCHER_LEN ) )
      {
         PD_LOG( PDERROR, "Extent eyecatcher[%s] is invalid, offset: %llu, "
                 "data seq: %d", _pDataExtent->_eyeCatcher, _curOffset,
                 _curDataFileSeq ) ;
         rc = SDB_BAR_DAMAGED_BK_FILE ;
         goto error ;
      }
      if ( 0 != _expectExtID )
      {
         if ( _pDataExtent->_extentID != _expectExtID )
         {
            PD_LOG( PDERROR, "Extent id[%llu] is not expect[%llu], offset: "
                    "%llu, data seq: %d", _pDataExtent->_extentID, _expectExtID,
                    _curOffset, _curDataFileSeq ) ;
            rc = SDB_BAR_DAMAGED_BK_FILE ;
            goto error ;
         }
         ++_expectExtID ;
      }
      else
      {
         _expectExtID = _pDataExtent->_extentID + 1 ;
      }

      *pExtHeader = _pDataExtent ;

      // read data
      if ( _pDataExtent->_dataSize > 0 )
      {
         rc = _allocBuff( _pDataExtent->_dataSize ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to alloc memory, rc: %d", rc ) ;

         if ( 0 == _pDataExtent->_thinCopy )
         {
            rc = _read( _curFile, _pBuff, _pDataExtent->_dataSize ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to read extent data, "
                         "offset: %llu, data seq: %d, rc: %d", _curOffset,
                         _curDataFileSeq, rc ) ;
            _curOffset += _pDataExtent->_dataSize ;
         }
         else
         {
            ossMemset( _pBuff, 0, _pDataExtent->_dataSize ) ;
         }

         /// uncompress
         if ( 0 != _pDataExtent->_compressed )
         {
            CHAR *pBuffTmp = NULL ;
            UINT32 destLen = BAR_MAX_EXTENT_DATA_SIZE ;
            UINT32 unCompLen = 0 ;

            if ( !_pCompressor )
            {
               PD_LOG( PDERROR, "Compressor is NULL" ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            rc = _pCompressor->getUncompressedLen( _pBuff,
                                                   _pDataExtent->_dataSize,
                                                   unCompLen ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get uncompressed length, "
                         "rc: %d", rc ) ;

            if ( unCompLen > destLen )
            {
               destLen = unCompLen ;
            }

            pBuffTmp = _allocCompressBuff( destLen ) ;
            if ( !pBuffTmp )
            {
               PD_LOG( PDERROR, "Alloc uncompressed buffer failed" ) ;
               rc = SDB_OOM ;
               goto error ;
            }
            // uncompress data
            rc = _pCompressor->decompress( _pBuff, _pDataExtent->_dataSize,
                                           pBuffTmp, destLen, NULL ) ;
            PD_RC_CHECK( rc, PDERROR, "Uncompress data failed, rc: %d",
                         rc ) ;

            *pBuff = pBuffTmp ;
            _pDataExtent->_dataSize = destLen ;
         }
         else
         {
            *pBuff = _pBuff ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _barRSOfflineLogger implement
   */
   _barRSOfflineLogger::_barRSOfflineLogger ()
   {
      _curSUOffset      = 0 ;
      _curSUSequence    = 0 ;
      _openedSU         = FALSE ;
      _incDataFileBeginSeq = (UINT32)-1 ;
      _hasLoadDMS       = FALSE ;
   }

   _barRSOfflineLogger::~_barRSOfflineLogger ()
   {
      // ignore the rc - this is a destructor and can't handle failure
      _closeSUFile () ;
      _replBucket.fini() ;
   }

   INT32 _barRSOfflineLogger::_closeSUFile ()
   {
      INT32 rc = SDB_OK ;
      if ( _openedSU )
      {
         // SU files are opened for writing during restore, so set fsync
         rc = _close( _curSUFile, TRUE ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Error syncing and closing su file" ) ;
         }
         _openedSU      = FALSE ;
         _curSUName     = "" ;
         _curSUOffset   = 0 ;
         _curSUSequence = 0 ;
         _curSUFileName = "" ;
      }
      return rc ;
   }

   INT32 _barRSOfflineLogger::_parseExtentMeta( barBackupExtentHeader *pExtHeader,
                                                string &suName,
                                                string & suFileName,
                                                UINT32 & sequence,
                                                UINT64 & offset )
   {
      INT32 rc = SDB_OK ;

      if ( NULL == pExtHeader->getMetaData() )
      {
         PD_LOG( PDERROR, "Invalid raw data, meta data can't be empty" ) ;
         rc = SDB_BAR_DAMAGED_BK_FILE ;
         goto error ;
      }

      try
      {
         BSONObj metaObj( pExtHeader->getMetaData() ) ;
         BSONElement ele = metaObj.getField( BAR_SU_NAME ) ;
         if ( ele.type() != String )
         {
            PD_LOG( PDERROR, "Field[%s] type[%d] error in [%s]",
                    BAR_SU_NAME, ele.type(), metaObj.toString().c_str() ) ;
            rc = SDB_BAR_DAMAGED_BK_FILE ;
            goto error ;
         }
         suName = ele.str() ;
         ele = metaObj.getField( BAR_SU_FILE_NAME ) ;
         if ( ele.type() != String )
         {
            PD_LOG( PDERROR, "Field[%s] type[%d] error in [%s]",
                    BAR_SU_FILE_NAME, ele.type(), metaObj.toString().c_str() ) ;
            rc = SDB_BAR_DAMAGED_BK_FILE ;
            goto error ;
         }
         suFileName = ele.str() ;
         ele = metaObj.getField( BAR_SU_FILE_OFFSET ) ;
         if ( ele.type() != NumberLong )
         {
            PD_LOG( PDERROR, "Field[%s] type[%d] error in [%s]",
                    BAR_SU_FILE_OFFSET, ele.type(), metaObj.toString().c_str() ) ;
            rc = SDB_BAR_DAMAGED_BK_FILE ;
            goto error ;
         }
         offset = (UINT64)ele.numberLong() ;
         ele = metaObj.getField( BAR_SU_SEQUENCE ) ;
         if ( ele.type() != NumberInt )
         {
            PD_LOG( PDERROR, "Field[%s] type[%d] error in [%s]",
                    BAR_SU_SEQUENCE, ele.type(), metaObj.toString().c_str() ) ;
            rc = SDB_BAR_DAMAGED_BK_FILE ;
            goto error ;
         }
         sequence = (UINT32)ele.numberInt() ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception when get meta obj: %s", e.what() ) ;
         rc = SDB_BAR_DAMAGED_BK_FILE ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _barRSOfflineLogger::_prepareRestore( pmdEDUCB * cb,
                                               BOOLEAN &isEmpty )
   {
      INT32 rc = SDB_OK ;
      MAP_BACKUP_INFO_IT it ;
      barBackupHeaderSimple *pInfo = NULL ;
      DPS_LSN expectLSN = _pDPSCB->expectLsn() ;
      DPS_LSN_OFFSET beginLSN = DPS_INVALID_LSN_OFFSET ;

      isEmpty = FALSE ;

      /// init repl bucket
      rc = _replBucket.init() ;
      PD_RC_CHECK( rc, PDERROR, "Init repl bucket failed, rc: %d", rc ) ;

      /// find the right beginID
      if ( 0 == expectLSN.offset && _beginID < 0 )
      {
         _beginID = 0 ;
      }

      if ( _beginID < 0 )
      {
         it = _mapBackupInfo.begin() ;
         while( it != _mapBackupInfo.end() )
         {
            pInfo = &(it->second) ;
            if ( expectLSN.compareOffset( pInfo->_beginLSNOffset ) < 0 )
            {
               PD_LOG( PDERROR, "Data node's expect lsn[%lld] is less than "
                       "backup[Name:%s,ID:%d]'s begin lsn[%lld]",
                       expectLSN.offset, backupName(), it->first,
                       pInfo->_beginLSNOffset ) ;
               std::cout << "Data node's expect lsn[" << expectLSN.offset
                         << "] is less than backup[Name:" << backupName()
                         << ",ID" << it->first << "] 's begin lsn["
                         << pInfo->_beginLSNOffset << "]" << std::endl ;
               rc = SDB_SYS ;
               goto error ;
            }
            else if ( expectLSN.compareOffset( pInfo->_endLSNOffset ) < 0 )
            {
               /// find it
               _beginID = it->first ;
               PD_LOG( PDEVENT, "Find the begin increase id[%d]", _beginID ) ;
               std::cout << "Find the begin increase id: " << _beginID
                         << std::endl ;
               break ;
            }
            else
            {
               pInfo = NULL ;
               ++it ;
            }
         }
      }
      else
      {
         it = _mapBackupInfo.find( _beginID ) ;
         if ( it == _mapBackupInfo.end() )
         {
            PD_LOG( PDERROR, "The begin backup[Name:%s,ID:%d] is not exist",
                    backupName(), _beginID ) ;
            rc = SDB_BAR_BACKUP_NOTEXIST ;
            goto error ;
         }
         pInfo = &( it->second ) ;
      }

      if ( !pInfo || _beginID < 0 )
      {
         PD_LOG( PDEVENT, "Data node[LSN:%lld] is newer than or the "
                 "same with backup[Name:%s,ID:%d,LSN:%lld]",
                 expectLSN.offset, backupName(), _metaFileSeq,
                 _metaHeader._endLSNOffset ) ;
         std::cout << "Data node[LSN:" << expectLSN.offset
                   << "] is newer than or the same with backup[Name:"
                   << backupName() << ",ID:" << _metaFileSeq
                   << ",LSN:" << _metaHeader._endLSNOffset
                   << "], don't need to restore."
                   << "You can use param '-b 0' to force full restore."
                   << std::endl ;

         isEmpty = TRUE ;
         _metaHeader._transLSNOffset = DPS_INVALID_LSN_OFFSET ;
         goto done ;
      }

      /// full restore
      if ( 0 == _beginID )
      {
         _incDataFileBeginSeq = pInfo->_lastDataSequence + 1 ;
         beginLSN = pInfo->_beginLSNOffset ;
         rc = cleanDMSData() ;
         PD_RC_CHECK( rc, PDERROR, "Clear all data failed, rc: %d", rc ) ;
      }
      /// inc restore
      else
      {
         _curDataFileSeq = pInfo->_lastDataSequence - pInfo->_dataFileNum ;
         _incDataFileBeginSeq = _curDataFileSeq + 1 ;
         _expectExtID = 0 ;

         if ( expectLSN.offset != pInfo->_beginLSNOffset )
         {
            PD_LOG( PDERROR, "Data node's expect lsn[%lld] is not the "
                    "same with backup[Name:%s,ID:%d]'s begin lsn[%lld]",
                    expectLSN.offset, backupName(), it->first,
                    pInfo->_beginLSNOffset ) ;
            std::cout << "Data node's expect lsn[" << expectLSN.offset
                      << "] is not the same with backup[Name:"
                      << backupName() << ",ID:" << it->first
                      << "]'s begin lsn[" << pInfo->_beginLSNOffset
                      << "], can't do restore." << std::endl ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

      /// move lsn
      if ( DPS_INVALID_LSN_OFFSET != beginLSN )
      {
         rc = _pDPSCB->move( beginLSN, 1 ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to move dps to %lld, rc: %d",
                      beginLSN, rc ) ;
      }

   done:
      /// forbidden trans rollback
      _metaHeader._transLSNOffset = DPS_INVALID_LSN_OFFSET ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _barRSOfflineLogger::_doRestore( pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;
      barBackupExtentHeader *pExtHeader = NULL ;
      CHAR *pBuff = NULL ;
      BOOLEAN restoreDPS = FALSE ;
      BOOLEAN restoreInc = FALSE ;

      // read data
      while ( TRUE )
      {
         if ( cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }
         rc = _readData( &pExtHeader, &pBuff ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to read data, rc: %d", rc ) ;

         // finished
         if ( NULL == pExtHeader )
         {
            break ;
         }

         switch ( pExtHeader->_dataType )
         {
            case BAR_DATA_TYPE_CONFIG :
               rc = _processConfigData( pExtHeader, pBuff ) ;
               break ;
            case BAR_DATA_TYPE_RAW_DATA :
               rc = _processRawData( pExtHeader, pBuff ) ;
               break ;
            case BAR_DATA_TYPE_RAW_IDX :
               rc = _processRawIndex( pExtHeader, pBuff ) ;
               break ;
            case BAR_DATA_TYPE_RAW_LOBM :
               rc = _processRawLobM( pExtHeader, pBuff ) ;
               break ;
            case BAR_DATA_TYPE_RAW_LOBD :
               rc = _processRawLobD( pExtHeader, pBuff ) ;
               break ;
            case BAR_DATA_TYPE_REPL_LOG :
               if ( !restoreDPS )
               {
                  restoreDPS = TRUE ;
                  PD_LOG( PDEVENT, "Begin to restore dps logs..." ) ;
                  std::cout << "Begin to restore dps logs..." << std::endl ;
               }
               if ( !restoreInc && _curDataFileSeq >= _incDataFileBeginSeq )
               {
                  restoreInc = TRUE ;
                  rc = _closeSUFile() ;
                  if ( SDB_OK != rc )
                  {
                     break ;
                  }

                  PD_LOG( PDEVENT, "Begin to load all collection spaces..." ) ;
                  std::cout << "Begin to load all collection spaces..."
                            << std::endl ;
                  // need to load dms
                  pmdGetKRCB()->setIsRestore( FALSE ) ;
                  rc = _pDMSCB->init() ;
                  pmdGetKRCB()->setIsRestore( TRUE ) ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to load collection spaces, "
                               "rc: %d", rc ) ;
                  _hasLoadDMS = TRUE ;
               }
               rc = _processReplLog( pExtHeader, pBuff, restoreInc, cb ) ;
               break ;
            default :
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Unknow data type[%d]", pExtHeader->_dataType ) ;
               break ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to process data extent[type:%d, "
                      "id:%lld], data file seq: %d, offset: %lld, rc: %d",
                      pExtHeader->_dataType, pExtHeader->_extentID,
                      _curDataFileSeq, _curOffset, rc ) ;
      }

   done:
      {
         INT32 rc2 = _closeSUFile() ;
         if ( SDB_OK == rc )
         {
            rc = rc2 ;
         }
      }
      /// wait all log complete
      if ( _replBucket.maxReplSync() > 0 )
      {
         PD_LOG( PDEVENT, "Wait repl bucket empty completed" ) ;
         _replBucket.close() ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _barRSOfflineLogger::_processConfigData( barBackupExtentHeader * pExtHeader,
                                                  const CHAR * pData )
   {
      return SDB_OK ;
   }

   INT32 _barRSOfflineLogger::_processRawData( barBackupExtentHeader * pExtHeader,
                                               const CHAR * pData )
   {
      return _writeSU( pExtHeader, pData ) ;
   }

   INT32 _barRSOfflineLogger::_processRawIndex( barBackupExtentHeader * pExtHeader,
                                                const CHAR * pData )
   {
      return _writeSU( pExtHeader, pData ) ;
   }

   INT32 _barRSOfflineLogger::_processRawLobM( barBackupExtentHeader * pExtHeader,
                                               const CHAR * pData )
   {
      return _writeSU( pExtHeader, pData ) ;
   }

   INT32 _barRSOfflineLogger::_processRawLobD( barBackupExtentHeader * pExtHeader,
                                               const CHAR * pData )
   {
      return _writeSU( pExtHeader, pData ) ;
   }

   INT32 _barRSOfflineLogger::_processReplLog( barBackupExtentHeader * pExtHeader,
                                               const CHAR * pData,
                                               BOOLEAN isIncData,
                                               _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      INT32 replayRC = SDB_OK ;
      const CHAR *pLogIndex = NULL ;
      DPS_LSN expectLSN ;
      DPS_LSN completeLSN ;
      DPS_LSN_OFFSET firstOffset = DPS_INVALID_LSN_OFFSET ;
      BOOLEAN inRetry = FALSE ;
      _clsReplayer replayer ;

   retry:
      pLogIndex = pData ;
      while ( pLogIndex < pData + pExtHeader->_dataSize )
      {
         BOOLEAN canSkip = FALSE ;
         dpsLogRecordHeader *pHeader = (dpsLogRecordHeader *)pLogIndex ;
         expectLSN = _pDPSCB->expectLsn() ;

         canSkip = ( expectLSN.compareOffset( pHeader->_lsn ) > 0 ) ;

         if ( isIncData && canSkip && inRetry )
         {
            // in retry, just skip
            pLogIndex += pHeader->_length ;
            continue ;
         }

         // judge lsn valid
         if ( 0 != expectLSN.compareOffset( pHeader->_lsn ) )
         {
            PD_LOG( PDERROR, "Expect lsn[%lld] is not the same with cur "
                    "lsn[%lld]", expectLSN.offset, pHeader->_lsn ) ;
            rc = SDB_BAR_DAMAGED_BK_FILE ;
            goto error ;
         }

         // if in inc backup data file
         if ( isIncData )
         {
            if ( DPS_INVALID_LSN_OFFSET == firstOffset )
            {
               firstOffset = pHeader->_lsn ;
            }

            if ( _replBucket.maxReplSync() > 0 )
            {
               rc = replayer.replayByBucket( pHeader, cb, &_replBucket ) ;
            }
            else
            {
               // should ignore duplicated keys on user indexes
               // NOTE: data backup from secondary node with parallel replay
               //       mode may contain duplicated keys
               rc = replayer.replay( pHeader, cb, TRUE, TRUE ) ;
            }
            if ( rc )
            {
               PD_LOG( PDERROR, "Failed to reply log[ lsn: %lld, type: "
                       "%d, len: %d ], rc: %d", pHeader->_lsn, pHeader->_type,
                       pHeader->_length, rc ) ;
               replayRC = rc ;
               goto error ;
            }
         }
         // write lsn
         rc = _pDPSCB->recordRow( pLogIndex, pHeader->_length ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to write lsn[%d,%lld], length: %d, "
                      "rc: %d", pHeader->_version, pHeader->_lsn,
                      pHeader->_length, rc ) ;
         // update trans info
         if ( DPS_INVALID_LSN_OFFSET != _metaHeader._transLSNOffset )
         {
            dpsLogRecord record ;
            rc = record.load( pLogIndex ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to load logRecord, lsn[%d,%lld], "
                         "type: %d, rc: %d", pHeader->_version, pHeader->_lsn,
                         pHeader->_type, rc ) ;
            _pTransCB->saveTransInfoFromLog( record ) ;
         }

         pLogIndex += pHeader->_length ;
      }

   done:
      if ( isIncData && _replBucket.maxReplSync() > 0 )
      {
         // wait for bucket empty
         rc = _replBucket.waitEmptyAndRollback( NULL, &completeLSN ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to wait for bucket empty, rc: %d", rc ) ;

            // error happenned
            // - rollback trans info
            // - move log back
            // - check if pending on duplicated key issue
            INT32 rcTmp = _pTransCB->rollbackTransInfoFromLog( _pDPSCB,
                                                               completeLSN ) ;
            if ( SDB_OK != rcTmp )
            {
               PD_LOG( PDERROR, "Failed to rollback trans info to "
                       "LSN [%u, %llu], rc: %d", completeLSN.version,
                       completeLSN.offset, rcTmp ) ;
            }

            rcTmp = _pDPSCB->move( completeLSN.offset, completeLSN.version ) ;
            if ( SDB_OK != rcTmp )
            {
               PD_LOG( PDERROR, "Failed to move lsn to [%u, %llu], rc: %d",
                       completeLSN.version, completeLSN.offset, rcTmp ) ;
            }
            else
            {
               PD_LOG( PDEVENT, "Move lsn to[%u, %llu]",
                       completeLSN.version, completeLSN.offset ) ;

               if ( _replBucket.hasPending() )
               {
                  // if pending and some records have been replayed, retry again
                  if ( DPS_INVALID_LSN_OFFSET != firstOffset &&
                       DPS_INVALID_LSN_OFFSET != completeLSN.offset &&
                       completeLSN.offset > firstOffset )
                  {
                     PD_LOG( PDEVENT, "Retry resolve pending lsn "
                             "[%u, %llu]", completeLSN.version,
                             completeLSN.offset ) ;
                     // retry again
                     inRetry = TRUE ;
                     firstOffset = DPS_INVALID_LSN_OFFSET ;
                     replayRC = SDB_OK ;
                     goto retry ;
                  }
               }
            }
            if ( SDB_OK == replayRC )
            {
               replayRC = rc ;
            }
         }
      }

      if ( SDB_OK != replayRC )
      {
         std::cout << "Reply log failed: " << replayRC << std::endl ;
      }
      return rc ;

   error:
      goto done ;
   }

   INT32 _barRSOfflineLogger::_writeSU( barBackupExtentHeader * pExtHeader,
                                        const CHAR * pData )
   {
      INT32 rc = SDB_OK ;
      string suName ;
      string suFileName ;
      UINT32 sequence = 0 ;
      UINT64 offset = 0 ;
      string path ;

      rc = _parseExtentMeta( pExtHeader, suName, suFileName, sequence,
                             offset ) ;
      if ( rc )
      {
         goto error ;
      }

      if ( _openedSU && 0 != _curSUFileName.compare( suFileName ) )
      {
         rc = _closeSUFile() ;
         if ( rc )
         {
            goto error ;
         }
      }

      if ( BAR_DATA_TYPE_RAW_DATA == pExtHeader->_dataType )
      {
         path = _pOptCB->getDbPath() ;
      }
      else if ( BAR_DATA_TYPE_RAW_IDX == pExtHeader->_dataType )
      {
         path = _pOptCB->getIndexPath() ;
      }
      else if ( BAR_DATA_TYPE_RAW_LOBD == pExtHeader->_dataType )
      {
         path = _pOptCB->getLobPath() ;
      }
      else
      {
         /// lob meta
         path = _pOptCB->getLobMetaPath() ;
      }

      rc = _openSUFile( path, suName, suFileName, sequence  ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open su file[%s], rc: %d",
                   suFileName.c_str(), rc ) ;

      if ( offset != _curSUOffset )
      {
         rc = ossSeek( &_curSUFile, offset, OSS_SEEK_SET ) ;
         PD_RC_CHECK( rc, PDERROR, "Seek file[%s] offset[%lld] failed, rc: %d",
                      _curSUFileName.c_str(), offset, rc ) ;
         _curSUOffset = offset ;
      }

      rc = _flush( _curSUFile, pData, pExtHeader->_dataSize ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to write data to file[%s], rc: %d",
                   _curSUFileName.c_str(), rc ) ;
      _curSUOffset += pExtHeader->_dataSize ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _barRSOfflineLogger::_openSUFile( const string &path,
                                           const string & suName,
                                           const string & suFileName,
                                           UINT32 sequence )
   {
      INT32 rc = SDB_OK ;
      string pathName = rtnFullPathName( path, suFileName ) ;

      if ( _openedSU )
      {
         goto done ;
      }
      // open as a writer, the close will call fsync
      rc = ossOpen( pathName.c_str(), OSS_CREATE|OSS_READWRITE,
                    OSS_RU | OSS_WU | OSS_RG, _curSUFile ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open su file[%s], rc: %d",
                   pathName.c_str(), rc ) ;

      _openedSU = TRUE ;
      _curSUName = suName ;
      _curSUFileName = suFileName ;
      _curSUSequence = sequence ;

      PD_LOG( PDEVENT, "Begin to restore su[%s]...", _curSUFileName.c_str() );
      std::cout << "Begin to restore su: " << _curSUFileName.c_str()
                << " ..." << std::endl ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _barRSOfflineLogger::_afterRestore( pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      if ( DPS_INVALID_LSN_OFFSET != _metaHeader._transLSNOffset )
      {
         if ( !_hasLoadDMS )
         {
            PD_LOG( PDEVENT, "Begin to load all collection spaces..." ) ;
            std::cout << "Begin to load all collection spaces..." << std::endl ;

            // load all collectionspaces
            pmdGetKRCB()->setIsRestore( FALSE ) ;
            rc = _pDMSCB->init() ;
            pmdGetKRCB()->setIsRestore( TRUE ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to load collection spaces, "
                         "rc: %d", rc ) ;
            _hasLoadDMS = TRUE ;
         }

         PD_LOG( PDEVENT, "Begin to rollback all trans..." ) ;
         std::cout << "Begin to rollback all trans..." << std::endl ;
         // rollback trans
         rc = rtnTransRollbackAll( cb, 0 ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to rollback all trans, rc: %d",
                      rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _barBackupMgr implement
   */
   _barBackupMgr::_barBackupMgr ( const string &checkGroupName,
                                  const string &checkHostName,
                                  const string &checkSvcName )
   {
      _checkGroupName = checkGroupName ;
      _checkHostName = checkHostName ;
      _checkSvcName = checkSvcName ;
      
   }

   _barBackupMgr::~_barBackupMgr ()
   {
   }

   UINT32 _barBackupMgr::count ()
   {
      return _backupInfo.size() ;
   }

   /*INT32 _barBackupMgr::_enumSpecBackup()
   {
      barBackupInfo info ;
      info._name = backupName () ;
      info._maxIncID = _ensureMetaFileSeq() ;

      if ( info._maxIncID > 0 )
      {
         _backupInfo.push_back( info ) ;
      }
      return SDB_OK ;
   }*/

   INT32 _barBackupMgr::_enumBackups ( const string &fullPath,
                                       const string &subPath )
   {
      INT32 rc = SDB_OK ;
      barBackupInfo info ;
      set< barBackupInfo > tmpSet ;
      UINT32 incID = 0 ;

      fs::path dbDir ( fullPath ) ;
      fs::directory_iterator end_iter ;

      info._relPath = subPath ;

      if ( fs::exists ( dbDir ) && fs::is_directory ( dbDir ) )
      {
         for ( fs::directory_iterator dir_iter ( dbDir );
               dir_iter != end_iter; ++dir_iter )
         {
            if ( fs::is_regular_file ( dir_iter->status() ) )
            {
               const std::string fileName =
                  dir_iter->path().filename().string() ;

               /// clear info
               info._setSeq.clear() ;

               if ( parseMetaFile( fileName, info._name, incID ) &&
                    ( _backupName.empty() ||
                      0 == _backupName.compare( info._name ) ) &&
                    0 == tmpSet.count( info ) &&
                    _ensureMetaFileSeq( fullPath, info._name,
                                        &(info._setSeq) ) > 0 )
               {
                  _backupInfo.push_back( info ) ;
                  tmpSet.insert( info ) ;
               }
            }
            else if ( fs::is_directory( dir_iter->path() ) )
            {
               string tmpSubPath = subPath ;
               if ( !tmpSubPath.empty() )
               {
                  tmpSubPath += OSS_FILE_SEP ;
               }
               tmpSubPath += dir_iter->path().leaf().string() ;
               _enumBackups( dir_iter->path().string(), tmpSubPath ) ;
            }
         }
      }
      else
      {
         PD_LOG( PDERROR, "Path[%s] not exist or invalid", path() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _barBackupMgr::init( const CHAR *path,
                              const CHAR *backupName,
                              const CHAR *prefix )
   {
      INT32 rc = SDB_OK ;
      _backupInfo.clear() ;

      rc = _initInner( path, backupName, prefix ) ;
      if ( rc )
      {
         goto error ;
      }

      if ( !backupName || 0 == ossStrlen( backupName ) )
      {
         _backupName = "" ;
      }
      rc = _enumBackups( _path, "" ) ;

      PD_RC_CHECK( rc, PDERROR, "Enum backup failed, path: %s, name: %s, "
                   "rc: %d", _path.c_str(), _backupName.c_str(), rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _barBackupMgr::_backupToBSON( const barBackupInfo &info,
                                       vector< BSONObj > &vecBackup,
                                       BOOLEAN detail )
   {
      INT32 rc = SDB_OK ;
      UINT32 incID = 0 ;
      UINT64 secretValue = 0 ;
      BOOLEAN hasError = FALSE ;
      BSONObj obj ;

      set< UINT32 >::const_iterator it = info._setSeq.begin() ;
      while ( it != info._setSeq.end() )
      {
         incID = *it ;
         ++it ;

         rc = _readMetaHeader( incID, &_metaHeader, TRUE, secretValue ) ;
         if ( SDB_BAR_DAMAGED_BK_FILE == rc )
         {
            hasError = TRUE ;
            rc = SDB_OK ;
         }
         else
         {
            hasError = FALSE ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to read meta header, rc: %d", rc ) ;

         // if check node info
         if ( !_checkGroupName.empty() &&
              0 != ossStrncmp( _metaHeader._groupName, _checkGroupName.c_str(),
                               BAR_BACKUP_GROUPNAME_LEN - 1 ) )
         {
            goto done ;
         }
         if ( !_checkHostName.empty() &&
              0 != ossStrncmp( _metaHeader._hostName, _checkHostName.c_str(),
                               BAR_BACKUP_HOSTNAME_LEN - 1 ) )
         {
            goto done ;
         }
         if ( !_checkSvcName.empty() &&
              0 != ossStrncmp( _metaHeader._svcName, _checkSvcName.c_str(),
                               BAR_BACKUP_SVCNAME_LEN - 1 ) )
         {
            goto done ;
         }

         if ( 0 == secretValue )
         {
            secretValue = _metaHeader._secretValue ;
         }

         rc = _metaHeaderToBSON( &_metaHeader, incID, info._relPath,
                                 hasError, obj, detail ) ;
         PD_RC_CHECK( rc, PDERROR, "Meta header to bson failed, rc: %d", rc ) ;

         vecBackup.push_back( obj ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _barBackupMgr::_metaHeaderToBSON( barBackupHeader *pHeader,
                                           UINT32 incID,
                                           const string &relPath,
                                           BOOLEAN hasError, BSONObj &obj,
                                           BOOLEAN detail )
   {
      BSONObjBuilder builder ;

      builder.append( FIELD_NAME_VERSION, pHeader->_version ) ;
      builder.append( FIELD_NAME_NAME, pHeader->_name ) ;
      builder.append( FIELD_NAME_ID, (INT32)incID ) ;
      if ( 0 != pHeader->_description[0] )
      {
         builder.append( FIELD_NAME_DESP, pHeader->_description ) ;
      }
      if ( !relPath.empty() )
      {
         builder.append( FIELD_NAME_PATH, relPath.c_str() ) ;
      }

      if ( SDB_ROLE_STANDALONE != pmdGetKRCB()->getDBRole() )
      {
         monAppendSystemInfo( builder, MON_MASK_NODE_NAME |
                                       MON_MASK_GROUP_NAME ) ;
      }
      else
      {
         if ( 0 != pHeader->_groupName[0] )
         {
            builder.append( FIELD_NAME_GROUPNAME, pHeader->_groupName ) ;
         }
         string nodeName = pHeader->_hostName ;
         if ( !nodeName.empty() )
         {
            nodeName += ":" ;
            nodeName += pHeader->_svcName ;
            builder.append( FIELD_NAME_NODE_NAME, nodeName ) ;
         }
      }

      builder.append( FIELD_NAME_ENSURE_INC,
                      pHeader->_opType == BAR_BACKUP_OP_TYPE_INC ?
                      true : false ) ;

      // stat info
      builder.append( "BeginLSNOffset", (INT64)pHeader->_beginLSNOffset ) ;
      builder.append( "EndLSNOffset", (INT64)pHeader->_endLSNOffset ) ;
      builder.append( "TransLSNOffset", (INT64)pHeader->_transLSNOffset ) ;

      builder.append( "StartTime", pHeader->_startTimeStr ) ;
      if ( detail )
      {
         builder.append( "EndTime", pHeader->_endTimeStr ) ;
         builder.append( "UseTime", (INT32)pHeader->_useTime ) ;
         builder.append( "CSNum", (INT32)pHeader->_csNum ) ;
         builder.append( "DataFileNum", (INT32)pHeader->_dataFileNum ) ;
         builder.append( "BeginDataFileSeq",
                         (INT32)pHeader->_lastDataSequence -
                         (INT32)pHeader->_dataFileNum + 1 ) ;
         builder.append( "LastDataFileSeq", (INT32)pHeader->_lastDataSequence ) ;
         builder.append( "LastExtentID", (INT64)pHeader->_lastExtentID ) ;
         builder.append( "DataSize", (INT64)pHeader->_dataSize ) ;
         builder.append( "ThinDataSize", (INT64)pHeader->_thinDataSize ) ;
         builder.append( "CompressedDataSize",
                         (INT64)pHeader->_compressDataSize ) ;

         UINT64 totalSize = pHeader->_dataSize + pHeader->_thinDataSize +
                            pHeader->_compressDataSize ;
         INT32 ratio = (INT32)((FLOAT64)(pHeader->_dataSize*100)/totalSize) ;
         builder.append( "CompressedRatio", ratio ) ;

         builder.append( FIELD_NAME_COMPRESSIONTYPE,
                         utilCompressType2String(
                         (UINT8)(pHeader->_compressionType) ) ) ;
      }

      builder.append( "LastLSN", (INT64)pHeader->_lastLSN ) ;
      builder.append( "LastLSNCode", pHeader->_lastLSNCode ) ;
      builder.append( "HasError", hasError ? true : false ) ;
      obj = builder.obj () ;

      return SDB_OK ;
   }

   INT32 _barBackupMgr::list( vector < BSONObj > &vecBackup,
                              BOOLEAN detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj backupObj ;
      string tmpPath = _path ;
      string tmpName = _backupName ;

      vecBackup.clear() ;

      vector< barBackupInfo >::iterator it = _backupInfo.begin() ;
      while ( it != _backupInfo.end() )
      {
         barBackupInfo &info = ( *it ) ;

         if ( info._relPath.empty() )
         {
            _path = tmpPath ;
         }
         else
         {
            _path = rtnFullPathName( tmpPath, info._relPath ) ;
         }
         _backupName = info._name ;

         rc = _backupToBSON( info, vecBackup, detail ) ;
         PD_RC_CHECK( rc, PDERROR, "Backup[%s] to bson failed, rc: %d",
                      (*it)._name.c_str(), rc ) ;
         ++it ;
      }

   done:
      _path = tmpPath ;
      _backupName = tmpName ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _barBackupMgr::_dropAllInc( const string &backupName )
   {
      INT32 rc = SDB_OK ;
      multimap< string, string > mapFiles ;
      multimap< string, string >::iterator it ;
      string filterMeta = backupName + BAR_BACKUP_META_FILE_EXT ;
      string filterData = backupName + "." ;

      string tmpName ;
      UINT32 seq = 0 ;

      /// enum data and remove
      rc = ossEnumFiles2( _path, mapFiles, filterData.c_str(),
                          OSS_MATCH_LEFT, 1 ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Enum directory[%s]'s data failed, rc: %d",
                 _path.c_str(), rc ) ;
         goto error ;
      }

      it = mapFiles.begin() ;
      while( it != mapFiles.end() )
      {
         if ( !parseDateFile( it->first, tmpName, seq ) ||
              tmpName != backupName )
         {
            ++it ;
            continue ;
         }
         if ( SDB_OK == ossAccess( it->second.c_str() ) )
         {
            rc = ossDelete( it->second.c_str() ) ;
            PD_RC_CHECK( rc, PDWARNING, "Failed to delete file[%s], rc: %d",
                         it->second.c_str(), rc ) ;
         }
         ++it ;
      }
      mapFiles.clear() ;

      /// enum meta and remove
      rc = ossEnumFiles2(_path, mapFiles, filterMeta.c_str(),
                          OSS_MATCH_LEFT, 1 ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Enum directory[%s]'s meta file failed, rc: %d",
                 _path.c_str(), rc ) ;
         goto error ;
      }

      it = mapFiles.begin() ;
      while( it != mapFiles.end() )
      {
         if ( !parseMetaFile( it->first, tmpName, seq ) ||
              tmpName != backupName )
         {
            ++it ;
            continue ;
         }
         if ( SDB_OK == ossAccess( it->second.c_str() ) )
         {
            rc = ossDelete( it->second.c_str() ) ;
            PD_RC_CHECK( rc, PDWARNING, "Failed to delete file[%s], rc: %d",
                         it->second.c_str(), rc ) ;
         }
         ++it ;
      }
      mapFiles.clear() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _barBackupMgr::_dropByInc ( const string &backupName, UINT32 incID )
   {
      INT32 rc = SDB_OK ;
      string fileName ;

      rc = _readMetaHeader( incID, &_metaHeader, TRUE, 0 ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Read meta header failed in "
                 "backup[Name:%s, IncID:%d], rc: %d",
                 backupName.c_str(), incID, rc ) ;
         goto error ;
      }

      // drop data file
      while ( _metaHeader._dataFileNum > 0 )
      {
         fileName = getDataFileName( backupName,
                                     _metaHeader._lastDataSequence ) ;
         if ( SDB_OK == ossAccess( fileName.c_str() ) )
         {
            rc = ossDelete( fileName.c_str() ) ;
            PD_RC_CHECK( rc, PDWARNING, "Failed to delete file[%s], rc: %d",
                         fileName.c_str(), rc ) ;
         }
         --_metaHeader._dataFileNum ;
         --_metaHeader._lastDataSequence ;
      }

      // drop meta file
      fileName = getIncFileName( backupName, incID ) ;
      if ( SDB_OK == ossAccess( fileName.c_str() ) )
      {
         rc = ossDelete( fileName.c_str() ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to delete file[%s], rc: %d",
                      fileName.c_str(), rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _barBackupMgr::drop ( INT32 incID )
   {
      INT32 rc = SDB_OK ;
      string tmpPath = _path ;
      string tmpName = _backupName ;

      vector< barBackupInfo >::iterator it = _backupInfo.begin() ;
      while ( it != _backupInfo.end() )
      {
         barBackupInfo &info = ( *it ) ;

         if ( info._relPath.empty() )
         {
            _path = tmpPath ;
         }
         else
         {
            _path = rtnFullPathName( tmpPath, info._relPath ) ;
         }
         _backupName = info._name ;

         if ( incID < 0 )
         {
            rc = _dropAllInc( info._name ) ;
         }
         else
         {
            if ( 0 == info._setSeq.count( (UINT32)incID ) )
            {
               rc = SDB_BAR_BACKUP_NOTEXIST ;
            }
            else
            {
               rc = _dropByInc( info._name, (UINT32)incID ) ;
            }
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to drop backup[%s], rc: %d",
                      (*it)._name.c_str(), rc ) ;
         ++it ;
      }

   done:
      _path = tmpPath ;
      _backupName = tmpName ;
      return rc ;
   error:
      goto done ;
   }

}

