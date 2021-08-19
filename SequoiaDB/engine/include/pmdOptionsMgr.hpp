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

   Source File Name = pmdOptionsMgr.hpp

   Descriptive Name = Process MoDel Main

   When/how to use: this program may be used for managing sequoiadb
   configuration.It can be initialized from cmd and configure file.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/22/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef PMDOPTIONSMGR_HPP_
#define PMDOPTIONSMGR_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossIO.hpp"
#include "pmdOptions.hpp"
#include "msgDef.hpp"
#include "pmdDef.hpp"
#include "ossSocket.hpp"
#include "utilParam.hpp"
#include "dpsDef.hpp"
#include "sdbInterface.hpp"

#include <string>
#include <iostream>
#include <vector>
#include <map>
#include "../bson/bson.h"

using namespace std ;
using namespace bson ;

namespace engine
{
   #define PMD_MAX_ENUM_STR_LEN        ( 32 )
   #define PMD_MAX_LONG_STR_LEN        ( 256 )
   #define PMD_MAX_SHORT_STR_LEN       ( 32 )
   #define PMD_MAX_LOGMOD_STR_LEN      ( 32 )

   enum PMD_CFG_STEP
   {
      PMD_CFG_STEP_INIT       = 0,           // initialize
      PMD_CFG_STEP_REINIT,                   // re-init
      PMD_CFG_STEP_CHG                       // change in runtime
   } ;

   enum PMD_CFG_CHANGE
   {
      PMD_CFG_CHANGE_INVALID         = 0,
      PMD_CFG_CHANGE_FORBIDDEN,              // DO NOT allow change
      PMD_CFG_CHANGE_RUN,                    // allow change in runtime
      PMD_CFG_CHANGE_REBOOT                  // allow change after reboot
   } ;

   enum PMD_CFG_DATA_TYPE
   {
      PMD_CFG_DATA_CMD        = 0,           // command
      PMD_CFG_DATA_BSON                      // BSON
   } ;

   /*
      _pmdParamValue define
   */
   struct _pmdParamValue
   {
      string         _value ;
      BOOLEAN        _hasMapped ;
      BOOLEAN        _hasField ;
      PMD_CFG_CHANGE _level;

      _pmdParamValue()
      {
         _hasMapped = FALSE ;
         _hasField = FALSE ;
         _level = PMD_CFG_CHANGE_INVALID ;
      }
      _pmdParamValue( INT32 value, BOOLEAN hasMappe,
                      BOOLEAN hasField,
                      PMD_CFG_CHANGE changeLevel = PMD_CFG_CHANGE_INVALID )
      {
         CHAR tmp[ 15 ] = { 0 } ;
         ossItoa( value, tmp, sizeof( tmp ) - 1 ) ;
         _value = tmp ;
         _hasMapped = hasMappe ;
         _hasField = hasField ;
         _level  = changeLevel ;
      }
      _pmdParamValue( const string &value, BOOLEAN hasMappe,
                      BOOLEAN hasField,
                      PMD_CFG_CHANGE changeLevel = PMD_CFG_CHANGE_INVALID )
      {
         _value = value ;
         _hasMapped = hasMappe ;
         _hasField = hasField ;
         _level  = changeLevel ;
      }
   } ;
   typedef _pmdParamValue pmdParamValue ;

   typedef map< string, pmdParamValue >      MAP_K2V ;

   /*
      Mask define
   */
   #define PMD_CFG_MASK_SKIP_UNFIELD         0x00000001
   #define PMD_CFG_MASK_SKIP_NORMALDFT       0x00000002
   #define PMD_CFG_MASK_SKIP_HIDEDFT         0x00000004
   #define PMD_CFG_MASK_MODE_LOCAL           0x00000008

   /*
      _pmdCfgExchange define
   */
   class _pmdCfgExchange : public SDBObject
   {
      friend class _pmdCfgRecord ;

      public:
         _pmdCfgExchange ( MAP_K2V *pMapField,
                           const BSONObj &dataObj,
                           BOOLEAN load = TRUE,
                           PMD_CFG_STEP step = PMD_CFG_STEP_INIT,
                           UINT32 mask = 0 ) ;
         _pmdCfgExchange ( MAP_K2V *pMapField,
                           MAP_K2V *pMapColdField,
                           const BSONObj &dataObj,
                           BOOLEAN load = TRUE,
                           PMD_CFG_STEP step = PMD_CFG_STEP_INIT,
                           UINT32 mask = 0 ) ;
         _pmdCfgExchange ( MAP_K2V *pMapField,
                           po::variables_map *pVMCmd,
                           po::variables_map *pVMFile = NULL,
                           BOOLEAN load = TRUE,
                           PMD_CFG_STEP step = PMD_CFG_STEP_INIT,
                           UINT32 mask = 0 ) ;
         ~_pmdCfgExchange () ;

