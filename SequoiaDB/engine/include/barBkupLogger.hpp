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

   Source File Name = barBkupLogger.hpp

   Descriptive Name = backup and recovery

   When/how to use: this program may be used on backup or restore db data.
   You can specfiy some options from parameters.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/22/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef BARBKUPLOGGER_HPP_
#define BARBKUPLOGGER_HPP_

#include "ossIO.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "core.hpp"
#include "pmdOptionsMgr.hpp"
#include "dpsDef.hpp"
#include "msgDef.h"
#include "dms.hpp"
#include "utilStr.hpp"
#include "clsReplBucket.hpp"

using namespace bson ;

namespace engine
{

   class _SDB_DMSCB ;
   class _dpsLogWrapper ;
   class dpsTransCB ;
   class _pmdOptionsMgr ;
   class _dmsStorageBase ;
   class _dmsStorageLob ;
   class _pmdEDUCB ;
   class _clsMgr ;

   #define BAR_BACKUP_META_FILE_EXT                   ".bak"

#pragma pack(4)
   /*
      Header: eye catcher define
   */
   #define BAR_BACKUP_HEADER_EYECATCHER_LEN           (8)
   #define BAR_BACKUP_META_EYECATCHER                 "BARMETA"
   #define BAR_BACKUP_DATA_EYECATCHER                 "BARDATA"

   /*
      Header: backup type define
   */
   #define BAR_BACKUP_TYPE_OFFLINE                    1
   #define BAR_BACKUP_TYPE_ONLINE                     2

   /*
      Header: backup operation type
   */
   #define BAR_BACKUP_OP_TYPE_FULL                    1
   #define BAR_BACKUP_OP_TYPE_INC                     2

   /*
      Header:: backup version define
   */
   #define BAR_BACKUP_CUR_VERSION                     2

   /*
      Backup length define
   */
   #define BAR_BACKUP_TIME_LEN                        (256)
   #define BAR_BACKUP_NAME_LEN                        (256)
   #define BAR_BACKUP_DESP_LEN                        (1024)
   #define BAR_BACKUP_PATH_LEN                        (1024)
   #define BAR_BACKUP_MD5_LEN                         (16)
   #define BAR_BACKUP_GROUPNAME_LEN                   (128)
   #define BAR_BACKUP_HOSTNAME_LEN                    (256)
   #define BAR_BACKUP_SVCNAME_LEN                     (256)

   /*
      Backup max data file size define
   */
#if defined OSS_ARCH_64
   #define BAR_DFT_DATAFILE_SIZE                      (102400)    // MB
   #define BAR_MAX_DATAFILE_SIZE                      (8388608)   // MB
#elif defined OSS_ARCH_32
   #define BAR_DFT_DATAFILE_SIZE                      (2048)      // MB
   #define BAR_MAX_DATAFILE_SIZE                      (2048)      // MB
#endif
   #define BAR_MIN_DATAFILE_SIZE                      (32)        // MB

   /*
      _barBackupHeader define
   */
   struct _barBackupHeader : public SDBObject
   {
      CHAR              _eyeCatcher[BAR_BACKUP_HEADER_EYECATCHER_LEN] ;
      UINT32            _version ;
      UINT32            _type ;
      UINT32            _opType ;
      DPS_LSN_OFFSET    _beginLSNOffset ;
      DPS_LSN_OFFSET    _endLSNOffset ;
      DPS_LSN_OFFSET    _transLSNOffset ;
      UINT64            _startTime ;
      UINT64            _endTime ;
      UINT32            _useTime ;
      UINT32            _csNum ;
      UINT32            _dataFileNum ;
      UINT32            _lastDataSequence ;
      UINT64            _lastExtentID ;
      UINT64            _maxDataFileSize ;
      UINT64            _dataSize ;
      CHAR              _startTimeStr[BAR_BACKUP_TIME_LEN] ;
      CHAR              _endTimeStr[BAR_BACKUP_TIME_LEN] ;
      CHAR              _name[BAR_BACKUP_NAME_LEN] ;
      CHAR              _description[BAR_BACKUP_DESP_LEN] ;
      CHAR              _path[BAR_BACKUP_PATH_LEN] ;
      UINT64            _secretValue ;
      CHAR              _groupName[BAR_BACKUP_GROUPNAME_LEN] ;
      CHAR              _hostName[BAR_BACKUP_HOSTNAME_LEN] ;
      CHAR              _svcName[BAR_BACKUP_SVCNAME_LEN] ;
      UINT64            _nodeID ;
      UINT64            _lastLSN ;
      UINT32            _lastLSNCode ;
      UINT64            _thinDataSize ;
      UINT64            _compressDataSize ;
      INT32             _compressionType ;
      CHAR              _pad[61932] ;