         PMD_CFG_STEP   getCfgStep() const { return _cfgStep ; }
         BOOLEAN        isLoad() const { return _isLoad ; }
         BOOLEAN        isSave() const { return !_isLoad ; }

         void           setLoad() { _isLoad = TRUE ; }
         void           setSave() { _isLoad = FALSE ; }
         void           setCfgStep( PMD_CFG_STEP step ) { _cfgStep = step ; }
         void           setWhole( BOOLEAN isWhole ) { _isWhole = isWhole ; }

      public:
         INT32 readInt( const CHAR *pFieldName, INT32 &value,
                        INT32 defaultValue, PMD_CFG_CHANGE changeLevel ) ;
         INT32 readInt( const CHAR *pFieldName, INT32 &value,
                        PMD_CFG_CHANGE changeLevel ) ;

         INT32 readString( const CHAR *pFieldName, CHAR *pValue, UINT32 len,
                           const CHAR *pDefault, PMD_CFG_CHANGE changeLevel ) ;
         INT32 readString( const CHAR *pFieldName, CHAR *pValue, UINT32 len,
                           PMD_CFG_CHANGE changeLevel ) ;

         INT32 writeInt( const CHAR *pFieldName, INT32 value ) ;
         INT32 writeString( const CHAR *pFieldName, const CHAR *pValue ) ;

         BOOLEAN hasField( const CHAR *pFieldName ) ;
         BOOLEAN isSkipUnField() const
         {
            return ( _mask & PMD_CFG_MASK_SKIP_UNFIELD ) ? TRUE : FALSE ;
         }
         BOOLEAN isSkipHideDefault() const
         {
            return ( _mask & PMD_CFG_MASK_SKIP_HIDEDFT ) ? TRUE : FALSE ;
         }
         BOOLEAN isSkipNormalDefault() const
         {
            return ( _mask & PMD_CFG_MASK_SKIP_NORMALDFT ) ? TRUE : FALSE ;
         }
         BOOLEAN isLocalMode() const
         {
            return ( _mask & PMD_CFG_MASK_MODE_LOCAL ) ? TRUE : FALSE ;
         }
         BOOLEAN isWhole() const { return _isWhole ; }

      private:
         const CHAR *getData( UINT32 &dataLen, MAP_K2V &mapKeyValue ) ;
         MAP_K2V*    getKVMap() ;

         void        _makeKeyValueMap( po::variables_map *pVM ) ;
         void        _saveToMapInt( const CHAR * pFieldName,
                                    INT32 &value,const INT32 &newValue,
                                    PMD_CFG_CHANGE changeLevel,
                                    BOOLEAN useDefault ) ;
         void        _saveToMapString( const CHAR *pFieldName,
                                       CHAR *pValue, UINT32 len,
                                       const string &newValue,
                                       PMD_CFG_CHANGE changeLevel,
                                       BOOLEAN useDefault ) ;

      private:
         MAP_K2V*                _pMapKeyField ;
         MAP_K2V*                _pMapColdKeyField ;
         PMD_CFG_STEP            _cfgStep ;
         BOOLEAN                 _isLoad ;

         PMD_CFG_DATA_TYPE       _dataType ;
         //
         BSONObj                 _dataObj ;
         BSONObjBuilder          _dataBuilder ;
         po::variables_map       *_pVMFile ;
         po::variables_map       *_pVMCmd ;
         stringstream            _strStream ;
         string                  _dataStr ;

         UINT32                  _mask ;
         BOOLEAN                 _isWhole ;

   } ;
   typedef _pmdCfgExchange pmdCfgExchange ;

   /*
      _pmdAddrPair define
   */
   typedef class _pmdAddrPair
   {
      public :
         CHAR _host[ OSS_MAX_HOSTNAME + 1 ] ;
         CHAR _service[ OSS_MAX_SERVICENAME + 1 ] ;

      _pmdAddrPair()
      {
         ossMemset( _host, 0, sizeof( _host ) ) ;
         ossMemset( _service, 0, sizeof( _service ) ) ;
      }
      _pmdAddrPair( const string &hostname, const string &svcname )
      {
         ossStrncpy( _host, hostname.c_str(), OSS_MAX_HOSTNAME ) ;
         _host[ OSS_MAX_HOSTNAME ] = 0 ;
         ossStrncpy( _service, svcname.c_str(), OSS_MAX_SERVICENAME ) ;
         _service[ OSS_MAX_SERVICENAME ] = 0 ;
      }
   } pmdAddrPair ;