      _barBackupHeader ()
      {
         SDB_ASSERT( DMS_PAGE_SIZE64K == sizeof(_barBackupHeader),
                     "_barBackupHeader size must be 64K" ) ;
         ossMemset( this, 0, sizeof( _barBackupHeader ) ) ;

         ossStrncpy( _eyeCatcher, BAR_BACKUP_META_EYECATCHER,
                     BAR_BACKUP_HEADER_EYECATCHER_LEN ) ;
         _version          = BAR_BACKUP_CUR_VERSION ;
         _type             = BAR_BACKUP_TYPE_OFFLINE ;
         _opType           = BAR_BACKUP_OP_TYPE_FULL ;
         _beginLSNOffset   = DPS_INVALID_LSN_OFFSET ;
         _endLSNOffset     = DPS_INVALID_LSN_OFFSET ;
         _transLSNOffset   = DPS_INVALID_LSN_OFFSET ;
         _maxDataFileSize  = (UINT64)BAR_DFT_DATAFILE_SIZE << 20 ;
         _dataFileNum      = 0 ;
         _lastDataSequence = 0 ;
         _lastExtentID     = 0 ;
         makeBeginTime() ;
         _secretValue = ossPack32To64( (UINT32)_startTime,
                                       (UINT32)(ossRand()*239641) ) ;
         _lastLSN          = ~0 ;
         _lastLSNCode      = 0 ;
         _thinDataSize     = 0 ;
         _compressDataSize = 0 ;
         _compressionType  = UTIL_COMPRESSOR_INVALID ;
      }
      void createTime( UINT64 &curTime, CHAR *pBuff, UINT32 size )
      {
         time_t tmpTime = time( NULL ) ;
         curTime = (UINT64)tmpTime ;
         utilAscTime( tmpTime, pBuff, size ) ;
      }
      void makeBeginTime ()
      {
         createTime( _startTime, _startTimeStr, BAR_BACKUP_TIME_LEN ) ;
      }
      void makeEndTime ()
      {
         createTime( _endTime, _endTimeStr, BAR_BACKUP_TIME_LEN ) ;
         _useTime = (UINT32)(_endTime - _startTime) ;
      }
      void setName ( const CHAR *name, const CHAR *prefix = NULL )
      {
         _name[0] = 0 ;
         if ( prefix )
         {
            ossSnprintf( _name, BAR_BACKUP_NAME_LEN - 1, "%s_%s",
                         prefix, name ) ;
         }
         else
         {
            ossStrncpy( _name, name, BAR_BACKUP_NAME_LEN - 1 ) ;
         }
      }
      void setDesp ( const CHAR *desp )
      {
         ossStrncpy( _description, desp, BAR_BACKUP_DESP_LEN - 1 ) ;
      }
      void setPath ( const CHAR *path )
      {
         ossStrncpy( _path, path, BAR_BACKUP_PATH_LEN - 1 ) ;
      }
   } ;
   typedef _barBackupHeader barBackupHeader ;
   #define BAR_BACKUP_HEADER_SIZE   sizeof(barBackupHeader)

   /*
      _barBackupDataHeader define
   */
   struct _barBackupDataHeader : public SDBObject
   {
      CHAR              _eyeCatcher[BAR_BACKUP_HEADER_EYECATCHER_LEN] ;
      UINT32            _version ;
      UINT32            _sequence ;
      UINT64            _time ;
      UINT64            _secretValue ;
      INT32             _compressionType ;
      CHAR              _pad[4060] ;