   /*
      _pmdCfgRecord define
   */
   class _pmdCfgRecord : public SDBObject, public _IParam
   {
      public:
         virtual  BOOLEAN hasField( const CHAR *pFieldName ) ;
         virtual  INT32   getFieldInt( const CHAR *pFieldName,
                                       INT32 &value,
                                       INT32 *pDefault = NULL ) ;
         virtual  INT32   getFieldStr( const CHAR *pFieldName,
                                       CHAR *pValue, UINT32 len,
                                       const CHAR *pDefault = NULL ) ;
         virtual  INT32   getFieldStr( const CHAR *pFieldName,
                                       std::string &strValue,
                                       const CHAR *pDefault = NULL ) ;

      public:
         _pmdCfgRecord () ;
         virtual ~_pmdCfgRecord () ;

         void  setConfigHandler( IConfigHandle *pConfigHandler ) ;
         IConfigHandle* getConfigHandler() const ;

         INT32 getResult () const { return _result ; }
         void  resetResult () { _result = SDB_OK ; }

         INT32 init( po::variables_map *pVMFile, po::variables_map *pVMCMD ) ;
         INT32 restore( const BSONObj &objData,
                        po::variables_map *pVMCMD ) ;
         INT32 change( const BSONObj &objData,
                       BOOLEAN isWhole = FALSE ) ;

         INT32 update( const BSONObj &userConfig,
                       BOOLEAN setForRestore,
                       BSONObj &errorObj ) ;

         INT32 toBSON ( BSONObj &objData,
                        UINT32 mask = PMD_CFG_MASK_SKIP_HIDEDFT ) ;
         INT32 toString( string &str,
                         UINT32 mask = PMD_CFG_MASK_SKIP_HIDEDFT ) ;

         UINT32 getChangeID () const { return _changeID ; }

      public:
         /*
            Parse address line(192.168.20.106:5000,192.168.30.102:1000...) to
            pmdAddrPair
         */
         INT32 parseAddressLine( const CHAR *pAddressLine,
                                 vector< pmdAddrPair > &vecAddr,
                                 const CHAR *pItemSep = ",",
                                 const CHAR *pInnerSep = ":",
                                 UINT32 maxSize = CLS_REPLSET_MAX_NODE_SIZE ) const ;

         string makeAddressLine( const vector< pmdAddrPair > &vecAddr,
                                 CHAR chItemSep = ',',
                                 CHAR chInnerSep = ':' ) const ;

      protected:
         INT32  _addToFieldMap( const string &key, INT32 value,
                                BOOLEAN hasMapped = TRUE,
                                BOOLEAN hasField = TRUE ) ;
         INT32  _addToFieldMap( const string &key, const string &value,
                                BOOLEAN hasMapped = TRUE,
                                BOOLEAN hasField = TRUE ) ;
         void  _updateFieldMap( pmdCfgExchange *pEX,
                                const CHAR *pFieldName,
                                const CHAR *pValue,
                                PMD_CFG_CHANGE changeLevel ) ;
         void  _purgeFieldMap( MAP_K2V &mapKeyField ) ;
         INT32  _saveUpdateChange( MAP_K2V &mapKeyField,
                                   MAP_K2V &mapColdKeyField,
                                   BOOLEAN setForRestore,
                                   BSONObj &errorObj ) ;

      protected:
         virtual INT32 doDataExchange( pmdCfgExchange *pEX ) = 0 ;
         virtual INT32 postLoaded( PMD_CFG_STEP step ) ;
         virtual INT32 preSaving() ;

      protected:
         INT32 rdxInt( pmdCfgExchange *pEX, const CHAR *pFieldName,
                       INT32 &value, BOOLEAN required, PMD_CFG_CHANGE changeLevel,
                       INT32 defaultValue, BOOLEAN hideParam = FALSE ) ;

         INT32 rdxUInt( pmdCfgExchange *pEX, const CHAR *pFieldName,
                        UINT32 &value, BOOLEAN required, PMD_CFG_CHANGE changeLevel,
                        UINT32 defaultValue, BOOLEAN hideParam = FALSE ) ;

         INT32 rdxShort( pmdCfgExchange *pEX, const CHAR *pFieldName,
                         INT16 &value, BOOLEAN required, PMD_CFG_CHANGE changeLevel,
                         INT16 defaultValue, BOOLEAN hideParam = FALSE ) ;

         INT32 rdxUShort( pmdCfgExchange *pEX, const CHAR *pFieldName,
                          UINT16 &value, BOOLEAN required, PMD_CFG_CHANGE changeLevel,
                          UINT16 defaultValue, BOOLEAN hideParam = FALSE ) ;

         INT32 rdxString( pmdCfgExchange *pEX, const CHAR *pFieldName,
                          CHAR *pValue, UINT32 len, BOOLEAN required,
                          PMD_CFG_CHANGE changeLevel, const CHAR *pDefaultValue,
                          BOOLEAN hideParam = FALSE ) ;

         INT32 rdxPath( pmdCfgExchange *pEX, const CHAR *pFieldName,
                        CHAR *pValue, UINT32 len, BOOLEAN required,
                        PMD_CFG_CHANGE changeLevel, const CHAR *pDefaultValue,
                        BOOLEAN hideParam = FALSE ) ;

         INT32 rdxPathRaw( pmdCfgExchange *pEX, const CHAR *pFieldName,
                           CHAR *pValue, UINT32 len, BOOLEAN required,
                           PMD_CFG_CHANGE changeLevel, const CHAR *pDefaultValue,
                           BOOLEAN hideParam = FALSE ) ;

         INT32 rdxBooleanS( pmdCfgExchange *pEX, const CHAR *pFieldName,
                            BOOLEAN &value, BOOLEAN required,
                            PMD_CFG_CHANGE changeLevel, BOOLEAN defaultValue,
                            BOOLEAN hideParam = FALSE ) ;

         INT32 rdvMinMax( pmdCfgExchange *pEX, UINT32 &value,
                          UINT32 minV, UINT32 maxV,
                          BOOLEAN autoAdjust = TRUE ) ;
         INT32 rdvMinMax( pmdCfgExchange *pEX, INT32 &value,
                          INT32 minV, INT32 maxV,
                          BOOLEAN autoAdjust = TRUE ) ;
         INT32 rdvMinMax( pmdCfgExchange *pEX, UINT16 &value,
                          UINT16 minV, UINT16 maxV,
                          BOOLEAN autoAdjust = TRUE ) ;
         INT32 rdvMaxChar( pmdCfgExchange *pEX, CHAR *pValue,
                           UINT32 maxChar, BOOLEAN autoAdjust = TRUE ) ;
         INT32 rdvNotEmpty( pmdCfgExchange *pEX, CHAR *pValue ) ;

      private:
         INT32 _rdxPath( pmdCfgExchange *pEX, const CHAR *pFieldName,
                         CHAR *pValue, UINT32 len, BOOLEAN required,
                         PMD_CFG_CHANGE changeLevel, const CHAR *pDefaultValue,
                         BOOLEAN hideParam,
                         BOOLEAN addSep ) ;

      private:
         string                              _curFieldName ;
         INT32                               _result ;
         UINT32                              _changeID ;
         IConfigHandle                       *_pConfigHander ;

         MAP_K2V                             _mapKeyValue ;
         MAP_K2V                             _mapColdKeyValue ;
      protected:
         ossSpinXLatch                       _mutex ;

   } ;
   typedef _pmdCfgRecord pmdCfgRecord ;

   #define PMD_OPT_VALUE_NONE                ( 0 )
   #define PMD_OPT_VALUE_FULLSYNC            ( 1 )
   #define PMD_OPT_VALUE_SHUTDOWN            ( 2 )

   /*
      _pmdOptionsMgr define
   */
   class _pmdOptionsMgr : public _pmdCfgRecord
   {
      public:
         _pmdOptionsMgr() ;
         ~_pmdOptionsMgr() ;

      protected:
         virtual INT32 doDataExchange( pmdCfgExchange *pEX ) ;
         virtual INT32 postLoaded( PMD_CFG_STEP step ) ;
         virtual INT32 preSaving() ;

      public:

         INT32 init( INT32 argc, CHAR **argv,
                     const std::string &exePath = "" ) ;

         INT32 initFromFile( const CHAR *pConfigFile,
                             BOOLEAN allowFileNotExist = FALSE ) ;

         INT32 removeAllDir() ;

         INT32 makeAllDir() ;

         INT32 reflush2File( UINT32 mask = PMD_CFG_MASK_SKIP_UNFIELD ) ;