      _barBackupDataHeader ()
      {
         SDB_ASSERT( DMS_PAGE_SIZE4K == sizeof(_barBackupDataHeader),
                     "_barBackupDataHeader size must be 4K" ) ;
         ossMemset( this, 0, sizeof(_barBackupDataHeader) ) ;

         ossStrncpy( _eyeCatcher, BAR_BACKUP_DATA_EYECATCHER,
                     BAR_BACKUP_HEADER_EYECATCHER_LEN ) ;
         _version       = BAR_BACKUP_CUR_VERSION ;
         _compressionType = UTIL_COMPRESSOR_INVALID ;
         _time          = time( NULL ) ;
      }
   } ;
   typedef _barBackupDataHeader barBackupDataHeader ;
   #define BAR_BACKUPDATA_HEADER_SIZE  sizeof(barBackupDataHeader)

   /*
      Extent eyecatcher define
   */
   #define BAR_EXTENT_EYECATCH                     "BARDE"

   /*
      Data type define
   */
   #define BAR_DATA_TYPE_CONFIG                    1     // config
   #define BAR_DATA_TYPE_RAW_DATA                  2     // raw data file data
   #define BAR_DATA_TYPE_RAW_IDX                   3     // raw index file data
   #define BAR_DATA_TYPE_REPL_LOG                  4     // repl log
   #define BAR_DATA_TYPE_RAW_LOBM                  5     // lob meta
   #define BAR_DATA_TYPE_RAW_LOBD                  6     // lob data

   /*
      _barBackupExtentHeader
   */
   struct _barBackupExtentHeader : public SDBObject
   {
      CHAR              _eyeCatcher[BAR_BACKUP_HEADER_EYECATCHER_LEN] ;
      UINT32            _version ;
      UINT64            _extentID ;
      UINT32            _dataType ;
      UINT32            _metaSize ;
      UINT64            _dataSize ;
      CHAR              _md5Value[BAR_BACKUP_MD5_LEN] ;
      UINT8             _thinCopy ;
      UINT8             _compressed ;
      CHAR              _reserved[10] ;
      // up for header(64B), down for meta bson obj data(max 960B)
      CHAR              _metaData[960] ;

      void init ()
      {
         ossMemset( this, 0, sizeof(_barBackupExtentHeader) ) ;

         ossStrncpy( _eyeCatcher, BAR_EXTENT_EYECATCH,
                     BAR_BACKUP_HEADER_EYECATCHER_LEN ) ;
         _version          = BAR_BACKUP_CUR_VERSION ;
         _extentID         = 0 ;
         _dataType         = BAR_DATA_TYPE_RAW_DATA ;
         _dataSize         = 0 ;
         _thinCopy         = 0 ;
         _compressed       = 0 ;
      }
      _barBackupExtentHeader ()
      {
         /*SDB_ASSERT( 64 == offsetof(_barBackupExtentHeader, _metaData),
                     "_pad offset must be 64" ) ;*/
         SDB_ASSERT( 1024 == sizeof(_barBackupExtentHeader),
                     "_barBackupExtentHeader size must be 1K" ) ;
         init () ;
      }
      const CHAR* getMetaData()
      {
         return ( 0 == _metaSize ) ? NULL : (const CHAR*)_metaData ;
      }
      INT32 setMetaData( const CHAR *data, UINT32 size )
      {
         if ( size > sizeof(_metaData) )
         {
            return SDB_SYS ;
         }
         _metaSize = size ;
         if ( data )
         {
            ossMemcpy( _metaData, data, size ) ;
         }
         return SDB_OK ;
      }
      UINT32 metaDataSize() const
      {
         return _metaSize ;
      }
      UINT64 dataSize() const
      {
         return _dataSize ;
      }
   } ;
   typedef _barBackupExtentHeader barBackupExtentHeader ;
   #define BAR_BACKUP_EXTENT_HEADER_SIZE sizeof(barBackupExtentHeader)

#pragma pack()

   class _utilCompressor ;
   /*
      _barBaseLogger define
   */
   class _barBaseLogger : public SDBObject
   {
      public:
         _barBaseLogger () ;
         virtual ~_barBaseLogger () ;

         const CHAR *backupName () const { return _backupName.c_str() ; }
         const CHAR *path () const { return _path.c_str() ; }