      public:
         OSS_INLINE const CHAR *getConfPath() const
         {
            return _krcbConfPath;
         }
         OSS_INLINE const CHAR *getConfFile() const
         {
            return _krcbConfFile ;
         }
         OSS_INLINE const CHAR *getCatFile() const
         {
            return _krcbCatFile ;
         }
         OSS_INLINE const CHAR *getReplLogPath() const
         {
            return _krcbLogPath;
         }
         OSS_INLINE const CHAR *getDbPath() const
         {
            return _krcbDbPath;
         }
         OSS_INLINE const CHAR *getIndexPath() const
         {
            return _krcbIndexPath;
         }
         OSS_INLINE const CHAR *getBkupPath() const
         {
            return _krcbBkupPath;
         }
         OSS_INLINE const CHAR *getWWWPath() const
         {
            return _krcbWWWPath ;
         }
         OSS_INLINE const CHAR *getLobPath() const
         {
            return _krcbLobPath ;
         }
         OSS_INLINE const CHAR *getLobMetaPath() const
         {
            return _krcbLobMetaPath ;
         }
         OSS_INLINE const CHAR *getDiagLogPath() const
         {
            return _krcbDiagLogPath;
         }
         OSS_INLINE const CHAR *getAuditLogPath() const
         {
            return _krcbAuditLogPath ;
         }
         OSS_INLINE UINT32 getMaxPooledEDU() const
         {
            return _krcbMaxPool;
         }
         OSS_INLINE UINT16 getServicePort() const
         {
            return _krcbSvcPort;
         }
         OSS_INLINE UINT16 getDiagLevel() const
         {
            return _krcbDiagLvl;
         }
         OSS_INLINE const CHAR *krcbRole() const
         {
            return _krcbRole;
         }
         OSS_INLINE const CHAR *replService() const
         {
            return _replServiceName ;
         }
         OSS_INLINE const CHAR *catService() const
         {
            return _catServiceName ;
         }
         OSS_INLINE const CHAR *shardService() const
         {
            return _shardServiceName ;
         }
         OSS_INLINE const CHAR *getRestService() const
         {
            return _restServiceName ;
         }
         OSS_INLINE const CHAR *getOMService() const
         {
            return _omServiceName ;
         }
         OSS_INLINE const CHAR *getServiceAddr() const
         {
            return _krcbSvcName ;
         }
         OSS_INLINE vector< _pmdAddrPair > catAddrs() const
         {
            return _vecCat ;
         }
         OSS_INLINE vector< _pmdAddrPair > omAddrs() const
         {
            return _vecOm ;
         }
         OSS_INLINE const CHAR *getTmpPath() const
         {
            return _dmsTmpBlkPath ;
         }
         std::string getCatAddr() const ;

         OSS_INLINE UINT32 getSortBufSize() const
         {
            return _sortBufSz ;
         }

         OSS_INLINE UINT32 getHjBufSize() const
         {
            return _hjBufSz ;
         }

         void clearCatAddr() ;
         void rmCatAddrItem( const CHAR *host, const CHAR *service ) ;
         void setCatAddr( const CHAR *host, const CHAR *service ) ;

         OSS_INLINE UINT32 catNum() const { return CATA_NODE_MAX_NUM ; }
         OSS_INLINE UINT64 getReplLogFileSz () const
         {
            return (UINT64)_logFileSz * DPS_LOG_FILE_SIZE_UNIT ;
         }
         OSS_INLINE UINT32 getReplLogFileNum () const { return _logFileNum ; }
         OSS_INLINE UINT32 numPreLoaders () const { return _numPreLoaders ; }
         OSS_INLINE UINT32 maxPrefPool () const { return _maxPrefPool ; }
         OSS_INLINE UINT32 maxSubQuery () const { return _maxSubQuery ; }
         OSS_INLINE UINT32 maxReplSync () const { return _maxReplSync ; }
         OSS_INLINE INT32  syncStrategy () const { return _syncStrategy ; }
         OSS_INLINE UINT32 replBucketSize () const { return _replBucketSize ; }
         OSS_INLINE BOOLEAN transactionOn () const { return _transactionOn ; }
         OSS_INLINE UINT32 transTimeout () const { return _transTimeout; }
         OSS_INLINE INT32 transIsolation () const { return _transIsolation; }
         OSS_INLINE BOOLEAN transLockwait () const { return _transLockwait; }
         OSS_INLINE BOOLEAN transAutoCommit() const { return _transAutoCommit ; }
         OSS_INLINE BOOLEAN transAutoRollback() const { return _transAutoRollback ; }
         OSS_INLINE BOOLEAN transUseRBS() const { return _transUseRBS ; }
         OSS_INLINE BOOLEAN memDebugEnabled () const { return _memDebugEnabled ; }
         OSS_INLINE BOOLEAN memDebugDetail() const { return _memDebugDetail ; }
         OSS_INLINE BOOLEAN memDebugVerify() const { return _memDebugVerify ; }
         OSS_INLINE UINT32 memDebugMask() const { return _memDebugMask ; }
         OSS_INLINE UINT32 memDebugSize () const { return _memDebugSize ; }
         OSS_INLINE UINT32 indexScanStep () const { return _indexScanStep ; }
         OSS_INLINE UINT32 getReplLogBuffSize () const { return _logBuffSize ; }
         OSS_INLINE const CHAR* dbroleStr() const { return _krcbRole ; }
         OSS_INLINE INT32 diagFileNum() const { return _dialogFileNum ; }
         OSS_INLINE INT32 auditFileNum() const { return _auditFileNum ; }
         OSS_INLINE UINT32 auditMask() const { return _auditMask ; }
         OSS_INLINE UINT32 ftMask() const { return _ftMask ; }
         OSS_INLINE UINT32 ftConfirmPeriod() const { return _ftConfirmPeriod ; }
         OSS_INLINE UINT32 ftConfirmRatio() const { return _ftConfirmRatio ; }
         OSS_INLINE INT32  ftLevel() const { return _ftLevel ; }
         OSS_INLINE UINT32 ftFusingTimeout() const { return _ftFusingTimeout ; }
         OSS_INLINE UINT32 ftSlowNodeThreshold() const { return _ftSlowNodeThreshold ; }
         OSS_INLINE UINT32 ftSlowNodeIncrement() const { return _ftSlowNodeIncrement ; }
         OSS_INLINE UINT32 syncwaitTimeout() const { return _syncwaitTimeout ; }
         OSS_INLINE UINT32 shutdownWaitTimeout() const { return _shutdownWaitTimeout ; }
         OSS_INLINE BOOLEAN isDpsLocal() const { return _dpslocal ; }
         OSS_INLINE UINT32 sharingBreakTime() const { return _sharingBreakTime ; }
         OSS_INLINE UINT32 startShiftTime() const { return _startShiftTime * OSS_ONE_SEC ; }
         OSS_INLINE BOOLEAN isTraceOn() const { return _traceOn ; }
         OSS_INLINE UINT32 traceBuffSize() const { return _traceBufSz ; }
         OSS_INLINE BOOLEAN useDirectIOInLob() const { return _directIOInLob ; }
         OSS_INLINE BOOLEAN sparseFile() const { return _sparseFile ; }
         OSS_INLINE UINT8 weight() const { return (UINT8)_weight ; }
         OSS_INLINE BOOLEAN authEnabled() const { return _auth ; }
         OSS_INLINE UINT32 getPlanBuckets() const { return _planBucketNum ; }
         OSS_INLINE UINT32 getOprTimeout() const { return _oprtimeout ; }
         OSS_INLINE UINT32 getOverFlowRatio() const { return _overflowRatio ; }
         OSS_INLINE UINT32 getExtendThreshold() const { return _extendThreshold ; }
         OSS_INLINE UINT32 getSignalInterval() const { return _signalInterval ; }
         OSS_INLINE UINT32 getMaxCacheSize() const { return _maxCacheSize ; }
         OSS_INLINE UINT32 getMaxCacheJob() const { return _maxCacheJob ; }
         OSS_INLINE UINT32 getMaxSyncJob() const { return _maxSyncJob ; }
         OSS_INLINE UINT32 getSyncInterval() const { return _syncInterval ; }
         OSS_INLINE UINT32 getSyncRecordNum() const { return _syncRecordNum ; }
         OSS_INLINE UINT32 getSyncDirtyRatio() const { return 0 ; /* Reserved */ }
         OSS_INLINE BOOLEAN isSyncDeep() const { return _syncDeep ; }

         OSS_INLINE BOOLEAN archiveOn() const { return _archiveOn ; }
         OSS_INLINE BOOLEAN archiveCompressOn() const { return _archiveCompressOn ; }
         OSS_INLINE const CHAR* getArchivePath() const { return _archivePath ; }
         OSS_INLINE UINT32 getArchiveTimeout() const { return _archiveTimeout ; }
         OSS_INLINE UINT32 getArchiveExpired() const { return _archiveExpired ; }
         OSS_INLINE UINT32 getArchiveQuota() const { return _archiveQuota ; }

         OSS_INLINE UINT32 getDmsChkInterval() const { return _dmsChkInterval ; }
         OSS_INLINE UINT32 getCacheMergeSize() const { return _cacheMergeSize << 20 ; }
         OSS_INLINE UINT32 getPageAllocTimeout() const { return _pageAllocTimeout ; }
         OSS_INLINE BOOLEAN isEnabledPerfStat() const { return _perfStat ; }
         OSS_INLINE INT32 getOptCostThreshold() const { return _optCostThreshold ; }
         OSS_INLINE BOOLEAN isEnabledMixCmp() const { return _enableMixCmp ; }
         OSS_INLINE UINT32  getDataErrorOp() const { return _dataErrorOp ; }
         OSS_INLINE UINT32 getPlanCacheLevel() const { return _planCacheLevel ; }
         OSS_INLINE const CHAR * getPrefInstStr () const { return _prefInstStr ; }
         OSS_INLINE const CHAR * getPrefInstModeStr () const { return _prefInstModeStr ; }
         OSS_INLINE BOOLEAN isPreferedStrict() const { return _preferedStrict ; }
         OSS_INLINE INT32 getPreferedPeriod() const { return _preferedPeriod ; }
         OSS_INLINE UINT32 getInstanceID () const { return _instanceID ; }
         OSS_INLINE UINT32 getMaxConn () const { return _maxconn ; }
         OSS_INLINE UINT32 getSvcSchedulerType() const { return _svcSchedulerType ; }
         OSS_INLINE UINT32 getSvcMaxConcurrency() const { return _svcMaxConcurrency ; }
         OSS_INLINE UINT32 logWriteMod() const { return _logWriteMod ; }
         OSS_INLINE BOOLEAN logTimeOn() const { return _logTimeOn ; }
         OSS_INLINE BOOLEAN isSleepEnabled() const { return _enableSleep ; }
         OSS_INLINE BOOLEAN recycleRecord() const { return _recycleRecord ; }
         OSS_INLINE UINT32 maxSockPerNode() const { return _maxSockPerNode ; }
         OSS_INLINE UINT32 maxSockPerThread() const { return _maxSockPerThread ; }
         OSS_INLINE UINT32 maxSockThread() const { return _maxSockThread ; }
         OSS_INLINE UINT32 maxTCSize() const { return _maxTCSize ; }
         OSS_INLINE UINT32 memPoolSize() const { return _memPoolSize ; }
         OSS_INLINE INT32  transReplSize() const { return _transReplSize ; }
         OSS_INLINE BOOLEAN transRCCount() const { return _transRCCount ; }
         OSS_INLINE UINT32 slowQueryThreshold() const { return _slowQueryThreshold ; }
         OSS_INLINE UINT32 monGroupMask() const { return _monGroupMask ; }
         OSS_INLINE UINT32 monHistEvent() const { return _monHistEvent ; }