      public:
         string   getMainFileName() ;
         string   getIncFileName( UINT32 sequence ) ;
         string   getIncFileName ( const string &backupName,
                                   UINT32 sequence ) ;
         string   getIncFileName ( const string &path,
                                   const string &backupName,
                                   UINT32 sequence ) ;
         string   getDataFileName ( UINT32 sequence ) ;
         string   getDataFileName ( const string &backupName,
                                    UINT32 sequence ) ;

         BOOLEAN  parseMainFile( const string &fileName,
                                 string &backupName ) ;

         BOOLEAN  parseIncFile( const string &fileName,
                                string &backupName,
                                UINT32 &incID ) ;

         BOOLEAN  parseMetaFile( const string &fileName,
                                 string &backupName,
                                 UINT32 &incID ) ;

         BOOLEAN  parseDateFile( const string &fileName,
                                 string &backupName,
                                 UINT32 &seq ) ;

      protected:
         UINT32      _ensureMetaFileSeq ( set< UINT32 > *pSetSeq = NULL ) ;
         UINT32      _ensureMetaFileSeq ( const string &backupName,
                                          set< UINT32 > *pSetSeq = NULL ) ;
         UINT32      _ensureMetaFileSeq ( const string &path,
                                          const string &backupName,
                                          set< UINT32 > *pSetSeq = NULL ) ;
         INT32       _initInner ( const CHAR *path, const CHAR *backupName,
                                  const CHAR *prefix ) ;

         INT32       _read( OSSFILE &file, CHAR *buf, SINT64 len ) ;
         INT32       _flush( OSSFILE &file, const CHAR *buf, SINT64 len ) ;

         INT32       _readMetaHeader( UINT32 incID,
                                      barBackupHeader *pHeader,
                                      BOOLEAN check = TRUE,
                                      UINT64 secretValue = 0 ) ;

         INT32       _readDataHeader( OSSFILE &file,
                                      const string &fileName,
                                      barBackupDataHeader *pHeader,
                                      BOOLEAN check = TRUE,
                                      UINT64 secretValue = 0,
                                      UINT32 sequence = 0 ) ;

         INT32       _readDataHeader( UINT32 sequence,
                                      barBackupDataHeader *pHeader,
                                      BOOLEAN check = TRUE,
                                      UINT64 secretValue = 0 ) ;

         string      _replaceWildcard( const CHAR *source ) ;

         BOOLEAN     _isDigital( const CHAR *pStr, UINT32 *pNum = NULL ) ;

         CHAR*       _allocCompressBuff( UINT64 buffSize ) ;

      protected:
         barBackupHeader               _metaHeader ;
         string                        _path ;
         string                        _backupName ;

         barBackupExtentHeader         *_pDataExtent ;

         _SDB_DMSCB                    *_pDMSCB ;
         _dpsLogWrapper                *_pDPSCB ;
         dpsTransCB                    *_pTransCB ;
         _pmdOptionsMgr                *_pOptCB ;
         _clsMgr                       *_pClsCB ;
         /// compressor
         _utilCompressor               *_pCompressor ;
         CHAR                          *_pCompressBuff ;
         UINT64                        _buffSize ;

   } ;
   typedef _barBaseLogger barBaseLogger ;

   /*
      _barBkupBaseLogger define
   */
   class _barBkupBaseLogger : public _barBaseLogger
   {
      public:
         _barBkupBaseLogger () ;
         virtual ~_barBkupBaseLogger () ;

         UINT32   getMetaFileSeq () const { return _metaFileSeq ; }
         INT32    dropCurBackup() ;
         INT32    dropAllBackup () ;

         INT32    init( const CHAR *path,
                        const CHAR *backupName,
                        INT32 maxDataFileSize,
                        const CHAR *prefix = NULL,
                        UINT32 opType = BAR_BACKUP_OP_TYPE_FULL,
                        BOOLEAN rewrite = TRUE,
                        const CHAR *backupDesp = NULL ) ;

         void     setBackupLog( BOOLEAN backupLog ) ;
         void     enableCompress( BOOLEAN compressed,
                                  UTIL_COMPRESSOR_TYPE compType ) ;