         std::string getOmAddr() const ;

#ifdef SDB_ENTERPRISE

#ifdef SDB_SSL
         OSS_INLINE BOOLEAN useSSL() const { return _useSSL ; }
#endif

#endif /* SDB_ENTERPRISE */
      protected: // rdx members
         CHAR        _krcbDbPath[ OSS_MAX_PATHSIZE + 1 ] ;
         CHAR        _krcbIndexPath[ OSS_MAX_PATHSIZE + 1 ] ;
         CHAR        _krcbDiagLogPath[ OSS_MAX_PATHSIZE + 1 ] ;
         CHAR        _krcbAuditLogPath [ OSS_MAX_PATHSIZE + 1 ] ;
         CHAR        _krcbLogPath[ OSS_MAX_PATHSIZE + 1 ] ;
         CHAR        _krcbBkupPath[ OSS_MAX_PATHSIZE + 1 ] ;
         CHAR        _krcbWWWPath[ OSS_MAX_PATHSIZE + 1 ] ;
         CHAR        _krcbLobPath[ OSS_MAX_PATHSIZE + 1 ] ;
         CHAR        _krcbLobMetaPath[ OSS_MAX_PATHSIZE + 1 ] ;
         UINT32      _krcbMaxPool ;
         CHAR        _krcbSvcName[ OSS_MAX_SERVICENAME + 1 ] ;
         CHAR        _replServiceName[ OSS_MAX_SERVICENAME + 1 ] ;
         CHAR        _catServiceName[ OSS_MAX_SERVICENAME + 1 ] ;
         CHAR        _shardServiceName[ OSS_MAX_SERVICENAME + 1 ] ;
         CHAR        _restServiceName[ OSS_MAX_SERVICENAME + 1 ] ;
         CHAR        _omServiceName[ OSS_MAX_SERVICENAME + 1 ] ;
         UINT16      _krcbDiagLvl ;
         CHAR        _krcbRole[ PMD_MAX_ENUM_STR_LEN + 1 ] ;
         CHAR        _syncStrategyStr[ PMD_MAX_ENUM_STR_LEN + 1 ] ;
         CHAR        _prefInstStr[ PMD_MAX_LONG_STR_LEN + 1 ] ;
         CHAR        _prefInstModeStr[ PMD_MAX_SHORT_STR_LEN + 1 ] ;
         CHAR        _auditMaskStr[ PMD_MAX_LONG_STR_LEN + 1 ] ;
         CHAR        _ftMaskStr[ PMD_MAX_LONG_STR_LEN + 1 ] ;
         CHAR        _memDebugMaskStr[ PMD_MAX_LONG_STR_LEN + 1 ] ;
         CHAR        _monGroupMaskStr[ PMD_MAX_LONG_STR_LEN + 1 ] ;
         UINT32      _logFileSz ;
         UINT32      _logFileNum ;
         UINT32      _numPreLoaders ;
         UINT32      _maxPrefPool ;
         UINT32      _maxSubQuery ;
         UINT32      _maxReplSync ;
         UINT32      _replBucketSize ;
         INT32       _syncStrategy ;
         UINT32      _dataErrorOp ;
         BOOLEAN     _memDebugEnabled ;
         BOOLEAN     _memDebugVerify ;
         BOOLEAN     _memDebugDetail ;
         UINT32      _memDebugMask ;
         UINT32      _memDebugSize ;
         UINT32      _indexScanStep ;
         BOOLEAN     _dpslocal ;
         BOOLEAN     _traceOn ;
         UINT32      _traceBufSz ;
         BOOLEAN     _transactionOn ;
         UINT32      _transTimeout ;
         INT32       _transIsolation ;
         BOOLEAN     _transLockwait ;
         BOOLEAN     _transAutoCommit ;
         BOOLEAN     _transAutoRollback ;
         BOOLEAN     _transUseRBS ;
         UINT32      _sharingBreakTime ;
         UINT32      _startShiftTime ;
         UINT32      _logBuffSize ;
         CHAR        _catAddrLine[ OSS_MAX_PATHSIZE + 1 ] ;
         CHAR        _dmsTmpBlkPath[ OSS_MAX_PATHSIZE + 1 ] ;
         UINT32      _sortBufSz ;
         UINT32      _hjBufSz ;
         INT32       _dialogFileNum ;
         INT32       _auditFileNum ;
         UINT32      _auditMask ;
         UINT32      _ftMask ;
         UINT32      _ftConfirmPeriod ;
         UINT32      _ftConfirmRatio ;
         INT32       _ftLevel ;
         UINT32      _ftFusingTimeout ;
         UINT32      _ftSlowNodeThreshold ;
         UINT32      _ftSlowNodeIncrement ;
         UINT32      _syncwaitTimeout ;
         UINT32      _shutdownWaitTimeout ;
         BOOLEAN     _directIOInLob ;
         BOOLEAN     _sparseFile ;
         UINT32      _weight ;
         BOOLEAN     _auth ;
         UINT32      _planBucketNum ;
         UINT32      _oprtimeout ;
         UINT32      _overflowRatio ;     // %
         UINT32      _extendThreshold ;   // MB
         UINT32      _signalInterval ;
         UINT32      _maxCacheSize ;      // MB
         UINT32      _maxCacheJob ;
         UINT32      _maxSyncJob ;
         UINT32      _syncInterval ;
         UINT32      _syncRecordNum ;
         BOOLEAN     _syncDeep ;
         CHAR        _omAddrLine[ OSS_MAX_PATHSIZE + 1 ] ;
         BOOLEAN     _archiveOn ;
         BOOLEAN     _archiveCompressOn ;
         CHAR        _archivePath[ OSS_MAX_PATHSIZE + 1 ] ;
         UINT32      _archiveTimeout ;
         UINT32      _archiveExpired ;
         UINT32      _archiveQuota ;
         UINT32      _dmsChkInterval ;
         UINT32      _cacheMergeSize ;
         UINT32      _pageAllocTimeout ;  // ms
         BOOLEAN     _perfStat ;
         INT32       _optCostThreshold ;
         BOOLEAN     _enableMixCmp ;
         UINT32      _planCacheLevel ;
         UINT32      _instanceID ;
         UINT32      _maxconn;
         UINT32      _svcSchedulerType ;
         UINT32      _svcMaxConcurrency ;
         BOOLEAN     _preferedStrict ;
         INT32       _preferedPeriod ;
         CHAR        _logWriteModStr[ PMD_MAX_LOGMOD_STR_LEN + 1 ] ;
         UINT32      _logWriteMod ;
         BOOLEAN     _logTimeOn ;
         BOOLEAN     _enableSleep ;
         BOOLEAN     _recycleRecord ;
         UINT32      _maxSockPerNode ;
         UINT32      _maxSockPerThread ;
         UINT32      _maxSockThread ;
         UINT32      _maxTCSize ;
         UINT32      _memPoolSize ;
         INT32       _transReplSize ;
         BOOLEAN     _transRCCount ;
         UINT32      _slowQueryThreshold ;
         UINT32      _monGroupMask ;
         UINT32      _monHistEvent ;

#ifdef SDB_ENTERPRISE

#ifdef SDB_SSL
         BOOLEAN     _useSSL;
#endif

#endif /* SDB_ENTERPRISE */
      private: // other configs
         CHAR        _krcbConfPath[ OSS_MAX_PATHSIZE + 1 ] ;
         CHAR        _krcbConfFile[ OSS_MAX_PATHSIZE + 1 ] ;
         CHAR        _krcbCatFile[ OSS_MAX_PATHSIZE + 1 ] ;
         vector< pmdAddrPair >   _vecCat ;
         UINT16      _krcbSvcPort ;
         std::string _exePath ;
         vector< pmdAddrPair >   _vecOm ;

   } ;

   typedef _pmdOptionsMgr pmdOptionsCB ;

   INT32 optString2LogMod( const CHAR *str, UINT32 &value ) ;
   INT32 optString2MonGroupMask( const CHAR *str, UINT32 &value ) ;
   INT32 optLogMod2String( UINT32 value, CHAR *str, INT32 len ) ;

}

#endif //PMDOPTIONSMGR_HPP_