         INT32    backup ( _pmdEDUCB *cb ) ;

      protected:
         virtual UINT32    _getBackupType () const = 0 ;

         virtual INT32     _prepareBackup ( _pmdEDUCB *cb, BOOLEAN &isEmpty ) = 0 ;
         virtual INT32     _doBackup ( _pmdEDUCB *cb ) = 0 ;
         virtual INT32     _afterBackup ( _pmdEDUCB *cb ) = 0 ;
         virtual INT32     _onWriteMetaFile () = 0 ;

      protected:
         INT32       _initCheckAndPrepare () ;
         INT32       _updateFromMetaFile( UINT32 incID ) ;

         INT32       _writeData( const CHAR *buf, INT64 len,
                                 BOOLEAN isExtHeader ) ;

         INT32       _writeExtent( barBackupExtentHeader *pHeader,
                                   const CHAR *pData ) ;

         barBackupExtentHeader *_nextDataExtent ( UINT32 dataType ) ;

      private:
         INT32       _openDataFile () ;
         INT32       _closeCurFile () ;

         INT32       _backupConfig () ;
         INT32       _writeMetaFile () ;

      protected:
         BOOLEAN                       _rewrite ;
         UINT32                        _metaFileSeq ;
         UINT64                        _lastLSN ;
         UINT32                        _lastLSNCode ;

         UINT64                        _curFileSize ;
         OSSFILE                       _curFile ;
         BOOLEAN                       _isOpened ;

         BOOLEAN                       _needBackupLog ;
         BOOLEAN                       _compressed ;

   } ;
   typedef _barBkupBaseLogger barBkupBaseLogger ;

   /*
      _barBKOfflineLogger define
   */
   class _barBKOfflineLogger : public _barBkupBaseLogger
   {
      public:
         _barBKOfflineLogger () ;
         virtual ~_barBKOfflineLogger () ;

      protected:
         virtual UINT32    _getBackupType () const ;

         virtual INT32     _prepareBackup ( _pmdEDUCB *cb, BOOLEAN &isEmpty ) ;
         virtual INT32     _doBackup ( _pmdEDUCB *cb ) ;
         virtual INT32     _afterBackup ( _pmdEDUCB *cb ) ;
         virtual INT32     _onWriteMetaFile () ;

      protected:
         INT32             _backupSU( _dmsStorageBase *pSU,
                                      _pmdEDUCB *cb ) ;
         INT32             _backupLog( _pmdEDUCB *cb ) ;
         INT32             _backupLobData( _dmsStorageLob *pLobSU,
                                           _pmdEDUCB *cb ) ;
         BSONObj           _makeExtentMeta( _dmsStorageBase *pSU ) ;
         BSONObj           _makeExtentMeta( _dmsStorageLob *pLobSU ) ;

      private:
         INT32             _nextThinCopyInfo ( _dmsStorageBase *pSU,
                                               UINT32 startExtID,
                                               UINT32 maxExtID,
                                               UINT32 maxNum,
                                               UINT32 &num,
                                               BOOLEAN &used ) ;

      private:
         UINT32            _curDataType ;
         UINT64            _curOffset ;
         UINT32            _curSequence ;
         INT32             _replStatus ;
         BOOLEAN           _hasRegBackup ;

         CHAR              *_pExtentBuff ;
   } ;
   typedef _barBKOfflineLogger barBKOfflineLogger ;

   /*
      _barBackupHeaderSimple define
   */
   struct _barBackupHeaderSimple
   {
      DPS_LSN_OFFSET    _beginLSNOffset ;
      DPS_LSN_OFFSET    _endLSNOffset ;
      DPS_LSN_OFFSET    _transLSNOffset ;
      UINT32            _dataFileNum ;
      UINT32            _lastDataSequence ;
      UINT64            _lastExtentID ;

      _barBackupHeaderSimple( const _barBackupHeader *pHeader )
      {
         _beginLSNOffset = pHeader->_beginLSNOffset ;
         _endLSNOffset = pHeader->_endLSNOffset ;
         _transLSNOffset = pHeader->_transLSNOffset ;
         _dataFileNum = pHeader->_dataFileNum ;
         _lastDataSequence = pHeader->_lastDataSequence ;
         _lastExtentID = pHeader->_lastExtentID ;
      }
      _barBackupHeaderSimple()
      {
         _beginLSNOffset   = DPS_INVALID_LSN_OFFSET ;
         _endLSNOffset     = DPS_INVALID_LSN_OFFSET ;
         _transLSNOffset   = DPS_INVALID_LSN_OFFSET ;
         _dataFileNum      = 0 ;
         _lastDataSequence = 0 ;
         _lastExtentID     = 0 ;
      }
   } ;
   typedef _barBackupHeaderSimple barBackupHeaderSimple ;


   typedef std::map< UINT32, barBackupHeaderSimple >  MAP_BACKUP_INFO ;
   typedef MAP_BACKUP_INFO::iterator                  MAP_BACKUP_INFO_IT ;
   /*
      _barRSBaseLogger define
   */
   class _barRSBaseLogger : public _barBaseLogger
   {
      public:
         _barRSBaseLogger () ;
         virtual ~_barRSBaseLogger () ;

         INT32    cleanDMSData () ;

         BSONObj  getConf () const { return _confObj ; }

         INT32    init( const CHAR *path, const CHAR *backupName,
                        const CHAR *prefix = NULL, INT32 incID = -1,
                        INT32 beginID = -1, BOOLEAN skipConf = FALSE ) ;

         INT32    restore ( _pmdEDUCB *cb ) ;

      protected:
         virtual INT32     _prepareRestore ( _pmdEDUCB *cb,
                                             BOOLEAN &isEmpty ) = 0 ;
         virtual INT32     _doRestore ( _pmdEDUCB *cb ) = 0 ;
         virtual INT32     _afterRestore ( _pmdEDUCB *cb ) = 0 ;

      protected:
         INT32       _initCheckAndPrepare ( INT32 incID,
                                            INT32 beginID,
                                            set< UINT32 > &setSeq ) ;
         INT32       _updateFromMetafile( UINT32 incID ) ;
         INT32       _loadConf () ;

         INT32       _restoreConfig () ;

         INT32       _readData( barBackupExtentHeader **pExtHeader,
                                CHAR **pBuff ) ;

      private:
         INT32       _openDataFile () ;
         INT32       _closeCurFile () ;
         INT32       _allocBuff( UINT64 buffSize ) ;
         void        _reset () ;

      protected:
         UINT32                        _metaFileSeq ;
         UINT64                        _secretValue ;
         UINT64                        _expectExtID ;
         UINT32                        _curDataFileSeq ;

         BSONObj                       _confObj ;
         OSSFILE                       _curFile ;
         BOOLEAN                       _isOpened ;
         UINT64                        _curOffset ;

         CHAR                          *_pBuff ;
         UINT64                        _buffSize ;

         MAP_BACKUP_INFO               _mapBackupInfo ;
         INT32                         _beginID ;

         BOOLEAN                       _skipConf ;
         BOOLEAN                       _isDoRestoring ;

   } ;
   typedef _barRSBaseLogger barRSBaseLogger ;

   /*
      _barRSOfflineLogger define
   */
   class _barRSOfflineLogger : public _barRSBaseLogger
   {
      public:
         _barRSOfflineLogger () ;
         virtual ~_barRSOfflineLogger () ;

      protected:
         virtual INT32     _prepareRestore ( _pmdEDUCB *cb,
                                             BOOLEAN &isEmpty ) ;
         virtual INT32     _doRestore ( _pmdEDUCB *cb ) ;
         virtual INT32     _afterRestore ( _pmdEDUCB *cb ) ;

         INT32    _processConfigData( barBackupExtentHeader *pExtHeader,
                                      const CHAR *pData ) ;
         INT32    _processRawData( barBackupExtentHeader *pExtHeader,
                                   const CHAR *pData ) ;
         INT32    _processRawIndex( barBackupExtentHeader *pExtHeader,
                                    const CHAR *pData ) ;
         INT32    _processRawLobM( barBackupExtentHeader *pExtHeader,
                                   const CHAR *pData ) ;
         INT32    _processRawLobD( barBackupExtentHeader *pExtHeader,
                                   const CHAR *pData ) ;
         INT32    _processReplLog( barBackupExtentHeader *pExtHeader,
                                   const CHAR *pData, BOOLEAN isIncData,
                                   _pmdEDUCB *cb ) ;

      private:
         INT32             _closeSUFile () ;
         INT32             _openSUFile ( const string &path,
                                         const string &suName,
                                         const string &suFileName,
                                         UINT32 sequence ) ;
         INT32             _parseExtentMeta( barBackupExtentHeader *pExtHeader,
                                             string &suName, string &suFileName,
                                             UINT32 &sequence,
                                             UINT64 &offset ) ;
         INT32             _writeSU( barBackupExtentHeader *pExtHeader,
                                     const CHAR *pData ) ;

      private:
         string               _curSUName ;
         UINT32               _curSUSequence ;
         UINT64               _curSUOffset ;
         string               _curSUFileName ;
         UINT32               _incDataFileBeginSeq ;
         BOOLEAN              _hasLoadDMS ;

         OSSFILE              _curSUFile ;
         BOOLEAN              _openedSU ;

         /// reply dps
         clsBucket            _replBucket ;
   } ;
   typedef _barRSOfflineLogger barRSOfflineLogger ;

   /*
      _barBackupInfo define
   */
   struct _barBackupInfo : public SDBObject
   {
      string         _relPath ;
      string         _name ;
      set<UINT32>    _setSeq ;

      bool operator<( const _barBackupInfo &rhs ) const
      {
         BOOLEAN lEmpty = _relPath.empty() ? TRUE : FALSE ;
         BOOLEAN rEmpty = rhs._relPath.empty() ? TRUE : FALSE ;
         int result = 0 ;

         if ( lEmpty && !rEmpty )
         {
            return true ;
         }
         else if ( !lEmpty && rEmpty )
         {
            return false ;
         }
         else if ( !lEmpty && !rEmpty )
         {
            result = ossStrcmp( _relPath.c_str(), rhs._relPath.c_str() ) ;
            if ( 0 != result )
            {
               return result < 0 ? true : false ;
            }
         }

         result = ossStrcmp( _name.c_str(), rhs._name.c_str() ) ;
         return result < 0 ? true : false ;
      }

      bool operator==( const _barBackupInfo &rhs ) const
      {
         if ( _relPath.length() == rhs._relPath.length() &&
              _relPath == rhs._relPath &&
              _name == rhs._name )
         {
            return true ;
         }
         return false ;
      }
   } ;
   typedef _barBackupInfo barBackupInfo ;

   /*
      _barBackupMgr define
   */
   class _barBackupMgr : public _barBaseLogger
   {
      public:
         _barBackupMgr ( const string &checkGroupName = "",
                         const string &checkHostName = "",
                         const string &checkSvcName = "" ) ;
         virtual ~_barBackupMgr () ;

         INT32          init( const CHAR *path,
                              const CHAR *backupName = NULL,
                              const CHAR *prefix = NULL ) ;
         INT32          list( vector< BSONObj > &vecBackup,
                              BOOLEAN detail = FALSE ) ;

         INT32          drop ( INT32 incID = -1 ) ;

         UINT32         count () ;

      protected:
         INT32          _enumBackups ( const string &fullPath,
                                       const string &subPath ) ;
         //INT32          _enumSpecBackup () ;

         INT32          _backupToBSON ( const barBackupInfo &info,
                                        vector< BSONObj > &vecBackup,
                                        BOOLEAN detail ) ;

         INT32          _metaHeaderToBSON( barBackupHeader *pHeader,
                                           UINT32 incID,
                                           const string &relPath,
                                           BOOLEAN hasError,
                                           BSONObj &obj,
                                           BOOLEAN detail ) ;

         INT32          _dropByInc( const string &backupName, UINT32 incID ) ;
         INT32          _dropAllInc( const string &backupName ) ;

      private:
         vector< barBackupInfo >             _backupInfo ;
         string                              _checkGroupName ;
         string                              _checkHostName ;
         string                              _checkSvcName ;

   } ;
   typedef _barBackupMgr barBackupMgr ;

}

#endif //BARBKUPLOGGER_HPP_

