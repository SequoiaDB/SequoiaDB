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

   Source File Name = pmdOptionsMgr.cpp

   Descriptive Name = Process MoDel Main

   When/how to use: this program may be used for managing sequoiadb
   configuration.It can be initialized from cmd and configure file.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "pmdOptionsMgr.hpp"
#include "dmsDef.hpp"
#include "pd.hpp"
#include "utilCommon.hpp"
#include "utilStr.hpp"
#include "msg.hpp"
#include "msgCatalog.hpp"
#include "ossMem.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"
#include "ossIO.hpp"
#include "ossVer.hpp"
#include "ossPath.hpp"
#include "dpsLogWrapper.hpp"
#include "omStrategyDef.hpp"
#include "optCommon.hpp"
#include "dpsTransDef.hpp"
#include "monMgr.hpp"
#include "rtnContextDef.hpp"
#include "rtnSortDef.hpp"
#include "clsUtil.hpp"
#include "wiredtiger/dmsWTDef.hpp"
#include <vector>
#include <boost/algorithm/string.hpp>

using namespace bson ;

namespace engine
{

   #define PMD_OPTION_LOG_WRITEMOD_INCREMENT_STR "increment"
   #define PMD_OPTION_LOG_WRITEMOD_FULL_STR      "full"

   #define JUDGE_RC( rc ) if ( SDB_OK != rc ) { goto error ; }
   #define ISALLOWRUNCHANGE( level )   ( ( level == PMD_CFG_CHANGE_REBOOT || \
                                       level == PMD_CFG_CHANGE_FORBIDDEN ) ? \
                                       FALSE : TRUE )
   #define PMD_OPTION_BRK_TIME_DEFAULT (7000)
   #define PMD_OPTION_OPR_TIME_DEFAULT (60000)
   #define PMD_OPTION_DFT_MAXPOOL      (50)
   #define PMD_MAX_PREF_POOL           (0) // modify 200 to 0
   #define PMD_MAX_SUB_QUERY           (10)
   #define PMD_MIN_SORTBUF_SZ          (RTN_SORT_MIN_BUFSIZE)
   #define PMD_DEFAULT_SORTBUF_SZ      (256)
   #define PMD_DEFAULT_HJ_SZ           (128)
   #define PMD_MIN_HJ_SZ               (64)
   #define PMD_DEFAULT_MAX_REPLSYNC    (10)
   #define PMD_DFT_REPL_BUCKET_SIZE    (32)
   #define PMD_DFT_INDEX_SCAN_STEP     (100)
   #define PMD_DFT_START_SHIFT_TIME    (600)
   #define PMD_MAX_NUMPAGECLEAN        (50)
   #define PMD_MIN_PAGECLEANINTERVAL   (1000)
   #define PMD_DFT_OVERFLOW_RETIO      (12)
   #define PMD_DFT_EXTEND_THRESHOLD    (32)
   #define PMD_DFT_MAX_CACHE_JOB       (10)
   #define PMD_DFT_MAX_SYNC_JOB        (10)
   #define PMD_DFT_SYNC_INTERVAL       (10000)  // 10 seconds
   #define PMD_DFT_SYNC_RECORDNUM      (0)
   #define PMD_DFT_SYNCWAIT_TIMEOUT    (600)
   #define PMD_DFT_SHUTDOWN_WAIT_TIMEOUT  (1200)
   #define PMD_DFT_FTFUSING_TIMEOUT    (10)
   #define PMD_DFT_FTSLOWNODE_THRESHOLD   ( 256 )
   #define PMD_DFT_FTSLOWNODE_INCREMENT   ( 8 )
   #define PMD_DFT_ARCHIVE_TIMEOUT     (600) // 10 minutes
   #define PMD_DFT_ARCHIVE_EXPIRED     (240) // 10 days
   #define PMD_MAX_ARCHIVE_EXPIRED     (4294967295/360) // hours to minutes
   #define PMD_DFT_ARCHIVE_QUOTA       (10)  // 10 GB
   #define PMD_DFT_DMS_CHK_INTERVAL    (0)   // disable
   #define PMD_DFT_CACHE_MERGE_SZ      (0)   // ms
   #define PMD_DFT_PAGE_ALLOC_TIMEOUT  (0)
   #define PMD_DFT_OPT_COST_THRESHOLD  (20)
   #define PMD_DFT_PLAN_CACHE_MAINCL_THRESHOLD (20)
   #define PMD_DFT_ENABLE_MIX_CMP      (FALSE)
   #define PMD_DFT_PREFINST            ( PREFER_INSTANCE_MASTER_STR )
   #define PMD_DFT_PREFINST_MODE       ( PREFER_INSTANCE_RANDOM_STR )
   #define PMD_DFT_PREF_CONSTRAINT     ("")
   #define PMD_DFT_INSTANCE_ID         ( NODE_INSTANCE_ID_UNKNOWN )
   #define PMD_DFT_PREFINST_PERIOD     ( PREFER_INSTANCE_DEF_PERIOD )
   #define PMD_DFT_MAX_CONN            (0)   // unlimited
   #define PMD_DFT_LOGWRITEMOD         ( PMD_OPTION_LOG_WRITEMOD_INCREMENT_STR )
   #define PMD_DFT_METACACHE_EXPIRED   (30) // half an hour
   #define PMD_MAX_METACACHE_EXPIRED   (43200) // 30 days
   #define PMD_DFT_METACACHE_LWM       (512)
   #define PMD_MAX_METACACHE_LWM       (10240)
   #define PMD_DFT_STAT_MCV_LIMIT      (200000)  // number of sample records
   #define PMD_MAX_STAT_MCV_LIMIT      (2000000)

   #define PMD_DFT_USER_CACHE_INTERVAL  (300000)  // milliseconds, 5 miniutes

   #define PMD_DFT_MEM_MXFAST          (-1)
   #define PMD_DFT_MEM_TRIM_THRESHOLD  (256)
   #define PMD_DFT_MEM_MMAP_THRESHOLD  (1024)
   #define PMD_DFT_MEM_MMAP_MAX        (4194304)
   #define PMD_DFT_MEM_TOP_PAD         (-1)

   /*
      _pmdCfgExchange implement
   */
   _pmdCfgExchange::_pmdCfgExchange( MAP_K2V *pMapField,
                                     const BSONObj &dataObj,
                                     BOOLEAN load,
                                     PMD_CFG_STEP step,
                                     UINT32 mask )
   :_pMapKeyField( pMapField ), _cfgStep( step ),
    _isLoad( load ), _dataObj( dataObj ), _mask( mask )
   {
      _dataType   = PMD_CFG_DATA_BSON ;
      _pVMFile    = NULL ;
      _pVMCmd     = NULL ;
      _isWhole    = FALSE ;
      _pMapColdKeyField = NULL ;

      SDB_ASSERT( _pMapKeyField, "Map key field can't be NULL" ) ;
   }

   _pmdCfgExchange::_pmdCfgExchange ( MAP_K2V *pMapField,
                                      MAP_K2V *pMapColdField,
                                      const BSONObj &dataObj,
                                      BOOLEAN load,
                                      PMD_CFG_STEP step,
                                      UINT32 mask )
   :_pMapKeyField( pMapField ), _pMapColdKeyField( pMapColdField ),
   _cfgStep( step ),  _isLoad( load ), _dataObj( dataObj ), _mask( mask )
   {
      _dataType   = PMD_CFG_DATA_BSON ;
      _pVMFile    = NULL ;
      _pVMCmd     = NULL ;
      _isWhole    = FALSE ;

      SDB_ASSERT( _pMapKeyField, "Map key field can't be NULL" ) ;
      SDB_ASSERT( _pMapColdKeyField, "Map cold key field can't be NULL" ) ;
   }

   _pmdCfgExchange::_pmdCfgExchange( MAP_K2V *pMapField,
                                     po::variables_map *pVMCmd,
                                     po::variables_map * pVMFile,
                                     BOOLEAN load,
                                     PMD_CFG_STEP step,
                                     UINT32 mask )
   :_pMapKeyField( pMapField ), _cfgStep( step ), _isLoad( load ),
    _pVMFile( pVMFile ), _pVMCmd( pVMCmd ), _mask( mask )
   {
      _dataType   = PMD_CFG_DATA_CMD ;
      _isWhole    = FALSE ;
      _pMapColdKeyField = NULL ;

      SDB_ASSERT( _pMapKeyField, "Map key field can't be NULL" ) ;
   }

   _pmdCfgExchange::~_pmdCfgExchange()
   {
      _pVMFile    = NULL ;
      _pVMCmd     = NULL ;
   }

   INT32 _pmdCfgExchange::readInt( const CHAR * pFieldName, INT32 &value,
                                   PMD_CFG_CHANGE changeLevel )
   {
      INT32 rc = SDB_OK ;
      INT32 readValue ;

      if ( PMD_CFG_DATA_BSON == _dataType )
      {
         BSONElement ele = _dataObj.getField( pFieldName ) ;
         if ( ele.eoo() )
         {
            rc = SDB_FIELD_NOT_EXIST ;
         }
         else if ( !ele.isNumber() )
         {
            PD_LOG( PDERROR, "Field[%s] type[%d] is not number", pFieldName,
                    ele.type() ) ;
            rc = SDB_INVALIDARG ;
         }
         else
         {
            readValue = (INT32)ele.numberInt() ;
         }
      }
      else if ( PMD_CFG_DATA_CMD == _dataType )
      {
         if ( _pVMCmd && _pVMCmd->count( pFieldName ) )
         {
            readValue = (*_pVMCmd)[pFieldName].as<int>() ;
         }
         else if ( _pVMFile && _pVMFile->count( pFieldName ) )
         {
            readValue = (*_pVMFile)[pFieldName].as<int>() ;
         }
         else
         {
            rc = SDB_FIELD_NOT_EXIST ;
         }
      }
      else
      {
         rc = SDB_SYS ;
      }

      if ( SDB_OK == rc )
      {
         _saveToMapInt(pFieldName, value, readValue, changeLevel, FALSE ) ;
      }

      return rc ;
   }

   INT32 _pmdCfgExchange::readInt( const CHAR * pFieldName, INT32 &value,
                                   INT32 defaultValue, PMD_CFG_CHANGE changeLevel )
   {
      INT32 rc = SDB_OK ;

      rc = readInt( pFieldName, value, changeLevel ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         _saveToMapInt(pFieldName, value, defaultValue, changeLevel, TRUE ) ;
         rc = SDB_OK ;
      }

      return rc ;
   }

   INT32 _pmdCfgExchange::readString( const CHAR *pFieldName, CHAR *pValue,
                                      UINT32 len, PMD_CFG_CHANGE changeLevel )
   {
      INT32 rc = SDB_OK ;
      string readValue ;

      if ( PMD_CFG_DATA_BSON == _dataType )
      {
         BSONElement ele = _dataObj.getField( pFieldName ) ;
         if ( ele.eoo() )
         {
            rc = SDB_FIELD_NOT_EXIST ;
         }
         else if ( NumberInt == ele.type() )
         {
            stringstream ss ;
            ss << (INT32)ele.numberInt() ;
            readValue =  ss.str() ;
         }
         else if ( String == ele.type() )
         {
            readValue = ele.String() ;
         }
         else
         {
            PD_LOG( PDERROR, "Field[%s] type[%d] is not string or int",
                    pFieldName, ele.type() ) ;
            rc = SDB_INVALIDARG ;
         }
      }
      else if ( PMD_CFG_DATA_CMD == _dataType )
      {
         if ( _pVMCmd && _pVMCmd->count( pFieldName ) )
         {
            readValue = (*_pVMCmd)[pFieldName].as<string>() ;
         }
         else if ( _pVMFile && _pVMFile->count( pFieldName ) )
         {
            readValue = (*_pVMFile)[pFieldName].as<string>() ;
         }
         else
         {
            rc = SDB_FIELD_NOT_EXIST ;
         }
      }
      else
      {
         rc = SDB_SYS ;
      }

      if ( SDB_OK == rc )
      {
         _saveToMapString( pFieldName, pValue, len, readValue,
                           changeLevel, FALSE );
      }

      return rc ;
   }

   INT32 _pmdCfgExchange::readString( const CHAR *pFieldName,
                                      CHAR *pValue, UINT32 len,
                                      const CHAR *pDefault,
                                      PMD_CFG_CHANGE changeLevel )
   {
      INT32 rc = SDB_OK ;

      rc = readString( pFieldName, pValue, len, changeLevel ) ;
      if ( SDB_FIELD_NOT_EXIST == rc && pDefault )
      {
         _saveToMapString( pFieldName, pValue, len, pDefault,
                           changeLevel, TRUE ) ;
         rc = SDB_OK ;
      }

      return rc ;
   }

   INT32 _pmdCfgExchange::writeInt( const CHAR * pFieldName, INT32 value )
   {
      INT32 rc = SDB_OK ;

      if ( PMD_CFG_DATA_BSON == _dataType )
      {
         _dataBuilder.append( pFieldName, value ) ;
      }
      else if ( PMD_CFG_DATA_CMD == _dataType )
      {
         _strStream << pFieldName << "=" << value << OSS_NEWLINE ;
      }
      else
      {
         rc = SDB_SYS ;
      }

      return rc ;
   }

   INT32 _pmdCfgExchange::writeString( const CHAR *pFieldName,
                                       const CHAR *pValue )
   {
      INT32 rc = SDB_OK ;

      if ( PMD_CFG_DATA_BSON == _dataType )
      {
         _dataBuilder.append( pFieldName, pValue ) ;
      }
      else if ( PMD_CFG_DATA_CMD == _dataType )
      {
         _strStream << pFieldName << "=" << pValue << OSS_NEWLINE ;
      }
      else
      {
         rc = SDB_SYS ;
      }

      return rc ;
   }

   BOOLEAN _pmdCfgExchange::hasField( const CHAR * pFieldName )
   {
      if ( PMD_CFG_DATA_BSON == _dataType &&
           !_dataObj.getField( pFieldName ).eoo() )
      {
         return TRUE ;
      }
      else if ( PMD_CFG_DATA_CMD == _dataType )
      {
         if ( ( _pVMCmd && _pVMCmd->count( pFieldName ) ) ||
              ( _pVMFile && _pVMFile->count( pFieldName ) ) )
         {
            return TRUE ;
         }
      }

      return FALSE ;
   }

   const CHAR* _pmdCfgExchange::getData( UINT32 & dataLen, MAP_K2V &mapKeyValue )
   {
      MAP_K2V::iterator it ;
      if ( PMD_CFG_DATA_BSON == _dataType )
      {
         it = mapKeyValue.begin() ;
         while ( it != mapKeyValue.end() )
         {
            if ( FALSE == it->second._hasMapped )
            {
               _dataBuilder.append( it->first, it->second._value ) ;
            }
            ++it ;
         }
         _dataObj = _dataBuilder.obj() ;
         dataLen = _dataObj.objsize() ;
         return _dataObj.objdata() ;
      }
      else if ( PMD_CFG_DATA_CMD == _dataType )
      {
         it = mapKeyValue.begin() ;
         while ( it != mapKeyValue.end() )
         {
            if ( FALSE == it->second._hasMapped )
            {
               _strStream << it->first << "=" << it->second._value
                          << OSS_NEWLINE ;
            }
            ++it ;
         }
         _dataStr = _strStream.str() ;
         dataLen = _dataStr.size() ;
         return _dataStr.c_str() ;
      }
      return NULL ;
   }

   void _pmdCfgExchange::_makeKeyValueMap( po::variables_map *pVM )
   {
      po::variables_map::iterator it = pVM->begin() ;
      MAP_K2V::iterator itKV ;
      MAP_K2V::iterator itKVCold ;
      while ( it != pVM->end() )
      {
         itKV = _pMapKeyField->find( it->first ) ;
         if ( itKV != _pMapKeyField->end() && TRUE == itKV->second._hasMapped )
         {
            ++it ;
            continue ;
         }
         if ( _pMapColdKeyField )
         {
            itKVCold = _pMapColdKeyField->find( it->first ) ;
            if ( itKVCold != _pMapColdKeyField->end() &&
                 TRUE == itKVCold->second._hasMapped )
            {
               continue ;
            }
         }
         try
         {
            (*_pMapKeyField)[ it->first ] = pmdParamValue( it->second.as<string>(),
                                                           FALSE, TRUE ) ;
            ++it ;
            continue ;
         }
         catch( std::exception )
         {
         }

         try
         {
            (*_pMapKeyField)[ it->first ] = pmdParamValue( it->second.as<int>(),
                                                           FALSE, TRUE ) ;
            ++it ;
            continue ;
         }
         catch( std::exception )
         {
         }

         ++it ;
      }
   }

   void _pmdCfgExchange::_saveToMapInt( const CHAR * pFieldName,
                                        INT32 &value, const INT32 &newValue,
                                        PMD_CFG_CHANGE changeLevel,
                                        BOOLEAN useDefault )
   {
      BOOLEAN hasFieldValue = useDefault ? FALSE : TRUE ;

      if ( PMD_CFG_STEP_CHG == _cfgStep &&
           !ISALLOWRUNCHANGE( changeLevel ) )
      {
         if ( NULL != _pMapColdKeyField )
         {
            (*_pMapColdKeyField)[ pFieldName ] = pmdParamValue( newValue, TRUE,
                                                                hasFieldValue,
                                                                changeLevel ) ;
         }
      }
      else
      {
         value = newValue ;
         (*_pMapKeyField)[ pFieldName ] = pmdParamValue( newValue, TRUE,
                                                         hasFieldValue,
                                                         changeLevel ) ;
      }
   }

   void _pmdCfgExchange::_saveToMapString( const CHAR *pFieldName,
                                           CHAR *pValue, UINT32 len,
                                           const string &newValue,
                                           PMD_CFG_CHANGE changeLevel,
                                           BOOLEAN useDefault )
   {
      BOOLEAN hasFieldValue = useDefault ? FALSE : TRUE ;

      if ( PMD_CFG_STEP_CHG == _cfgStep &&
           !ISALLOWRUNCHANGE( changeLevel ) )
      {
         if ( NULL != _pMapColdKeyField )
         {
            (*_pMapColdKeyField)[ pFieldName ] = pmdParamValue( newValue, TRUE,
                                                                hasFieldValue,
                                                                changeLevel ) ;
         }
      }
      else
      {
         ossStrncpy( pValue, newValue.c_str(), len ) ;
         pValue[ len - 1 ] = 0 ;
         (*_pMapKeyField)[ pFieldName ] = pmdParamValue( newValue, TRUE,
                                                         hasFieldValue,
                                                         changeLevel ) ;
      }
   }

   MAP_K2V* _pmdCfgExchange::getKVMap()
   {
      if ( PMD_CFG_DATA_CMD == _dataType )
      {
         if ( _pVMFile )
         {
            _makeKeyValueMap( _pVMFile ) ;
         }
         if ( _pVMCmd )
         {
            _makeKeyValueMap( _pVMCmd ) ;
         }
      }
      else
      {
         MAP_K2V::iterator itKV ;
         MAP_K2V::iterator itKVCold ;
         BSONObjIterator it( _dataObj ) ;
         while ( it.more() )
         {
            BSONElement e = it.next() ;

            itKV = _pMapKeyField->find( e.fieldName() ) ;
            if ( itKV != _pMapKeyField->end() &&
                 TRUE == itKV->second._hasMapped )
            {
               continue ;
            }

            if ( _pMapColdKeyField )
            {
               itKVCold = _pMapColdKeyField->find( e.fieldName() ) ;
               if ( itKVCold != _pMapColdKeyField->end() &&
                    TRUE == itKVCold->second._hasMapped )
               {
                  continue ;
               }
            }

            if ( String == e.type() )
            {
               (*_pMapKeyField)[ e.fieldName() ] = pmdParamValue( e.valuestrsafe(),
                                                                  FALSE, TRUE ) ;
            }
            else if ( e.isNumber() )
            {
               (*_pMapKeyField)[ e.fieldName() ] = pmdParamValue( e.numberInt(),
                                                                  FALSE, TRUE ) ;
            }
         }
      }

      return _pMapKeyField ;
   }

   /*
      _pmdCfgRecord implement
   */
   _pmdCfgRecord::_pmdCfgRecord ()
   {
      _result = SDB_OK ;
      _changeID = 0 ;
      _pConfigHander = NULL ;
      _hasAutoAdjust = FALSE ;
   }
   _pmdCfgRecord::~_pmdCfgRecord ()
   {
   }

   void _pmdCfgRecord::setConfigHandler( IConfigHandle * pConfigHandler )
   {
      _pConfigHander = pConfigHandler ;
   }

   IConfigHandle* _pmdCfgRecord::getConfigHandler() const
   {
      return _pConfigHander ;
   }

   INT32 _pmdCfgRecord::restore( const BSONObj & objData,
                                 po::variables_map *pVMCMD )
   {
      INT32 rc = SDB_OK ;

      pmdCfgExchange ex( &_mapKeyValue, objData, TRUE, PMD_CFG_STEP_INIT ) ;

      ossScopedLock lock( &_mutex ) ;

      /// clear info
      _mapKeyValue.clear() ;

      rc = doDataExchange( &ex ) ;
      if ( rc )
      {
         goto error ;
      }
      /// make kv map
      ex.getKVMap() ;

      if ( pVMCMD )
      {
         pmdCfgExchange ex1( &_mapKeyValue, pVMCMD, NULL, TRUE,
                             PMD_CFG_STEP_REINIT ) ;
         rc = doDataExchange( &ex1 ) ;
         if ( rc )
         {
            goto error ;
         }
         /// make kv map
         ex1.getKVMap() ;
      }
      rc = postLoaded( PMD_CFG_STEP_REINIT ) ;
      if ( rc )
      {
         goto error ;
      }

      if ( getConfigHandler() )
      {
         rc = getConfigHandler()->onConfigInit() ;
         if ( rc )
         {
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdCfgRecord::change( const BSONObj &objData,
                                BOOLEAN isWhole )
   {
      INT32 rc = SDB_OK ;
      BSONObj oldCfg ;
      MAP_K2V mapKeyField ;
      MAP_K2V::iterator it ;
      MAP_K2V::iterator itSelf ;
      BOOLEAN locked = FALSE ;
      pmdCfgExchange ex( &mapKeyField, objData, TRUE, PMD_CFG_STEP_CHG ) ;
      ex.setWhole( isWhole ) ;

      // save old cfg
      rc = toBSON( oldCfg, PMD_CFG_MASK_SKIP_UNFIELD ) ;
      PD_RC_CHECK( rc, PDERROR, "Save old config failed, rc: %d", rc ) ;

      _mutex.get() ;
      locked = TRUE ;

      // update new cfg
      rc = doDataExchange( &ex ) ;
      if ( rc )
      {
         goto restore ;
      }
      rc = postLoaded( PMD_CFG_STEP_CHG ) ;
      if ( rc )
      {
         goto restore ;
      }
      ++_changeID ;

      /// make kv map
      ex.getKVMap() ;
      it = mapKeyField.begin() ;
      while ( it != mapKeyField.end() )
      {
         if ( it->second._hasMapped )
         {
            _mapKeyValue[ it->first ] = it->second ;
            ++it ;
            continue ;
         }

         itSelf = _mapKeyValue.find( it->first ) ;
         if ( itSelf != _mapKeyValue.end() )
         {
            if ( itSelf->second._hasMapped &&
                 itSelf->second._value != it->second._value )
            {
               PD_LOG( PDWARNING, "Field[%s]'s value[%s] can't be modified "
                       "to [%s] in running", it->first.c_str(),
                       itSelf->second._value.c_str(),
                       it->second._value.c_str() ) ;
            }
            else if ( !itSelf->second._hasMapped )
            {
               _mapKeyValue[ it->first ] = it->second ;
            }
         }
         else
         {
            _mapKeyValue[ it->first ] = it->second ;
         }
         ++it ;
      }

      if ( isWhole )
      {
         itSelf = _mapKeyValue.begin() ;
         while( itSelf != _mapKeyValue.end() )
         {
            if ( !itSelf->second._hasMapped &&
                 mapKeyField.find( itSelf->first ) == mapKeyField.end() )
            {
               _mapKeyValue.erase( itSelf++ ) ;
            }
            else
            {
               ++itSelf ;
            }
         }
      }

      // change notify
      if ( getConfigHandler() )
      {
         getConfigHandler()->onConfigChange( getChangeID() ) ;
      }

   done:
      if ( locked )
      {
         _mutex.release() ;
      }
      return rc ;
   error:
      goto done ;
   restore:
      if ( locked )
      {
         _mutex.release() ;
         locked = FALSE ;
      }
      restore( oldCfg, NULL ) ;
      goto done ;
   }

   INT32 _pmdCfgRecord::update( const BSONObj &userConfig,
                                BOOLEAN setForRestore,
                                const controlParams &cp,
                                BSONObj &errorObj )
   {
      INT32 rc = SDB_OK ;
      BSONObj oldCfg ;
      MAP_K2V mapKeyField ;
      MAP_K2V mapColdKeyField ;
      BOOLEAN locked = FALSE ;
      pmdCfgExchange ex( &mapKeyField, &mapColdKeyField,
                         userConfig, TRUE, PMD_CFG_STEP_CHG ) ;

      // save old cfg
      rc = toBSON( oldCfg, 0 ) ;
      PD_RC_CHECK( rc, PDERROR, "Save old config failed, rc: %d", rc ) ;

      _mutex.get() ;
      locked = TRUE ;

      if ( TRUE == setForRestore )
      {
         ex.setWhole( TRUE ) ;
      }

      rc = doDataExchange( &ex ) ;
      if ( rc )
      {
         goto restore ;
      }

      rc = postLoaded( PMD_CFG_STEP_CHG ) ;
      if ( rc )
      {
         goto restore ;
      }

      /// make kv map
      ex.getKVMap() ;

      if ( !_shouldUpdateMKV( mapKeyField, cp.isForce ) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _saveUpdateChange( mapKeyField, mapColdKeyField,
                              setForRestore, errorObj ) ;

      if ( rc )
      {
         goto restore ;
      }
      ++_changeID ;

      if ( TRUE == setForRestore )
      {
         _purgeFieldMap( mapKeyField ) ;
      }

      // change notify
      if ( getConfigHandler() )
      {
         getConfigHandler()->onConfigChange( getChangeID() ) ;
      }

   done:
      if ( locked )
      {
         _mutex.release() ;
      }
      return rc ;
   error:
      goto done ;
   restore:
      if ( locked )
      {
         _mutex.release() ;
         locked = FALSE ;
      }
      restore( oldCfg, NULL ) ;
      goto done ;
   }

   INT32 _pmdCfgRecord::init( po::variables_map *pVMFile,
                              po::variables_map *pVMCMD )
   {
      INT32 rc = SDB_OK ;
      pmdCfgExchange ex( &_mapKeyValue, pVMCMD, pVMFile, TRUE,
                         PMD_CFG_STEP_INIT ) ;

      ossScopedLock lock( &_mutex ) ;

      rc = doDataExchange( &ex ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = postLoaded( PMD_CFG_STEP_INIT ) ;
      if ( rc )
      {
         goto error ;
      }
      ++_changeID ;

      /// make kv map
      ex.getKVMap() ;

      if ( getConfigHandler() )
      {
         rc = getConfigHandler()->onConfigInit() ;
         if ( rc )
         {
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdCfgRecord::toBSON( BSONObj &objData, UINT32 mask )
   {
      INT32 rc = SDB_OK ;

      ossScopedLock lock( &_mutex ) ;

      rc = preSaving() ;
      if ( SDB_OK == rc )
      {
         pmdCfgExchange ex( &_mapKeyValue, BSONObj(), FALSE,
                            PMD_CFG_STEP_INIT, mask ) ;
         rc = doDataExchange( &ex ) ;
         if ( SDB_OK == rc )
         {
            UINT32 dataLen = 0 ;
            try
            {
               objData = BSONObj( ex.getData( dataLen,
                                  _mapKeyValue ) ).getOwned() ;
            }
            catch( std::exception &e )
            {
               PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
               rc = SDB_SYS ;
            }
         }
      }
      return rc ;
   }

   INT32 _pmdCfgRecord::toString( string &str, UINT32 mask )
   {
      INT32 rc = SDB_OK ;

      ossScopedLock lock( &_mutex ) ;

      rc = preSaving() ;
      if ( SDB_OK == rc )
      {
         pmdCfgExchange ex( &_mapKeyValue, NULL, NULL, FALSE,
                            PMD_CFG_STEP_INIT, mask ) ;
         rc = doDataExchange( &ex ) ;
         if ( SDB_OK == rc )
         {
            UINT32 dataLen = 0 ;
            str = ex.getData( dataLen, _mapKeyValue ) ;
         }
      }
      return rc ;
   }

   INT32 _pmdCfgRecord::postLoaded( PMD_CFG_STEP step )
   {
      return SDB_OK ;
   }

   INT32 _pmdCfgRecord::preSaving()
   {
      return SDB_OK ;
   }

   INT32 _pmdCfgRecord::parseAddressLine( const CHAR * pAddressLine,
                                          vector < pmdAddrPair > & vecAddr,
                                          const CHAR * pItemSep,
                                          const CHAR * pInnerSep,
                                          UINT32 maxSize ) const
   {
      INT32 rc = SDB_OK ;
      vector<string> addrs ;
      pmdAddrPair addrItem ;

      if ( !pAddressLine || !pItemSep || !pInnerSep ||
           pItemSep[ 0 ] == 0 || pInnerSep[ 0 ] == 0 )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( pAddressLine[ 0 ] == 0 )
      {
         goto done ;
      }

      boost::algorithm::split( addrs, pAddressLine,
                               boost::algorithm::is_any_of(
                               pItemSep ) ) ;
      if ( maxSize > 0 && maxSize < addrs.size() )
      {
         std::cerr << "addr more than max member size" << endl ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      for ( vector<string>::iterator itr = addrs.begin() ;
            itr != addrs.end() ;
            ++itr )
      {
         vector<string> pair ;
         string tmp = *itr ;
         boost::algorithm::trim( tmp ) ;
         boost::algorithm::split( pair, tmp,
                                  boost::algorithm::is_any_of(
                                  pInnerSep ) ) ;
         if ( pair.size() != 2 )
         {
            std::cerr << "invalid address format" << endl ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if ( pair.at(0).size() == 0 || pair.at(1).size() == 0 )
         {
            std::cerr << "addr pair cannot be empty" << endl ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         UINT32 cpLen = pair.at(0).size() < OSS_MAX_HOSTNAME ?
                        pair.at(0).size() : OSS_MAX_HOSTNAME ;
         ossMemcpy( addrItem._host, pair.at(0).c_str(), cpLen ) ;
         addrItem._host[cpLen] = '\0' ;
         cpLen = pair.at(1).size() < OSS_MAX_SERVICENAME ?
                 pair.at(1).size() : OSS_MAX_SERVICENAME ;
         ossMemcpy( addrItem._service, pair.at(1).c_str(),cpLen ) ;
         addrItem._service[cpLen] = '\0' ;
         vecAddr.push_back( addrItem ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   string _pmdCfgRecord::makeAddressLine( const vector < pmdAddrPair > & vecAddr,
                                          CHAR chItemSep,
                                          CHAR chInnerSep ) const
   {
      UINT32 count = 0 ;
      stringstream ss ;
      for ( UINT32 i = 0; i < vecAddr.size() ; ++i )
      {
         const pmdAddrPair &item = vecAddr[ i ] ;
         if ( '\0' != item._host[ 0 ] )
         {
            if ( 0 != count )
            {
               ss << chItemSep ;
            }
            ss << item._host << chInnerSep << item._service ;
            ++count ;
         }
         else
         {
            break ;
         }
      }
      return ss.str() ;
   }

   INT32 _pmdCfgRecord::_addToFieldMap( const string &key,
                                        const string &value,
                                        BOOLEAN hasMapped,
                                        BOOLEAN hasField )
   {
       _mapKeyValue[ key ] = pmdParamValue( value, hasMapped, hasField ) ;
       return SDB_OK ;
   }

   INT32 _pmdCfgRecord::_addToFieldMap( const string &key,
                                        INT32 value,
                                        BOOLEAN hasMapped,
                                        BOOLEAN hasField )
   {
      _mapKeyValue[ key ] = pmdParamValue( value, hasMapped, hasField ) ;
      return SDB_OK ;
   }

   void _pmdCfgRecord::_updateFieldMap( pmdCfgExchange *pEX,
                                        const CHAR *pFieldName,
                                        const CHAR *pValue,
                                        PMD_CFG_CHANGE changeLevel )
   {
      MAP_K2V::iterator it ;

      if ( !pEX->isWhole() && !pEX->hasField( pFieldName ) )
      {
         goto done ;
      }

      it = _mapKeyValue.find( pFieldName ) ;
      if ( it != _mapKeyValue.end() )
      {
         it->second._value = pValue ;
      }

done:
      return ;
   }

   void _pmdCfgRecord::_purgeFieldMap( MAP_K2V &mapKeyField )
   {
      MAP_K2V::iterator itSelf ;
      itSelf = _mapKeyValue.begin() ;
      while( itSelf != _mapKeyValue.end() )
      {
         if ( !itSelf->second._hasMapped &&
              mapKeyField.find( itSelf->first ) == mapKeyField.end() )
         {
            _mapKeyValue.erase( itSelf++ ) ;
         }
         else
         {
            ++itSelf ;
         }
      }
   }

   BOOLEAN _pmdCfgRecord::_shouldUpdateMKV( MAP_K2V &mapKeyField,
                                            BOOLEAN isForce ) const
   {
      if ( !isForce )
      {
         MAP_K2V::iterator iter ;
         for ( iter = mapKeyField.begin(); iter != mapKeyField.end(); ++iter )
         {
            if ( !iter->second._hasMapped )
            {
               #ifdef SDB_ENGINE
               PD_LOG_MSG(
                   PDERROR,
                   "Error: config[%s] is not an official configuration, "
                   "check its spelling or add Force:true to options.",
                   iter->first.c_str() ) ;
               #else
               PD_LOG(
                   PDERROR,
                   "Error: config[%s] is not an official configuration, "
                   "check its spelling or add Force:true to options.",
                   iter->first.c_str() ) ;
               #endif
               return FALSE ;
            }
         }
      }
      return TRUE ;
   }

   INT32 _pmdCfgRecord::_saveUpdateChange( MAP_K2V &mapKeyField,
                                           MAP_K2V &mapColdKeyField,
                                           BOOLEAN setForRestore,
                                           BSONObj &errorObj )
   {
      INT32 rc = SDB_OK ;
      MAP_K2V::iterator iter ;
      MAP_K2V::iterator iterCold ;
      MAP_K2V::iterator itSelf ;
      MAP_K2V::iterator iterColdSelf ;
      BSONObjBuilder errorObjBuilder ;
      BSONArrayBuilder rebootArrBuilder ;
      BSONArrayBuilder forbidArrBuilder ;

      iter = mapKeyField.begin() ;
      while ( iter != mapKeyField.end() )
      {
         if ( TRUE == setForRestore &&
              TRUE == iter->second._hasField )
         {
            ++iter ;
            continue ;
         }

         _mapKeyValue[ iter->first ] = iter->second ;
         ++iter ;
      }

      try
      {
         iterCold = mapColdKeyField.begin() ;
         while ( iterCold != mapColdKeyField.end() )
         {
            if ( TRUE == setForRestore &&
                 TRUE == iterCold->second._hasField )
            {
               ++iterCold ;
               continue ;
            }

            if ( PMD_CFG_CHANGE_REBOOT == iterCold->second._level )
            {
               _mapColdKeyValue[ iterCold->first ] = iterCold->second ;
            }

            itSelf = _mapKeyValue.find( iterCold->first ) ;
            if ( itSelf != _mapKeyValue.end() &&
                 itSelf->second._value != iterCold->second._value )
            {
               if ( PMD_CFG_CHANGE_REBOOT == iterCold->second._level )
               {
                  rebootArrBuilder.append( iterCold->first ) ;
               }
               else if ( PMD_CFG_CHANGE_FORBIDDEN == iterCold->second._level )
               {
                  forbidArrBuilder.append( iterCold->first ) ;
               }
            }
            ++iterCold ;
         }

         errorObjBuilder.append( "Reboot", rebootArrBuilder.arr() ) ;
         errorObjBuilder.append( "Forbidden", forbidArrBuilder.arr() ) ;
         errorObj = errorObjBuilder.obj() ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDWARNING, "Exception during config update: %s",
                 e.what() ) ;
         rc = SDB_SYS ;
      }
      return rc ;
   }

   BOOLEAN _pmdCfgRecord::hasField( const CHAR * pFieldName )
   {
      if ( _mapKeyValue.find( pFieldName ) == _mapKeyValue.end() )
      {
         return FALSE ;
      }
      return TRUE ;
   }

   INT32 _pmdCfgRecord::getFieldInt( const CHAR * pFieldName,
                                     INT32 &value,
                                     INT32 *pDefault )
   {
      INT32 rc = SDB_OK ;

      if ( !hasField( pFieldName ) )
      {
         if ( pDefault )
         {
            value = *pDefault ;
         }
         else
         {
            rc = SDB_FIELD_NOT_EXIST ;
         }
      }
      else
      {
         value = ossAtoi( _mapKeyValue[ pFieldName ]._value.c_str() ) ;
      }

      return rc ;
   }

   INT32 _pmdCfgRecord::getFieldStr( const CHAR * pFieldName,
                                     CHAR *pValue, UINT32 len,
                                     const CHAR * pDefault )
   {
      INT32 rc = SDB_OK ;
      string tmpValue ;

      if ( !hasField( pFieldName ) )
      {
         if ( pDefault )
         {
            tmpValue.assign( pDefault ) ;
         }
         else
         {
            rc = SDB_FIELD_NOT_EXIST ;
         }
      }
      else
      {
         tmpValue = _mapKeyValue[ pFieldName ]._value ;
      }

      if ( SDB_OK == rc )
      {
         ossStrncpy( pValue, tmpValue.c_str(), len ) ;
         pValue[ len - 1 ] = 0 ;
      }

      return rc ;
   }

   INT32 _pmdCfgRecord::getFieldStr( const CHAR *pFieldName,
                                     std::string &strValue,
                                     const CHAR *pDefault )
   {
      INT32 rc = SDB_OK ;

      if ( !hasField( pFieldName ) )
      {
         if ( pDefault )
         {
            strValue.assign( pDefault ) ;
         }
         else
         {
            rc = SDB_FIELD_NOT_EXIST ;
         }
      }
      else
      {
         strValue = _mapKeyValue[ pFieldName ]._value ;
      }
      return rc ;
   }

   INT32 _pmdCfgRecord::rdxString( pmdCfgExchange *pEX, const CHAR *pFieldName,
                                   CHAR *pValue, UINT32 len, BOOLEAN required,
                                   PMD_CFG_CHANGE changeLevel,
                                   const CHAR *pDefaultValue,
                                   BOOLEAN hideParam )
   {
      if ( _result )
      {
         goto error ;
      }
      _curFieldName = pFieldName ;

      if ( pEX->isLoad() )
      {
         if ( PMD_CFG_STEP_REINIT == pEX->getCfgStep() &&
              !pEX->hasField( pFieldName ) )
         {
            goto done ;
         }
         else if ( PMD_CFG_STEP_CHG == pEX->getCfgStep() )
         {
            if ( !pEX->isWhole() && !pEX->hasField( pFieldName ) )
            {
               goto done ;
            }
         }

         if ( required )
         {
            _result = pEX->readString( pFieldName, pValue, len, changeLevel ) ;
         }
         else
         {
            _result = pEX->readString( pFieldName, pValue, len,
                                       pDefaultValue, changeLevel ) ;
         }
         if ( SDB_FIELD_NOT_EXIST == _result )
         {
            ossPrintf( "Error: Field[%s] not config\n", pFieldName ) ;
         }
         else if ( _result )
         {
            ossPrintf( "Error: Read field[%s] config failed, rc: %d\n",
                       pFieldName, _result ) ;
         }
      }
      else
      {
         MAP_K2V::iterator it ;
         MAP_K2V::iterator itCold ;

         /// 1. when is default
         if ( 0 == ossStrcmp( pValue, pDefaultValue ) )
         {
            if ( hideParam && pEX->isSkipHideDefault() )
            {
               goto done ;
            }
            else if ( !hideParam && pEX->isSkipNormalDefault() )
            {
               goto done ;
            }
         }

         /// 2. skip unfield
         if ( pEX->isSkipUnField() )
         {
            it = _mapKeyValue.find( pFieldName ) ;
            itCold = _mapColdKeyValue.end() ;

            if ( PMD_CFG_CHANGE_REBOOT == changeLevel )
            {
               itCold = _mapColdKeyValue.find( pFieldName ) ;
               if ( _mapColdKeyValue.end() != itCold && pEX->isLocalMode() )
               {
                  it = _mapKeyValue.end() ;
               }
            }

            if ( ( it == _mapKeyValue.end() || !it->second._hasField ) &&
               ( itCold == _mapColdKeyValue.end() || !itCold->second._hasField ) )
            {
               goto done ;
            }
         }

         if ( PMD_CFG_CHANGE_REBOOT != changeLevel ||
              !pEX->isLocalMode() ||
              ( _mapKeyValue.end() != ( it = _mapKeyValue.find( pFieldName ) ) &&
                0 != ossStrcmp( pValue, it->second._value.c_str() ) ) ||
              ( _mapColdKeyValue.end() == ( itCold = _mapColdKeyValue.find( pFieldName ) ) ) )
         {
             _result = pEX->writeString( pFieldName, pValue ) ;
         }
         else
         {
             _result = pEX->writeString( pFieldName,
                                         itCold->second._value.c_str() ) ;
         }
         if ( _result )
         {
            PD_LOG( PDWARNING, "Write field[%s] failed, rc: %d",
                    pFieldName, _result ) ;
         }
      }

   done:
      return _result ;
   error:
      goto done ;
   }

   INT32 _pmdCfgRecord::_rdxPath( pmdCfgExchange *pEX, const CHAR *pFieldName,
                                  CHAR *pValue, UINT32 len, BOOLEAN required,
                                  PMD_CFG_CHANGE changeLevel,
                                  const CHAR *pDefaultValue,
                                  BOOLEAN hideParam,
                                  BOOLEAN addSep )
   {
      _result = rdxString( pEX, pFieldName, pValue, len, required, changeLevel,
                           pDefaultValue, hideParam ) ;
      if ( SDB_OK == _result && pEX->isLoad() && 0 != pValue[0] )
      {
         std::string strTmp = pValue ;
         if ( NULL == ossGetRealPath( strTmp.c_str(), pValue, len ) )
         {
            ossPrintf( "Error: Failed to get real path for %s:%s\n",
                       pFieldName, strTmp.c_str() ) ;
            _result = SDB_INVALIDARG ;
         }
         else
         {
            pValue[ len - 1 ] = 0 ;

            if ( addSep )
            {
               utilCatPath( pValue, len, "" ) ;
            }
         }

         /// update map's value
         _updateFieldMap( pEX, pFieldName, pValue, changeLevel ) ;

      }
      return _result ;
   }

   INT32 _pmdCfgRecord::rdxPath( pmdCfgExchange *pEX, const CHAR *pFieldName,
                                 CHAR *pValue, UINT32 len, BOOLEAN required,
                                 PMD_CFG_CHANGE changeLevel,
                                 const CHAR *pDefaultValue,
                                 BOOLEAN hideParam )
   {
      return _rdxPath( pEX, pFieldName, pValue, len,
                       required, changeLevel, pDefaultValue,
                       hideParam, TRUE ) ;
   }

   INT32 _pmdCfgRecord::rdxPathRaw( pmdCfgExchange *pEX, const CHAR *pFieldName,
                                    CHAR *pValue, UINT32 len, BOOLEAN required,
                                    PMD_CFG_CHANGE changeLevel,
                                    const CHAR *pDefaultValue,
                                    BOOLEAN hideParam )
   {
      return _rdxPath( pEX, pFieldName, pValue, len,
                       required, changeLevel, pDefaultValue,
                       hideParam, FALSE ) ;
   }

   INT32 _pmdCfgRecord::rdxBooleanS( pmdCfgExchange *pEX,
                                     const CHAR *pFieldName,
                                     BOOLEAN &value,
                                     BOOLEAN required,
                                     PMD_CFG_CHANGE changeLevel,
                                     BOOLEAN defaultValue,
                                     BOOLEAN hideParam )
   {
      CHAR szTmp[ PMD_MAX_ENUM_STR_LEN + 1 ] = {0} ;
      string boolValue ;
      ossStrcpy( szTmp, value ? "TRUE" : "FALSE" ) ;
      _result = rdxString( pEX, pFieldName, szTmp, sizeof(szTmp), required,
                           changeLevel, defaultValue ? "TRUE" : "FALSE",
                           hideParam ) ;
      if ( SDB_OK == _result && pEX->isLoad() )
      {
         if ( SDB_INVALIDARG == ossStrToBoolean( szTmp, &value ) )
         {
            value = defaultValue ;
         }

         // update map's value
         boolValue = value ? "TRUE" : "FALSE" ;
         _updateFieldMap( pEX, pFieldName, boolValue.c_str(), changeLevel ) ;

      }
      return _result ;
   }

   INT32 _pmdCfgRecord::rdxInt( pmdCfgExchange *pEX, const CHAR *pFieldName,
                                INT32 &value, BOOLEAN required,
                                PMD_CFG_CHANGE changeLevel, INT32 defaultValue,
                                BOOLEAN hideParam )
   {
      if ( _result )
      {
         goto error ;
      }
      _curFieldName = pFieldName ;

      if ( pEX->isLoad() )
      {
         if ( PMD_CFG_STEP_REINIT == pEX->getCfgStep() &&
              !pEX->hasField( pFieldName ) )
         {
            goto done ;
         }
         else if ( PMD_CFG_STEP_CHG == pEX->getCfgStep() )
         {
            if ( !pEX->isWhole() && !pEX->hasField( pFieldName ) )
            {
               goto done ;
            }
         }

         if ( required )
         {
            _result = pEX->readInt( pFieldName, value, changeLevel ) ;
         }
         else
         {
            _result = pEX->readInt( pFieldName, value, defaultValue,
                                    changeLevel ) ;
         }
         if ( SDB_FIELD_NOT_EXIST == _result )
         {
            ossPrintf( "Error: Field[%s] not config\n", pFieldName ) ;
         }
         else if ( _result )
         {
            ossPrintf( "Error: Read field[%s] config failed, rc: %d\n",
                       pFieldName, _result ) ;
         }
      }
      else
      {
         MAP_K2V::iterator it ;
         MAP_K2V::iterator itCold ;

         /// 1. when is default
         if ( value == defaultValue )
         {
            if ( hideParam && pEX->isSkipHideDefault() )
            {
               goto done ;
            }
            else if ( !hideParam && pEX->isSkipNormalDefault() )
            {
               goto done ;
            }
         }

         /// 2. skip unfield
         if ( pEX->isSkipUnField() )
         {
            it = _mapKeyValue.find( pFieldName ) ;
            itCold = _mapColdKeyValue.end() ;

            if ( PMD_CFG_CHANGE_REBOOT == changeLevel )
            {
               itCold = _mapColdKeyValue.find( pFieldName ) ;
               if ( _mapColdKeyValue.end() != itCold && pEX->isLocalMode() )
               {
                  it = _mapKeyValue.end() ;
               }
            }

            if ( ( it == _mapKeyValue.end() || !it->second._hasField ) &&
                 ( itCold == _mapColdKeyValue.end() || !itCold->second._hasField ) )
            {
               goto done ;
            }
         }

         if ( PMD_CFG_CHANGE_REBOOT != changeLevel ||
              !pEX->isLocalMode() ||
              ( _mapKeyValue.end() != ( it = _mapKeyValue.find( pFieldName ) ) &&
                value != ossAtoi( it->second._value.c_str() ) ) ||
              ( _mapColdKeyValue.end() == ( itCold = _mapColdKeyValue.find( pFieldName ) ) ) )
         {
             _result = pEX->writeInt( pFieldName, value ) ;
         }
         else
         {
             _result = pEX->writeInt( pFieldName,
                                      ossAtoi( itCold->second._value.c_str() ) ) ;
         }
         if ( _result )
         {
            PD_LOG( PDWARNING, "Write field[%s] failed, rc: %d",
                    pFieldName, _result ) ;
         }
      }

   done:
      return _result ;
   error:
      goto done ;
   }

   INT32 _pmdCfgRecord::rdxUInt( pmdCfgExchange *pEX, const CHAR *pFieldName,
                                 UINT32 &value, BOOLEAN required,
                                 PMD_CFG_CHANGE changeLevel, UINT32 defaultValue,
                                 BOOLEAN hideParam )
   {
      INT32 tmpValue = (INT32)value ;
      _result = rdxInt( pEX, pFieldName, tmpValue, required, changeLevel,
                        (INT32)defaultValue, hideParam ) ;
      if ( SDB_OK == _result && pEX->isLoad() )
      {
         if ( tmpValue < 0 )
         {
            _result = SDB_INVALIDARG ;
            ossPrintf( "Waring: Field[%s] value[%d] is less than min value 0\n",
                       pFieldName, tmpValue ) ;
            goto error ;
         }
         value = (UINT32)tmpValue ;
      }
   done:
      return _result ;
   error:
      goto done ;
   }

   INT32 _pmdCfgRecord::rdxShort( pmdCfgExchange *pEX, const CHAR *pFieldName,
                                  INT16 &value, BOOLEAN required,
                                  PMD_CFG_CHANGE changeLevel, INT16 defaultValue,
                                  BOOLEAN hideParam )
   {
      INT32 tmpValue = (INT32)value ;
      _result = rdxInt( pEX, pFieldName, tmpValue, required, changeLevel,
                        (INT32)defaultValue, hideParam ) ;
      if ( SDB_OK == _result && pEX->isLoad() )
      {
         value = (INT16)tmpValue ;
      }
      return _result ;
   }

   INT32 _pmdCfgRecord::rdxUShort( pmdCfgExchange *pEX, const CHAR *pFieldName,
                                   UINT16 &value, BOOLEAN required,
                                   PMD_CFG_CHANGE changeLevel, UINT16 defaultValue,
                                   BOOLEAN hideParam )
   {
      INT32 tmpValue = (INT32)value ;
      _result = rdxInt( pEX, pFieldName, tmpValue, required, changeLevel,
                        (INT32)defaultValue, hideParam ) ;
      if ( SDB_OK == _result && pEX->isLoad() )
      {
         if ( tmpValue < 0 )
         {
            _result = SDB_INVALIDARG ;
            ossPrintf( "Waring: Field[%s] value[%d] is less than min value 0\n",
                       pFieldName, tmpValue ) ;
            goto error ;
         }
         value = (UINT16)tmpValue ;
      }
   done:
      return _result ;
   error:
      goto done ;
   }

   INT32 _pmdCfgRecord::rdvMinMax( pmdCfgExchange *pEX, UINT32 &value,
                                   UINT32 minV, UINT32 maxV,
                                   BOOLEAN autoAdjust )
   {
      if ( _result )
      {
         goto error ;
      }

      if ( !pEX->isLoad() )
      {
         goto done ;
      }

      if ( value < minV )
      {
         ossPrintf( "Waring: Field[%s] value[%u] is less than min value[%u]\n",
                    _curFieldName.c_str(), value, minV ) ;
         if ( autoAdjust )
         {
            value = minV ;
            _hasAutoAdjust = TRUE ;
         }
         else
         {
            _result = SDB_INVALIDARG ;
            goto error ;
         }
      }
      else if ( value > maxV )
      {
         ossPrintf( "Waring: Field[%s] value[%u] is more than max value[%u]\n",
                 _curFieldName.c_str(), value, maxV ) ;
         if ( autoAdjust )
         {
            value = maxV ;
            _hasAutoAdjust = TRUE ;
         }
         else
         {
            _result = SDB_INVALIDARG ;
            goto error ;
         }
      }

   done:
      return _result ;
   error:
      goto done ;
   }

   INT32 _pmdCfgRecord::rdvMinMax( pmdCfgExchange *pEX, INT32 &value,
                                   INT32 minV, INT32 maxV,
                                   BOOLEAN autoAdjust )
   {
      if ( _result )
      {
         goto error ;
      }

      if ( !pEX->isLoad() )
      {
         goto done ;
      }

      if ( value < minV )
      {
         ossPrintf( "Waring: Field[%s] value[%d] is less than min value[%d]\n",
                    _curFieldName.c_str(), value, minV ) ;
         if ( autoAdjust )
         {
            value = minV ;
            _hasAutoAdjust = TRUE ;
         }
         else
         {
            _result = SDB_INVALIDARG ;
            goto error ;
         }
      }
      else if ( value > maxV )
      {
         ossPrintf( "Waring: Field[%s] value[%d] is more than max value[%d]\n",
                 _curFieldName.c_str(), value, maxV ) ;
         if ( autoAdjust )
         {
            value = maxV ;
            _hasAutoAdjust = TRUE ;
         }
         else
         {
            _result = SDB_INVALIDARG ;
            goto error ;
         }
      }

   done:
      return _result ;
   error:
      goto done ;
   }

   INT32 _pmdCfgRecord::rdvMinMax( pmdCfgExchange *pEX, UINT16 &value,
                                   UINT16 minV, UINT16 maxV,
                                   BOOLEAN autoAdjust )
   {
      if ( _result )
      {
         goto error ;
      }

      if ( !pEX->isLoad() )
      {
         goto done ;
      }

      if ( value < minV )
      {
         ossPrintf( "Waring: Field[%s] value[%u] is less than min value[%u]\n",
                    _curFieldName.c_str(), value, minV ) ;
         if ( autoAdjust )
         {
            value = minV ;
            _hasAutoAdjust = TRUE ;
         }
         else
         {
            _result = SDB_INVALIDARG ;
            goto error ;
         }
      }
      else if ( value > maxV )
      {
         ossPrintf( "Waring: Field[%s] value[%u] is more than max value[%u]\n",
                    _curFieldName.c_str(), value, maxV ) ;
         if ( autoAdjust )
         {
            value = maxV ;
            _hasAutoAdjust = TRUE ;
         }
         else
         {
            _result = SDB_INVALIDARG ;
            goto error ;
         }
      }

   done:
      return _result ;
   error:
      goto done ;
   }

   INT32 _pmdCfgRecord::rdvMaxChar( pmdCfgExchange *pEX, CHAR *pValue,
                                    UINT32 maxChar, BOOLEAN autoAdjust )
   {
      UINT32 len = 0 ;

      if ( _result )
      {
         goto error ;
      }
      if ( !pEX->isLoad() )
      {
         goto done ;
      }

      len = ossStrlen( pValue ) ;
      if ( len > maxChar )
      {
         ossPrintf( "Waring: Field[%s] value[%s] length more than [%u]\n",
                    _curFieldName.c_str(), pValue, maxChar ) ;
         if ( autoAdjust )
         {
            pValue[ maxChar ] = 0 ;
            _hasAutoAdjust = TRUE ;
         }
         else
         {
            _result = SDB_INVALIDARG ;
            goto error ;
         }
      }

   done:
      return _result ;
   error:
      goto done ;
   }

   INT32 _pmdCfgRecord::rdvNotEmpty( pmdCfgExchange * pEX, CHAR *pValue )
   {
      if ( _result )
      {
         goto error ;
      }
      if ( !pEX->isLoad() )
      {
         goto done ;
      }

      if ( ossStrlen( pValue ) == 0 )
      {
         ossPrintf( "Waring: Field[%s] is empty\n", _curFieldName.c_str() ) ;
         _result = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return _result ;
   error:
      goto done ;
   }

   /*
      _pmdOptionsMgr implement
   */
   _pmdOptionsMgr::_pmdOptionsMgr()
   {
      // rdx members
      ossMemset( _krcbDbPath, 0, OSS_MAX_PATHSIZE + 1 ) ;
      ossMemset( _krcbIndexPath, 0, OSS_MAX_PATHSIZE + 1 ) ;
      ossMemset( _krcbDiagLogPath, 0, OSS_MAX_PATHSIZE + 1 ) ;
      ossMemset( _krcbAuditLogPath, 0, OSS_MAX_PATHSIZE + 1 ) ;
      ossMemset( _krcbLogPath, 0, OSS_MAX_PATHSIZE + 1 ) ;
      ossMemset( _krcbBkupPath, 0, OSS_MAX_PATHSIZE + 1 ) ;
      ossMemset( _krcbWWWPath, 0, OSS_MAX_PATHSIZE + 1 ) ;
      ossMemset( _krcbSvcName, 0, OSS_MAX_SERVICENAME + 1) ;
      ossMemset( _replServiceName, 0, OSS_MAX_SERVICENAME + 1) ;
      ossMemset( _catServiceName, 0, OSS_MAX_SERVICENAME + 1) ;
      ossMemset( _shardServiceName, 0, OSS_MAX_SERVICENAME + 1) ;
      ossMemset( _restServiceName, 0, OSS_MAX_SERVICENAME + 1) ;
      ossMemset( _omServiceName, 0, OSS_MAX_SERVICENAME + 1 ) ;
      ossMemset( _krcbRole, 0, PMD_MAX_ENUM_STR_LEN + 1) ;
      ossMemset( _syncStrategyStr, 0, PMD_MAX_ENUM_STR_LEN + 1) ;
      ossMemset( _prefInstStr, 0, PMD_MAX_LONG_STR_LEN + 1 ) ;
      ossMemset( _prefInstModeStr, 0, PMD_MAX_SHORT_STR_LEN + 1 ) ;
      ossMemset( _prefConstraint, 0, sizeof( _prefConstraint ) ) ;
      ossMemset( _catAddrLine, 0, OSS_MAX_PATHSIZE + 1 ) ;
      ossMemset( _dmsTmpBlkPath, 0, OSS_MAX_PATHSIZE + 1 ) ;
      ossMemset( _krcbLobPath, 0, OSS_MAX_PATHSIZE + 1 ) ;
      ossMemset( _krcbLobMetaPath, 0, OSS_MAX_PATHSIZE + 1 ) ;
      ossMemset( _auditMaskStr, 0, sizeof( _auditMaskStr ) ) ;
      ossMemset( _ftMaskStr, 0, sizeof( _ftMaskStr ) ) ;
      ossMemset( _memDebugMaskStr, 0, sizeof( _memDebugMaskStr ) ) ;
      ossMemset( _serviceMaskStr, 0, sizeof( _serviceMaskStr ) ) ;

      _krcbMaxPool         = 0 ;
      _krcbDiagLvl         = (UINT16)PDWARNING ;
      _logFileSz           = 0 ;
      _logFileNum          = 0 ;
      _numPreLoaders       = 0 ;
      _maxPrefPool         = PMD_MAX_PREF_POOL ;
      _maxSubQuery         = PMD_MAX_SUB_QUERY ;
      _maxReplSync         = PMD_DEFAULT_MAX_REPLSYNC ;
      _syncStrategy        = CLS_SYNC_NONE ;
      _dataErrorOp         = PMD_OPT_VALUE_FULLSYNC ;
      _replBucketSize      = PMD_DFT_REPL_BUCKET_SIZE ;
      _memDebugEnabled     = FALSE ;
      _memDebugDetail      = FALSE ;
      _memDebugVerify      = FALSE ;
      _memDebugMask        = 0 ;
      _memDebugSize        = 0 ;
      _indexScanStep       = PMD_DFT_INDEX_SCAN_STEP ;
      _dpslocal            = FALSE ;
      _traceOn             = FALSE ;
      _traceBufSz          = TRACE_DFT_BUFFER_SIZE ;
      _transactionOn       = TRUE ;
      _transIsolation      = DPS_TRANS_ISOLATION_DFT ;
      _transLockwait       = DPS_TRANS_LOCKWAIT_DFT ;
      _transAutoCommit     = DPS_TRANS_AUTOCOMMIT_DFT ;
      _transAutoRollback   = DPS_TRANS_AUTOROLLBACK_DFT ;
      _transUseRBS         = DPS_TRANS_USE_RBS_DFT ;
      _transTimeout        = DPS_TRANS_DFT_TIMEOUT ;
      _sharingBreakTime    = PMD_OPTION_BRK_TIME_DEFAULT ;
      _startShiftTime      = PMD_DFT_START_SHIFT_TIME ;
      _logBuffSize         = DPS_DFT_LOG_BUF_SZ ;
      _sortBufSz           = PMD_DEFAULT_SORTBUF_SZ ;
      _hjBufSz             = PMD_DEFAULT_HJ_SZ ;
      _dialogFileNum       = 0 ;
      _auditFileNum        = 0 ;
      _auditMask           = 0 ;
      _ftMask              = PMD_FT_MASK_DFT ;
      _ftConfirmPeriod     = PMD_FT_CACL_INTERVAL_DFT ;
      _ftConfirmRatio      = PMD_FT_CACL_RATIO_DFT ;
      _ftLevel             = FT_LEVEL_SEMI ;
      _ftFusingTimeout     = PMD_DFT_FTFUSING_TIMEOUT ;
      _ftSlowNodeThreshold = PMD_DFT_FTSLOWNODE_THRESHOLD ;
      _ftSlowNodeIncrement = PMD_DFT_FTSLOWNODE_INCREMENT ;
      _syncwaitTimeout     = PMD_DFT_SYNCWAIT_TIMEOUT ;
      _shutdownWaitTimeout = PMD_DFT_SHUTDOWN_WAIT_TIMEOUT ;
      _directIOInLob       = FALSE ;
      _sparseFile          = TRUE ;
      _weight              = 0 ;
      _auth                = TRUE ;
      _planBucketNum       = OPT_PLAN_DEF_CACHE_BUCKETS ;
      _oprtimeout          = PMD_OPTION_OPR_TIME_DEFAULT ;
      _overflowRatio       = PMD_DFT_OVERFLOW_RETIO ;
      _extendThreshold     = PMD_DFT_EXTEND_THRESHOLD ;
      _signalInterval      = 0 ;
      _maxCacheSize        = 0 ;
      _maxCacheJob         = 0 ;
      _maxSyncJob          = PMD_DFT_MAX_SYNC_JOB ;
      _syncInterval        = PMD_DFT_SYNC_INTERVAL ;
      _syncRecordNum       = PMD_DFT_SYNC_RECORDNUM ;
      _syncDeep            = FALSE ;
      _serviceMask         = PMD_SVC_MASK_NONE ;
      _maxContextNum       = RTN_MAX_CTX_NUM_DFT ;
      _maxSessionContextNum = RTN_MAX_SESS_CTX_NUM_DFT ;
      _contextTimeout      = RTN_CTX_TIMEOUT_DFT ;

      // archive related
      _archiveOn = FALSE ;
      _archiveCompressOn = TRUE ;
      ossMemset( _archivePath, 0, OSS_MAX_PATHSIZE + 1 ) ;
      _archiveTimeout = PMD_DFT_ARCHIVE_TIMEOUT ;
      _archiveExpired = PMD_DFT_ARCHIVE_EXPIRED ;
      _archiveQuota = PMD_DFT_ARCHIVE_QUOTA ;

      _dmsChkInterval = PMD_DFT_DMS_CHK_INTERVAL ;
      _cacheMergeSize = PMD_DFT_CACHE_MERGE_SZ ;
      _pageAllocTimeout = PMD_DFT_PAGE_ALLOC_TIMEOUT ;
      _perfStat = FALSE ;
      _optCostThreshold = PMD_DFT_OPT_COST_THRESHOLD ;
      _enableMixCmp = PMD_DFT_ENABLE_MIX_CMP ;
      _planCacheLevel = OPT_PLAN_NOCACHE ;
      _instanceID = PMD_DFT_INSTANCE_ID ;
      _maxconn = PMD_DFT_MAX_CONN ;

      _svcSchedulerType = 0 ;
      _svcMaxConcurrency= 0 ;

      _preferredStrict = FALSE ;
      _preferredPeriod = PMD_DFT_PREFINST_PERIOD ;

      ossMemset( _logWriteModStr, 0, sizeof(_logWriteModStr) ) ;
      _logWriteMod = DPS_LOG_WRITE_MOD_INCREMENT ;

      _logTimeOn = FALSE ;

      _enableSleep = FALSE ;
      _recycleRecord = FALSE ;

      _indexCoverOn = TRUE ;

      _maxSockPerNode = 1 ;
      _maxSockPerThread = 0 ;
      _maxSockThread = 1 ;

      _maxTCSize = 0 ;
      _memPoolSize = 0 ;
      _memPoolThreshold = 0 ;

      _transReplSize = -1 ;
      _transRCCount = DPS_TRANS_RCCOUNT_DFT ;
      _transAllowLockEscalation = DPS_TRANS_ALLOWLOCKESCALATION_DFT ;
      _transMaxLockNum = DPS_TRANS_MAXLOCKNUM_DFT ;
      _transMaxLogSpaceRatio = DPS_TRANS_MAXLOGSPACERATIO_DFT ;
      _transConsistencyStrategy = SDB_CONSISTENCY_PRY_LOC_MAJOR ;

      _detectDisk = TRUE ;
      _diagSecureOn = TRUE ;
      _metacacheexpired = PMD_DFT_METACACHE_EXPIRED ;
      _metacachelwm = PMD_DFT_METACACHE_LWM ;

      _statMCVLimit = PMD_DFT_STAT_MCV_LIMIT ;

      _remoteLocationConsistency = TRUE ;
      _consultRollbackLogOn = TRUE ;
      _privilegeCheckEnabled = FALSE ;
      _userCacheInterval = PMD_DFT_USER_CACHE_INTERVAL ;

      _memMXFast = PMD_DFT_MEM_MXFAST ;
      _memTrimThreshold = PMD_DFT_MEM_TRIM_THRESHOLD ;
      _memMmapThreshold = PMD_DFT_MEM_MMAP_THRESHOLD ;
      _memMmapMax = PMD_DFT_MEM_MMAP_MAX ;
      _memTopPad = PMD_DFT_MEM_TOP_PAD ;

      ossMemset( _storageEngineName, 0, sizeof( _storageEngineName ) ) ;
      _storageEngineType = DMS_STORAGE_ENGINE_UNKNOWN ;

      _wtCacheSize = DMS_DFT_WT_CACHE_SIZE ;
      _wtEvictTarget = DMS_DFT_WT_EVICT_TARGET ;
      _wtEvictTrigger = DMS_DFT_WT_EVICT_TRIGGER ;
      _wtEvictDirtyTarget = DMS_DFT_WT_EVICT_DIRTY_TARGET ;
      _wtEvictDirtyTrigger = DMS_DFT_WT_EVICT_DIRTY_TRIGGER ;
      _wtEvictUpdatesTarget = DMS_DFT_WT_EVICT_UPDATES_TARGET ;
      _wtEvictUpdatesTrigger = DMS_DFT_WT_EVICT_UPDATES_TRIGGER ;
      _wtEvictThreadsMin = DMS_DFT_WT_EVICT_THREADS_MIN ;
      _wtEvictThreadsMax = DMS_DFT_WT_EVICT_THREADS_MAX ;
      _wtCheckPointInterval = DMS_DFT_WT_CHECK_POINT_INTERVAL ;

#ifdef SDB_ENTERPRISE

#ifdef SDB_SSL
      _useSSL              = FALSE ;
#endif

#endif /* SDB_ENTERPRISE */
      // other configs
      ossMemset( _krcbConfPath, 0, sizeof( _krcbConfPath ) ) ;
      ossMemset( _krcbConfFile, 0, sizeof( _krcbConfFile ) ) ;
      ossMemset( _krcbCatFile, 0, sizeof( _krcbCatFile ) ) ;
      _krcbSvcPort    = OSS_DFT_SVCPORT ;
      _invalidConfNum = 0 ;
   }

   _pmdOptionsMgr::~_pmdOptionsMgr()
   {
   }

   INT32 _pmdOptionsMgr::doDataExchange( pmdCfgExchange * pEX )
   {
      resetResult () ;

      // map configs begin
      // --confpath
      rdxPath( pEX, PMD_OPTION_CONFPATH , _krcbConfPath, sizeof(_krcbConfPath),
               FALSE, PMD_CFG_CHANGE_FORBIDDEN, PMD_CURRENT_PATH ) ;
      // --dbpath
      rdxPath( pEX, PMD_OPTION_DBPATH, _krcbDbPath, sizeof(_krcbDbPath),
               FALSE, PMD_CFG_CHANGE_FORBIDDEN, PMD_CURRENT_PATH ) ;
      rdvNotEmpty( pEX, _krcbDbPath ) ;
      // --indexpath
      rdxPath( pEX, PMD_OPTION_IDXPATH, _krcbIndexPath, sizeof(_krcbIndexPath),
               FALSE, PMD_CFG_CHANGE_FORBIDDEN, "" ) ;
      // --diagpath
      rdxPath( pEX, PMD_OPTION_DIAGLOGPATH, _krcbDiagLogPath,
               sizeof(_krcbDiagLogPath), FALSE, PMD_CFG_CHANGE_REBOOT, "" ) ;
      // --auditpath
      rdxPath( pEX, PMD_OPTION_AUDITLOGPATH, _krcbAuditLogPath,
               sizeof(_krcbAuditLogPath), FALSE, PMD_CFG_CHANGE_REBOOT, "" ) ;
      // --logpath
      rdxPath( pEX, PMD_OPTION_LOGPATH, _krcbLogPath, sizeof(_krcbLogPath),
               FALSE, PMD_CFG_CHANGE_FORBIDDEN, "" ) ;
      // --bkuppath
      rdxPath( pEX, PMD_OPTION_BKUPPATH, _krcbBkupPath, sizeof(_krcbBkupPath),
               FALSE, PMD_CFG_CHANGE_REBOOT, "" ) ;
      // --wwwpath
      rdxPath( pEX, PMD_OPTION_WWWPATH, _krcbWWWPath, sizeof(_krcbWWWPath),
               FALSE, PMD_CFG_CHANGE_FORBIDDEN, "", TRUE ) ;

      // --lobpath
      rdxPath( pEX, PMD_OPTION_LOBPATH, _krcbLobPath, sizeof(_krcbLobPath),
               FALSE, PMD_CFG_CHANGE_FORBIDDEN, "" ) ;
      // --lobmetapath
      rdxPath( pEX, PMD_OPTION_LOBMETAPATH, _krcbLobMetaPath, sizeof(_krcbLobMetaPath),
               FALSE, PMD_CFG_CHANGE_FORBIDDEN, "" ) ;

      // --maxpool
      rdxUInt( pEX, PMD_OPTION_MAXPOOL, _krcbMaxPool, FALSE, PMD_CFG_CHANGE_RUN,
               PMD_OPTION_DFT_MAXPOOL ) ;
      rdvMinMax( pEX, _krcbMaxPool, 0, 10000, TRUE ) ;

      // --diagnum
      rdxInt( pEX, PMD_OPTION_DIAGLOG_NUM, _dialogFileNum, FALSE, PMD_CFG_CHANGE_RUN,
              PD_DFT_FILE_NUM ) ;
      rdvMinMax( pEX, _dialogFileNum, PD_MIN_FILE_NUM,
                 OSS_SINT32_MAX, TRUE ) ;
      // --auditnum
      rdxInt( pEX, PMD_OPTION_AUDIT_NUM, _auditFileNum, FALSE, PMD_CFG_CHANGE_RUN,
              PD_DFT_FILE_NUM ) ;
      rdvMinMax( pEX, _auditFileNum, PD_MIN_FILE_NUM,
                 OSS_SINT32_MAX, TRUE ) ;
      // --auditmask
      rdxString( pEX, PMD_OPTION_AUDIT_MASK, _auditMaskStr, sizeof(_auditMaskStr),
                 FALSE, PMD_CFG_CHANGE_RUN, AUDIT_MASK_DFT_STR ) ;
      // --ftmask
      rdxString( pEX, PMD_OPTION_FT_MASK, _ftMaskStr, sizeof(_ftMaskStr),
                 FALSE, PMD_CFG_CHANGE_RUN, PMD_FT_MASK_DFT_STR ) ;
      // --ftconfirmperiod
      rdxUInt( pEX, PMD_OPTION_FT_CONFIRM_PERIOD, _ftConfirmPeriod,
               FALSE, PMD_CFG_CHANGE_RUN, PMD_FT_CACL_INTERVAL_DFT ) ;
      rdvMinMax( pEX, _ftConfirmPeriod, PMD_FT_CACL_INTERVAL_MIN,
                 PMD_FT_CACL_INTERVAL_MAX ) ;
      // --ftconfirmratio
      rdxUInt( pEX, PMD_OPTION_FT_CONFIRM_RATIO, _ftConfirmRatio,
               FALSE, PMD_CFG_CHANGE_RUN, PMD_FT_CACL_RATIO_DFT ) ;
      rdvMinMax( pEX, _ftConfirmRatio, PMD_FT_CACL_RATIO_MIN,
                 PMD_FT_CACL_RATIO_MAX ) ;
      // --ftlevel
      rdxInt( pEX, PMD_OPTION_FT_LEVEL, _ftLevel,
              FALSE, PMD_CFG_CHANGE_RUN, FT_LEVEL_SEMI ) ;
      rdvMinMax( pEX, _ftLevel, FT_LEVEL_FUSING, FT_LEVEL_WHOLE ) ;
      // --ftfusingtimeout
      rdxUInt( pEX, PMD_OPTION_FT_FUSING_TIMEOUT, _ftFusingTimeout,
               FALSE, PMD_CFG_CHANGE_RUN, PMD_DFT_FTFUSING_TIMEOUT ) ;
      rdvMinMax( pEX, _ftFusingTimeout, 0, 3600 ) ;
      // --ftslownodethreshold
      rdxUInt( pEX, PMD_OPTION_FT_SLOWNODE_THRESHOLD, _ftSlowNodeThreshold,
               FALSE, PMD_CFG_CHANGE_RUN, PMD_DFT_FTSLOWNODE_THRESHOLD,
               TRUE ) ;
      rdvMinMax( pEX, _ftSlowNodeThreshold, 1, 10000 ) ;
      // --ftslownodeincrement
      rdxUInt( pEX, PMD_OPTION_FT_SLOWNODE_INCREMENT, _ftSlowNodeIncrement,
               FALSE, PMD_CFG_CHANGE_RUN, PMD_DFT_FTSLOWNODE_INCREMENT,
               TRUE ) ;
      rdvMinMax( pEX, _ftSlowNodeIncrement, 0, 10000 ) ;
      // --syncwaittimeout
      rdxUInt( pEX, PMD_OPTION_SYNCWAIT_TIMEOUT, _syncwaitTimeout,
               FALSE, PMD_CFG_CHANGE_RUN, PMD_DFT_SYNCWAIT_TIMEOUT ) ;
      rdvMinMax( pEX, _syncwaitTimeout, 1, 3600 ) ;
      // --shutdownwaittimeout
      rdxUInt( pEX, PMD_OPTION_SHUTDOWN_WAITTIMEOUT, _shutdownWaitTimeout,
               FALSE, PMD_CFG_CHANGE_RUN, PMD_DFT_SHUTDOWN_WAIT_TIMEOUT ) ;
      rdvMinMax( pEX, _shutdownWaitTimeout, 0, 864000 ) ;
      // --svcname
      rdxString( pEX, PMD_OPTION_SVCNAME, _krcbSvcName, sizeof(_krcbSvcName),
                 FALSE, PMD_CFG_CHANGE_FORBIDDEN,
                 boost::lexical_cast<string>(OSS_DFT_SVCPORT).c_str() ) ;
      rdvNotEmpty( pEX, _krcbSvcName ) ;
      // --replname
      rdxString( pEX, PMD_OPTION_REPLNAME, _replServiceName,
                 sizeof(_replServiceName), FALSE, PMD_CFG_CHANGE_FORBIDDEN, "" ) ;
      // --catalogname
      rdxString( pEX, PMD_OPTION_CATANAME, _catServiceName,
                 sizeof(_catServiceName), FALSE, PMD_CFG_CHANGE_FORBIDDEN, "" ) ;
      // --shardname
      rdxString( pEX, PMD_OPTION_SHARDNAME, _shardServiceName,
                 sizeof(_shardServiceName), FALSE, PMD_CFG_CHANGE_FORBIDDEN, "" ) ;
      // --restname
      rdxString( pEX, PMD_OPTION_RESTNAME, _restServiceName,
                 sizeof(_restServiceName), FALSE, PMD_CFG_CHANGE_FORBIDDEN, "" ) ;
      // --omname
      rdxString( pEX, PMD_OPTION_OMNAME, _omServiceName,
                 sizeof(_omServiceName), FALSE, PMD_CFG_CHANGE_FORBIDDEN, "", TRUE ) ;
      // --diaglevel
      rdxUShort( pEX, PMD_OPTION_DIAGLEVEL, _krcbDiagLvl, FALSE, PMD_CFG_CHANGE_RUN,
                 (UINT16)PDWARNING ) ;
      rdvMinMax( pEX, _krcbDiagLvl, PDSEVERE, PDDEBUG, TRUE ) ;
      // --role
      rdxString( pEX, PMD_OPTION_ROLE, _krcbRole, sizeof(_krcbRole),
                 FALSE, PMD_CFG_CHANGE_FORBIDDEN, SDB_ROLE_STANDALONE_STR ) ;
      rdvNotEmpty( pEX, _krcbRole ) ;
      // --logfilesz
      rdxUInt( pEX, PMD_OPTION_LOGFILESZ, _logFileSz, FALSE, PMD_CFG_CHANGE_FORBIDDEN,
               PMD_DFT_LOG_FILE_SZ ) ;
      rdvMinMax( pEX, _logFileSz, PMD_MIN_LOG_FILE_SZ, PMD_MAX_LOG_FILE_SZ,
                 TRUE ) ;
      // --logfilenum
      rdxUInt( pEX, PMD_OPTION_LOGFILENUM, _logFileNum, FALSE, PMD_CFG_CHANGE_FORBIDDEN,
               PMD_DFT_LOG_FILE_NUM ) ;
      rdvMinMax( pEX, _logFileNum, 1, 60000, TRUE ) ;
      // --logbuffsize
      rdxUInt( pEX, PMD_OPTION_LOGBUFFSIZE, _logBuffSize, FALSE, PMD_CFG_CHANGE_REBOOT,
               DPS_DFT_LOG_BUF_SZ ) ;
      rdvMinMax( pEX, _logBuffSize, 512, 1024000, TRUE ) ;
      // --numpreload
      rdxUInt( pEX, PMD_OPTION_NUMPRELOAD, _numPreLoaders, FALSE, PMD_CFG_CHANGE_REBOOT, 0 ) ;
      rdvMinMax( pEX, _numPreLoaders, 0, 100, TRUE ) ;
      // --maxprefpool
      rdxUInt( pEX, PMD_OPTION_MAX_PREF_POOL, _maxPrefPool, FALSE, PMD_CFG_CHANGE_REBOOT,
               PMD_MAX_PREF_POOL ) ;
      rdvMinMax( pEX, _maxPrefPool, 0, 1000, TRUE ) ;
      // --maxsubquery
      rdxUInt( pEX, PMD_OPTION_MAX_SUB_QUERY, _maxSubQuery, FALSE, PMD_CFG_CHANGE_RUN,
               PMD_MAX_SUB_QUERY <= _maxPrefPool ?
               PMD_MAX_SUB_QUERY : _maxPrefPool ) ;
      rdvMinMax( pEX, _maxSubQuery, 0, _maxPrefPool, TRUE ) ;
      // --maxreplsync
      rdxUInt( pEX, PMD_OPTION_MAX_REPL_SYNC, _maxReplSync, FALSE, PMD_CFG_CHANGE_RUN,
               PMD_DEFAULT_MAX_REPLSYNC ) ;
      rdvMinMax( pEX, _maxReplSync, 0, 200, TRUE ) ;
      // --replbucketsize
      rdxUInt( pEX, PMD_OPTION_REPL_BUCKET_SIZE, _replBucketSize, FALSE,
               PMD_CFG_CHANGE_REBOOT, PMD_DFT_REPL_BUCKET_SIZE, TRUE ) ;
      rdvMinMax( pEX, _replBucketSize, 1, 4096, TRUE ) ;
      // --syncstrategy
      rdxString( pEX, PMD_OPTION_SYNC_STRATEGY, _syncStrategyStr,
                 sizeof( _syncStrategyStr ), FALSE, PMD_CFG_CHANGE_RUN, "", FALSE ) ;
      // --preferedinstance / --preferredinstance
      rdxString( pEX, PMD_OPTION_PREFINST, _prefInstStr,
                 sizeof( _prefInstStr ), FALSE, PMD_CFG_CHANGE_RUN, PMD_DFT_PREFINST ) ;
      rdxString( pEX, PMD_OPTION_PREFERREDINST, _prefInstStr,
                 sizeof( _prefInstStr ), FALSE, PMD_CFG_CHANGE_RUN, _prefInstStr ) ;
      // --preferedinstancemode / --preferredinstancemode
      rdxString( pEX, PMD_OPTION_PREFINST_MODE, _prefInstModeStr,
                 sizeof( _prefInstModeStr ), FALSE, PMD_CFG_CHANGE_RUN,
                 PMD_DFT_PREFINST_MODE ) ;
      rdxString( pEX, PMD_OPTION_PREFERREDINST_MODE, _prefInstModeStr,
                 sizeof( _prefInstModeStr ), FALSE, PMD_CFG_CHANGE_RUN,
                 _prefInstModeStr ) ;
      // --preferedstrict / --preferredstrict
      rdxBooleanS( pEX, PMD_OPTION_PREFINST_STRICT, _preferredStrict, FALSE,
                   PMD_CFG_CHANGE_RUN, FALSE ) ;
      rdxBooleanS( pEX, PMD_OPTION_PREFERREDINST_STRICT, _preferredStrict, FALSE,
                   PMD_CFG_CHANGE_RUN, _preferredStrict ) ;
      // --preferedperiod / --preferredperiod
      rdxInt( pEX, PMD_OPTION_PREFINST_PERIOD, _preferredPeriod, FALSE,
              PMD_CFG_CHANGE_RUN, PMD_DFT_PREFINST_PERIOD ) ;
      rdxInt( pEX, PMD_OPTION_PREFERREDINST_PERIOD, _preferredPeriod, FALSE,
              PMD_CFG_CHANGE_RUN, _preferredPeriod ) ;
      rdvMinMax( pEX, _preferredPeriod, -1, OSS_SINT32_MAX, TRUE ) ;
      // --preferredconstraint
      rdxString( pEX, PMD_OPTION_PREFERRED_CONSTRAINT, _prefConstraint,
                 sizeof( _prefConstraint ), FALSE, PMD_CFG_CHANGE_RUN,
                 PMD_DFT_PREF_CONSTRAINT ) ;
      // --instanceid
      rdxUInt( pEX, PMD_OPTION_INSTANCE_ID, _instanceID, FALSE, PMD_CFG_CHANGE_REBOOT,
               PMD_DFT_INSTANCE_ID, FALSE ) ;
      rdvMinMax( pEX, _instanceID, NODE_INSTANCE_ID_UNKNOWN,
                 NODE_INSTANCE_ID_MAX - 1, TRUE ) ;
      // --dataerrorop
      rdxUInt( pEX, PMD_OPTION_DATAERROR_OP, _dataErrorOp,
               FALSE, PMD_CFG_CHANGE_RUN, PMD_OPT_VALUE_FULLSYNC, FALSE ) ;
      rdvMinMax( pEX, _dataErrorOp, PMD_OPT_VALUE_NONE,
                 PMD_OPT_VALUE_SHUTDOWN, TRUE ) ;
      // --memdebug
      rdxBooleanS( pEX, PMD_OPTION_MEMDEBUG, _memDebugEnabled, FALSE,
                   PMD_CFG_CHANGE_RUN, FALSE, TRUE ) ;
      // --memdebugdetail
      rdxBooleanS( pEX, PMD_OPTION_MEMDEBUGDETAIL, _memDebugDetail, FALSE,
                   PMD_CFG_CHANGE_RUN, FALSE, TRUE ) ;
      // --memdebugverify
      rdxBooleanS( pEX, PMD_OPTION_MEMDEBUGVERIFY, _memDebugVerify, FALSE,
                   PMD_CFG_CHANGE_RUN, FALSE, TRUE ) ;
      // --memdebugmask
      rdxString( pEX, PMD_OPTION_MEMDEBUGMASK, _memDebugMaskStr,
                 sizeof( _memDebugMaskStr ), FALSE, PMD_CFG_CHANGE_RUN,
                 OSS_MEMDEBUG_MASK_DFT_STR, FALSE ) ;
      // --memdebugsize
      rdxUInt( pEX, PMD_OPTION_MEMDEBUGSIZE, _memDebugSize, FALSE,
               PMD_CFG_CHANGE_RUN, 0, TRUE ) ;
      // --indexscanstep
      rdxUInt( pEX, PMD_OPTION_INDEX_SCAN_STEP, _indexScanStep, FALSE,
               PMD_CFG_CHANGE_RUN, PMD_DFT_INDEX_SCAN_STEP, TRUE ) ;
      rdvMinMax( pEX, _indexScanStep, 1, 10000, TRUE ) ;
      // --dpslocal
      rdxBooleanS( pEX, PMD_OPTION_DPSLOCAL, _dpslocal, FALSE,
                   PMD_CFG_CHANGE_REBOOT, FALSE, TRUE ) ;
      // --traceOn
      rdxBooleanS( pEX, PMD_OPTION_TRACEON, _traceOn, FALSE,
                   PMD_CFG_CHANGE_REBOOT, FALSE, TRUE ) ;
      // --traceBufSz
      rdxUInt( pEX, PMD_OPTION_TRACEBUFSZ, _traceBufSz, FALSE,
               PMD_CFG_CHANGE_REBOOT, TRACE_DFT_BUFFER_SIZE, TRUE ) ;
      rdvMinMax( pEX, _traceBufSz, TRACE_MIN_BUFFER_SIZE,
                 TRACE_MAX_BUFFER_SIZE, TRUE ) ;
      // --transactionOn
      rdxBooleanS( pEX, PMD_OPTION_TRANSACTIONON, _transactionOn, FALSE,
                   PMD_CFG_CHANGE_REBOOT, TRUE ) ;
      // --transactiontimeout
      rdxUInt( pEX, PMD_OPTION_TRANSTIMEOUT, _transTimeout, FALSE,
               PMD_CFG_CHANGE_RUN, DPS_TRANS_DFT_TIMEOUT, FALSE ) ;
      rdvMinMax( pEX, _transTimeout, 0, 3600, TRUE ) ;
      // --transisolation
      rdxInt( pEX, PMD_OPTION_TRANS_ISOLATION, _transIsolation, FALSE,
              PMD_CFG_CHANGE_RUN, DPS_TRANS_ISOLATION_DFT, FALSE ) ;
      rdvMinMax( pEX, _transIsolation,
                 TRANS_ISOLATION_RU, TRANS_ISOLATION_MAX - 1, TRUE ) ;
      // --translockwait
      rdxBooleanS( pEX, PMD_OPTION_TRANS_LOCKWAIT, _transLockwait, FALSE,
                   PMD_CFG_CHANGE_RUN, DPS_TRANS_LOCKWAIT_DFT, FALSE ) ;
      // --transautocommit
      rdxBooleanS( pEX, PMD_OPTION_TRANS_AUTOCOMMIT, _transAutoCommit, FALSE,
                   PMD_CFG_CHANGE_RUN, DPS_TRANS_AUTOCOMMIT_DFT, FALSE ) ;
      // --transautorollback
      rdxBooleanS( pEX, PMD_OPTION_TRANS_AUTOROLLBACK, _transAutoRollback, FALSE,
                   PMD_CFG_CHANGE_RUN, DPS_TRANS_AUTOROLLBACK_DFT, FALSE ) ;
      // --transuserbs
      rdxBooleanS( pEX, PMD_OPTION_TRANS_USE_RBS, _transUseRBS, FALSE,
                   PMD_CFG_CHANGE_RUN, DPS_TRANS_USE_RBS_DFT, FALSE ) ;
      // --sharingBreak
      rdxUInt( pEX, PMD_OPTION_SHARINGBRK, _sharingBreakTime, FALSE,
               PMD_CFG_CHANGE_RUN, PMD_OPTION_BRK_TIME_DEFAULT, TRUE ) ;
      rdvMinMax( pEX, _sharingBreakTime, 5000, 300000, TRUE ) ;
      // --startshifttime
      rdxUInt( pEX, PMD_OPTION_START_SHIFT_TIME, _startShiftTime, FALSE,
               PMD_CFG_CHANGE_RUN, PMD_DFT_START_SHIFT_TIME, TRUE ) ;
      rdvMinMax( pEX, _startShiftTime, 0, 7200, TRUE ) ;
      // --catalogaddr
      rdxString( pEX, PMD_OPTION_CATALOG_ADDR, _catAddrLine,
                 sizeof(_catAddrLine), FALSE, PMD_CFG_CHANGE_FORBIDDEN, "" ) ;

      // --dmsTmpBlkPath
      rdxPath( pEX, PMD_OPTION_DMS_TMPBLKPATH, _dmsTmpBlkPath,
               sizeof(_dmsTmpBlkPath), FALSE, PMD_CFG_CHANGE_REBOOT, "" ) ;

      // --sortBufSz
      rdxUInt( pEX, PMD_OPTION_SORTBUF_SIZE, _sortBufSz,
               FALSE, PMD_CFG_CHANGE_RUN, PMD_DEFAULT_SORTBUF_SZ ) ;
      rdvMinMax( pEX, _sortBufSz, PMD_MIN_SORTBUF_SZ,
                 -1, TRUE ) ;

      // --hjBufSz
      rdxUInt( pEX, PMD_OPTION_HJ_BUFSZ, _hjBufSz,
               FALSE, PMD_CFG_CHANGE_RUN, PMD_DEFAULT_HJ_SZ ) ;
      rdvMinMax( pEX, _hjBufSz, PMD_MIN_HJ_SZ,
                 -1, TRUE ) ;

      rdxBooleanS( pEX, PMD_OPTION_DIRECT_IO_IN_LOB, _directIOInLob,
                   FALSE, PMD_CFG_CHANGE_RUN, FALSE, FALSE ) ;

      rdxBooleanS( pEX, PMD_OPTION_SPARSE_FILE, _sparseFile,
                   FALSE, PMD_CFG_CHANGE_RUN, TRUE, FALSE ) ;

      // --weight
      rdxUInt( pEX, PMD_OPTION_WEIGHT, _weight,
               FALSE, PMD_CFG_CHANGE_RUN, 10, FALSE ) ;
      rdvMinMax( pEX, _weight, 1, 100, TRUE ) ;

#ifdef SDB_ENTERPRISE

#ifdef SDB_SSL
      // --usessl
      rdxBooleanS( pEX, PMD_OPTION_USESSL, _useSSL,
                   FALSE, PMD_CFG_CHANGE_RUN, FALSE, FALSE ) ;
#endif

#endif /* SDB_ENTERPRISE */
      // --auth
      rdxBooleanS( pEX, PMD_OPTION_AUTH, _auth,
                   FALSE, PMD_CFG_CHANGE_RUN, TRUE, FALSE ) ;
      // --planbuckets
      rdxUInt( pEX, PMD_OPTION_PLAN_BUCKETS, _planBucketNum,
               FALSE, PMD_CFG_CHANGE_RUN, OPT_PLAN_DEF_CACHE_BUCKETS, FALSE ) ;
      rdvMinMax( pEX, _planBucketNum, OPT_PLAN_MIN_CACHE_BUCKETS,
                 OPT_PLAN_MAX_CACHE_BUCKETS, TRUE ) ;
      // --optimeout
      rdxUInt( pEX, PMD_OPTION_OPERATOR_TIMEOUT, _oprtimeout, FALSE,
               PMD_CFG_CHANGE_RUN, PMD_OPTION_OPR_TIME_DEFAULT, FALSE ) ;
      // --overflowratio
      rdxUInt( pEX, PMD_OPTION_OVER_FLOW_RATIO, _overflowRatio, FALSE,
               PMD_CFG_CHANGE_RUN, PMD_DFT_OVERFLOW_RETIO, FALSE ) ;
      rdvMinMax( pEX, _overflowRatio, 0, 10000, TRUE ) ;
      // --extendthreshold
      rdxUInt( pEX, PMD_OPTION_EXTEND_THRESHOLD, _extendThreshold, FALSE,
               PMD_CFG_CHANGE_RUN, PMD_DFT_EXTEND_THRESHOLD, TRUE ) ;
      rdvMinMax( pEX, _extendThreshold, 0, 128, TRUE ) ;
      // --signalinterval
      rdxUInt( pEX, PMD_OPTION_SIGNAL_INTERVAL, _signalInterval, FALSE,
              PMD_CFG_CHANGE_RUN, 0, TRUE ) ;
      // --maxcachesize
      rdxUInt( pEX, PMD_OPTION_MAX_CACHE_SIZE, _maxCacheSize, FALSE,
               PMD_CFG_CHANGE_RUN,0, FALSE ) ;
      /// --maxcachejob
      rdxUInt( pEX, PMD_OPTION_MAX_CACHE_JOB, _maxCacheJob, FALSE,
               PMD_CFG_CHANGE_RUN, PMD_DFT_MAX_CACHE_JOB, FALSE ) ;
      rdvMinMax( pEX, _maxCacheJob, 2, 200, TRUE ) ;
      /// --maxsyncjob
      rdxUInt( pEX, PMD_OPTION_MAX_SYNC_JOB, _maxSyncJob, FALSE,
               PMD_CFG_CHANGE_RUN, PMD_DFT_MAX_SYNC_JOB, FALSE ) ;
      rdvMinMax( pEX, _maxSyncJob, 2, 200, TRUE ) ;
      /// --syncinterval
      rdxUInt( pEX, PMD_OPTION_SYNC_INTERVAL, _syncInterval, FALSE,
               PMD_CFG_CHANGE_RUN, PMD_DFT_SYNC_INTERVAL, FALSE ) ;
      /// --syncrecordnum
      rdxUInt( pEX, PMD_OPTION_SYNC_RECORDNUM, _syncRecordNum, FALSE,
               PMD_CFG_CHANGE_RUN, PMD_DFT_SYNC_RECORDNUM, FALSE ) ;
      /// --syncdeep
      rdxBooleanS( pEX, PMD_OPTION_SYNC_DEEP, _syncDeep, FALSE,
                   PMD_CFG_CHANGE_RUN, FALSE, FALSE ) ;

      // --archiveon
      rdxBooleanS( pEX, PMD_OPTION_ARCHIVE_ON, _archiveOn,
                   FALSE, PMD_CFG_CHANGE_REBOOT, FALSE, FALSE ) ;

      // --archivecompresson
      rdxBooleanS( pEX, PMD_OPTION_ARCHIVE_COMPRESS_ON, _archiveCompressOn,
                   FALSE, PMD_CFG_CHANGE_RUN, TRUE, FALSE ) ;

      // --archivepath
      rdxPath( pEX, PMD_OPTION_ARCHIVE_PATH, _archivePath, sizeof(_archivePath),
               FALSE, PMD_CFG_CHANGE_FORBIDDEN, "" ) ;

      // --archivetimeout
      rdxUInt( pEX, PMD_OPTION_ARCHIVE_TIMEOUT, _archiveTimeout, FALSE,
               PMD_CFG_CHANGE_RUN, PMD_DFT_ARCHIVE_TIMEOUT, FALSE ) ;

      // --archiveexpired
      rdxUInt( pEX, PMD_OPTION_ARCHIVE_EXPIRED, _archiveExpired, FALSE,
               PMD_CFG_CHANGE_RUN, PMD_DFT_ARCHIVE_EXPIRED, FALSE ) ;
      rdvMinMax( pEX, _archiveExpired, 0, PMD_MAX_ARCHIVE_EXPIRED, TRUE ) ;

      // --archivequota
      rdxUInt( pEX, PMD_OPTION_ARCHIVE_QUOTA, _archiveQuota, FALSE,
               PMD_CFG_CHANGE_RUN, PMD_DFT_ARCHIVE_QUOTA, FALSE ) ;

      // --omaddr
      rdxString( pEX, PMD_OPTION_OM_ADDR, _omAddrLine,
                 sizeof(_omAddrLine), FALSE, PMD_CFG_CHANGE_FORBIDDEN, "" ) ;

      // --dmschkinterval
      rdxUInt( pEX, PMD_OPTION_DMS_CHK_INTERVAL, _dmsChkInterval,
               FALSE, PMD_CFG_CHANGE_RUN, PMD_DFT_DMS_CHK_INTERVAL, TRUE ) ;
      rdvMinMax( pEX, _dmsChkInterval, 0, 240, TRUE ) ;

      // --cachemergesz
      rdxUInt( pEX, PMD_OPTION_CACHE_MERGE_SIZE, _cacheMergeSize,
               FALSE, PMD_CFG_CHANGE_RUN, PMD_DFT_CACHE_MERGE_SZ, TRUE ) ;
      rdvMinMax( pEX, _cacheMergeSize, 0, 64, TRUE ) ;

      // --pagealloctimeout
      rdxUInt( pEX, PMD_OPTION_PAGE_ALLOC_TIMEOUT, _pageAllocTimeout,
               FALSE, PMD_CFG_CHANGE_RUN, PMD_DFT_PAGE_ALLOC_TIMEOUT, TRUE ) ;
      rdvMinMax( pEX, _pageAllocTimeout, 0, 3600000, TRUE ) ;

      // --perfstat
      rdxBooleanS( pEX, PMD_OPTION_PERF_STAT, _perfStat, FALSE,
                   PMD_CFG_CHANGE_RUN, FALSE, TRUE ) ;

      // --optcostthreshold
      rdxInt( pEX, PMD_OPTION_OPT_COST_THRESHOLD, _optCostThreshold, FALSE,
              PMD_CFG_CHANGE_RUN, PMD_DFT_OPT_COST_THRESHOLD, TRUE ) ;
      rdvMinMax( pEX, _optCostThreshold, -1, INT_MAX, TRUE ) ;

      // --plancachemainclthreshold
      rdxInt( pEX, PMD_OPTION_PLAN_CACHE_MAINCL_THRESHOLD, _planCacheMainCLThreshold, FALSE,
              PMD_CFG_CHANGE_RUN, PMD_DFT_PLAN_CACHE_MAINCL_THRESHOLD, TRUE ) ;
      rdvMinMax( pEX, _planCacheMainCLThreshold, -1, INT_MAX, TRUE ) ;

      // --enablemixcmp
      rdxBooleanS( pEX, PMD_OPTION_ENABLE_MIX_CMP, _enableMixCmp, FALSE,
                   PMD_CFG_CHANGE_RUN, PMD_DFT_ENABLE_MIX_CMP, TRUE ) ;

      // --optcachelevel
      rdxUInt( pEX, PMD_OPTION_PLAN_CACHE_LEVEL, _planCacheLevel, FALSE,
               PMD_CFG_CHANGE_RUN, OPT_PLAN_PARAMETERIZED, FALSE ) ;
      rdvMinMax( pEX, _planCacheLevel, OPT_PLAN_NOCACHE, OPT_PLAN_FUZZYOPTR,
                 TRUE ) ;

      // --maxconn
      rdxUInt( pEX, PMD_OPTION_MAX_CONN,_maxconn, FALSE,
               PMD_CFG_CHANGE_RUN, PMD_DFT_MAX_CONN, FALSE ) ;
      rdvMinMax( pEX, _maxconn, 0, 30000, TRUE ) ;

      // --svcscheduler
      rdxUInt( pEX, PMD_OPTION_SVCSCHEDULER, _svcSchedulerType, FALSE,
               PMD_CFG_CHANGE_REBOOT, 0, FALSE ) ;
      rdvMinMax( pEX, _svcSchedulerType, SCHED_TYPE_NONE, SCHED_TYPE_CONTAINER,
                 TRUE ) ;

      // --svcmaxconcurrency
      rdxUInt( pEX, PMD_OPTION_SVC_MAX_CONCURRENCY, _svcMaxConcurrency, FALSE,
               PMD_CFG_CHANGE_RUN, 100, FALSE ) ;

      // --logwritemod
      rdxString( pEX, PMD_OPTION_LOGWRITEMOD, _logWriteModStr,
                 sizeof( _logWriteModStr ), FALSE, PMD_CFG_CHANGE_RUN,
                 PMD_DFT_LOGWRITEMOD, FALSE ) ;

      // --logtimeon
      rdxBooleanS( pEX, PMD_OPTION_LOG_TIME_ON, _logTimeOn,
                   FALSE, PMD_CFG_CHANGE_RUN, FALSE, FALSE ) ;

      // --enablesleep
      rdxBooleanS( pEX, PMD_OPTION_SLEEP, _enableSleep, FALSE,
                   PMD_CFG_CHANGE_RUN, FALSE, TRUE ) ;
      // --recyclerecord
      rdxBooleanS( pEX, PMD_OPTION_RECYCLE_RECORD, _recycleRecord, FALSE,
                   PMD_CFG_CHANGE_RUN, FALSE, TRUE ) ;

      // --indexcoveron
      rdxBooleanS( pEX, PMD_OPTION_INDEXCOVERON, _indexCoverOn, FALSE,
                   PMD_CFG_CHANGE_RUN, TRUE ) ;

      // --maxsocketpernode
      rdxUInt( pEX, PMD_OPTION_MAXSOCKET_PER_NODE, _maxSockPerNode, FALSE,
               PMD_CFG_CHANGE_RUN, 5, FALSE ) ;
      rdvMinMax( pEX, _maxSockPerNode, 1, 100, TRUE ) ;

      // --maxsocketperthread
      rdxUInt( pEX, PMD_OPTION_MAXSOCKET_PER_THREAD, _maxSockPerThread, FALSE,
               PMD_CFG_CHANGE_RUN, 1, FALSE ) ;

      // --maxsocketthread
      rdxUInt( pEX, PMD_OPTION_MAX_SOCKET_THREAD, _maxSockThread, FALSE,
               PMD_CFG_CHANGE_RUN, 10, FALSE ) ;
      rdvMinMax( pEX, _maxSockThread, 1, 100, TRUE ) ;

      // --tcmaxsize
      rdxUInt( pEX, PMD_OPTION_TC_MAX_SIZE, _maxTCSize, FALSE,
               PMD_CFG_CHANGE_RUN, 2048, TRUE ) ;
      rdvMinMax( pEX, _maxTCSize, 0, 65536, TRUE ) ;

      // --mempoolsize
      rdxUInt( pEX, PMD_OPTION_MEMPOOL_SIZE, _memPoolSize, FALSE,
               PMD_CFG_CHANGE_RUN, 8192, TRUE ) ;

      // --mempoolthreshold
      rdxUInt( pEX, PMD_OPTION_MEMPOOL_THRESHOLD, _memPoolThreshold, FALSE,
               PMD_CFG_CHANGE_RUN, 0, TRUE ) ;

      // --transreplsize
      rdxInt( pEX, PMD_OPTION_TRANS_REPLSIZE, _transReplSize, FALSE,
              PMD_CFG_CHANGE_RUN, 2, TRUE ) ;
      rdvMinMax( pEX, _transReplSize, -1, CLS_REPLSET_MAX_NODE_SIZE ) ;

      // --transrccount
      rdxBooleanS( pEX, PMD_OPTION_TRANS_RCCOUNT, _transRCCount, FALSE,
                   PMD_CFG_CHANGE_RUN, DPS_TRANS_RCCOUNT_DFT, FALSE ) ;

      // --transallowlockescalation
      rdxBooleanS( pEX, PMD_OPTION_TRANSALLOWLOCKESCALATION,
                   _transAllowLockEscalation, FALSE, PMD_CFG_CHANGE_RUN,
                   DPS_TRANS_ALLOWLOCKESCALATION_DFT, FALSE ) ;

      // --transmaxlocknum
      rdxInt( pEX, PMD_OPTION_TRANSMAXLOCKNUM, _transMaxLockNum, FALSE,
              PMD_CFG_CHANGE_RUN, DPS_TRANS_MAXLOCKNUM_DFT, FALSE ) ;
      rdvMinMax( pEX, _transMaxLockNum, DPS_TRANS_MAXLOCKNUM_MIN,
                 DPS_TRANS_MAXLOCKNUM_MAX ) ;

      // --transmaxlogspaceratio
      rdxInt( pEX, PMD_OPTION_TRANSMAXLOGSPACERATIO, _transMaxLogSpaceRatio,
              FALSE, PMD_CFG_CHANGE_RUN, DPS_TRANS_MAXLOGSPACERATIO_DFT, FALSE ) ;
      rdvMinMax( pEX, _transMaxLogSpaceRatio, DPS_TRANS_MAXLOGSPACERATIO_MIN,
                 DPS_TRANS_MAXLOGSPACERATIO_MAX ) ;

      // --transonsistencystrategy
      rdxInt( pEX, PMD_OPTION_TRANSCONSISTENCYSTRATEGY, (INT32&)_transConsistencyStrategy,
              FALSE, PMD_CFG_CHANGE_RUN,
              (INT32)SDB_CONSISTENCY_PRY_LOC_MAJOR, TRUE ) ;
      rdvMinMax( pEX, (INT32&)_transConsistencyStrategy, (INT32)SDB_CONSISTENCY_NODE,
                 (INT32)SDB_CONSISTENCY_PRY_LOC_MAJOR ) ;

      // --monslowquerythreshold
      rdxUInt( pEX, PMD_OPTION_MON_SLOWQUERY_THRESHOLD, _slowQueryThreshold, FALSE,
               PMD_CFG_CHANGE_RUN, 300, TRUE ) ;

      // --mongroupmask
      rdxString( pEX, PMD_OPTION_MON_GROUP_MASK, _monGroupMaskStr,
                 sizeof (_monGroupMaskStr), FALSE, PMD_CFG_CHANGE_RUN,
                 PMD_MON_GROUP_MASK_DEFT_STR) ;

      // --monhistevent
      rdxUInt( pEX, PMD_OPTION_MON_HIST_EVENT, _monHistEvent, FALSE,
               PMD_CFG_CHANGE_RUN, 1000, TRUE ) ;

      // --serviceumask
      rdxString( pEX, PMD_OPTION_SERVICE_MASK, _serviceMaskStr,
                 sizeof( _serviceMaskStr ), FALSE, PMD_CFG_CHANGE_REBOOT,
                 PMD_SVC_MASK_NONE_STR ) ;

      // --maxcontextnum
      rdxInt( pEX, PMD_OPTION_MAXCONTEXTNUM, _maxContextNum, FALSE,
              PMD_CFG_CHANGE_RUN, RTN_MAX_CTX_NUM_DFT, FALSE ) ;
      rdvMinMax( pEX, _maxContextNum, 0, RTN_MAX_CTX_NUM_MAX, TRUE ) ;

      // --maxsessioncontextnum
      rdxInt( pEX, PMD_OPTION_MAXSESSIONCONTEXTNUM, _maxSessionContextNum,
              FALSE, PMD_CFG_CHANGE_RUN, RTN_MAX_SESS_CTX_NUM_DFT, FALSE ) ;
      rdvMinMax( pEX, _maxSessionContextNum, 0, RTN_MAX_SESS_CTX_NUM_MAX,
                 TRUE ) ;

      // --contexttimeout
      rdxInt( pEX, PMD_OPTION_CONTEXTTIMEOUT, _contextTimeout, FALSE,
              PMD_CFG_CHANGE_RUN, RTN_CTX_TIMEOUT_DFT, FALSE ) ;
      rdvMinMax( pEX, _contextTimeout, RTN_CTX_TIMEOUT_MIN,
                 RTN_CTX_TIMEOUT_MAX, TRUE ) ;

      // --detectdisk
      rdxBooleanS( pEX, PMD_OPTION_DETECT_DISK, _detectDisk,
                   FALSE, PMD_CFG_CHANGE_RUN, TRUE, TRUE ) ;

      // --diagsecureon
      rdxBooleanS( pEX, PMD_OPTION_DIAG_SECURE_ON, _diagSecureOn,
                   FALSE, PMD_CFG_CHANGE_RUN, TRUE, FALSE ) ;

      // --metacacheexpired
      rdxUInt( pEX, PMD_OPTION_METACACHE_EXPIRED, _metacacheexpired, FALSE,
               PMD_CFG_CHANGE_RUN, PMD_DFT_METACACHE_EXPIRED, FALSE ) ;
      rdvMinMax( pEX, _metacacheexpired, 0, PMD_MAX_METACACHE_EXPIRED, TRUE ) ;

      // --metacachelwm
      rdxUInt( pEX, PMD_OPTION_METACACHE_LWM, _metacachelwm, FALSE,
               PMD_CFG_CHANGE_RUN, PMD_DFT_METACACHE_LWM, FALSE ) ;
      rdvMinMax( pEX, _metacachelwm, 0, PMD_MAX_METACACHE_LWM, TRUE ) ;

      // --statmvclimit
      rdxUInt( pEX, PMD_OPTION_STAT_MCV_LIMIT, _statMCVLimit, FALSE,
               PMD_CFG_CHANGE_RUN, PMD_DFT_STAT_MCV_LIMIT, TRUE ) ;
      rdvMinMax( pEX, _statMCVLimit, 0, PMD_MAX_STAT_MCV_LIMIT, TRUE ) ;

      // --remotelocationconsistency
      rdxBooleanS( pEX, PMD_OPTION_REMOTE_LOCATION_CONSISTENCY, _remoteLocationConsistency, FALSE,
                   PMD_CFG_CHANGE_RUN, TRUE, TRUE ) ;

      // --consultrollbacklogon
      rdxBooleanS( pEX, PMD_OPTION_CONSULT_ROLLBACK_LOG_ON, _consultRollbackLogOn, FALSE,
                   PMD_CFG_CHANGE_RUN, TRUE, TRUE ) ;

      // --privilegecheck
      rdxBooleanS( pEX, PMD_OPTION_PRIVILEGE_CHECK, _privilegeCheckEnabled,
                   FALSE, PMD_CFG_CHANGE_REBOOT, FALSE, FALSE ) ;

      // --usercacheinterval
      rdxUInt( pEX, PMD_OPTION_USER_CACHE_INTERVAL, _userCacheInterval, FALSE,
               PMD_CFG_CHANGE_REBOOT, PMD_DFT_USER_CACHE_INTERVAL, FALSE );

      // --memmxfast
      rdxInt( pEX, PMD_OPTION_MEM_MXFAST, _memMXFast, FALSE,
              PMD_CFG_CHANGE_RUN, PMD_DFT_MEM_MXFAST, TRUE ) ;
      rdvMinMax( pEX, _memMXFast, -1, 160, TRUE ) ;

      // --memtrimthreshold
      rdxInt( pEX, PMD_OPTION_MEM_TRIM_THRESHOLD, _memTrimThreshold, FALSE,
              PMD_CFG_CHANGE_RUN, PMD_DFT_MEM_TRIM_THRESHOLD, TRUE ) ;
      rdvMinMax( pEX, _memTrimThreshold, -1, 65536, TRUE ) ;

      // --memmmapthreshold
      rdxInt( pEX, PMD_OPTION_MEM_MMAP_THRESHOLD, _memMmapThreshold, FALSE,
              PMD_CFG_CHANGE_RUN, PMD_DFT_MEM_MMAP_THRESHOLD, TRUE ) ;
      rdvMinMax( pEX, _memMmapThreshold, -1, 65536, TRUE ) ;

      // --memmmapmax
      rdxInt( pEX, PMD_OPTION_MEM_MMAP_MAX, _memMmapMax, FALSE,
              PMD_CFG_CHANGE_RUN, PMD_DFT_MEM_MMAP_MAX, TRUE ) ;
      rdvMinMax( pEX, _memMmapMax, -1, 100000000, TRUE ) ;

      // --memtoppad
      rdxInt( pEX, PMD_OPTION_MEM_TOP_PAD, _memTopPad, FALSE,
              PMD_CFG_CHANGE_RUN, PMD_DFT_MEM_TOP_PAD, TRUE ) ;
      rdvMinMax( pEX, _memTopPad, -1, 16777216, TRUE ) ;

      // --storageengine
      rdxString( pEX, PMD_OPTION_STORAGEENGINE, _storageEngineName,
                 sizeof( _storageEngineName ), FALSE, PMD_CFG_CHANGE_FORBIDDEN,
                 DMS_STORAGE_ENGINE_NAME_WIREDTIGER ) ;

      // --wtcachesize
      rdxUInt( pEX, PMD_OPTION_WT_CACHE_SIZE, _wtCacheSize, FALSE,
               PMD_CFG_CHANGE_RUN, DMS_DFT_WT_CACHE_SIZE ) ;
      rdvMinMax( pEX, _wtCacheSize, DMS_MIN_WT_CACHE_SIZE,
                 DMS_MAX_WT_CACHE_SIZE, TRUE ) ;

      // --wtevicttarget
      rdxUInt( pEX, PMD_OPTION_WT_EVICT_TARGET, _wtEvictTarget, FALSE,
               PMD_CFG_CHANGE_RUN, DMS_DFT_WT_EVICT_TARGET ) ;
      rdvMinMax( pEX, _wtEvictTarget, DMS_MIN_WT_EVICT_TARGET,
                 DMS_MAX_WT_EVICT_TARGET, TRUE ) ;

      // --wtevicttrigger
      rdxUInt( pEX, PMD_OPTION_WT_EVICT_TRIGGER, _wtEvictTrigger, FALSE,
               PMD_CFG_CHANGE_RUN, DMS_DFT_WT_EVICT_TRIGGER ) ;
      rdvMinMax( pEX, _wtEvictTrigger, DMS_MIN_WT_EVICT_TRIGGER,
                 DMS_MAX_WT_EVICT_TRIGGER, TRUE ) ;

      // --wtevictdirtytarget
      rdxUInt( pEX, PMD_OPTION_WT_EVICT_DIRTY_TARGET, _wtEvictDirtyTarget, FALSE,
               PMD_CFG_CHANGE_RUN, DMS_DFT_WT_EVICT_DIRTY_TARGET ) ;
      rdvMinMax( pEX, _wtEvictDirtyTarget, DMS_MIN_WT_EVICT_DIRTY_TARGET,
                 DMS_MAX_WT_EVICT_DIRTY_TARGET, TRUE ) ;

      // --wtevictdirtytrigger
      rdxUInt( pEX, PMD_OPTION_WT_EVICT_DIRTY_TRIGGER, _wtEvictDirtyTrigger, FALSE,
               PMD_CFG_CHANGE_RUN, DMS_DFT_WT_EVICT_DIRTY_TRIGGER ) ;
      rdvMinMax( pEX, _wtEvictDirtyTrigger, DMS_MIN_WT_EVICT_DIRTY_TRIGGER,
                 DMS_MAX_WT_EVICT_DIRTY_TRIGGER, TRUE ) ;

      // --wtevictupdatestarget
      rdxUInt( pEX, PMD_OPTION_WT_EVICT_UPDATES_TARGET, _wtEvictUpdatesTarget, FALSE,
               PMD_CFG_CHANGE_RUN, DMS_DFT_WT_EVICT_UPDATES_TARGET ) ;
      rdvMinMax( pEX, _wtEvictUpdatesTarget, DMS_MIN_WT_EVICT_UPDATES_TARGET,
                 DMS_MAX_WT_EVICT_UPDATES_TARGET, TRUE ) ;

      // --wtevictupdatestrigger
      rdxUInt( pEX, PMD_OPTION_WT_EVICT_UPDATES_TRIGGER, _wtEvictUpdatesTrigger, FALSE,
               PMD_CFG_CHANGE_RUN, DMS_DFT_WT_EVICT_UPDATES_TRIGGER ) ;
      rdvMinMax( pEX, _wtEvictUpdatesTrigger, DMS_MIN_WT_EVICT_UPDATES_TRIGGER,
                 DMS_MAX_WT_EVICT_UPDATES_TRIGGER, TRUE ) ;

      // --wtevictthreadsmin
      rdxUInt( pEX, PMD_OPTION_WT_EVICT_THREADS_MIN, _wtEvictThreadsMin, FALSE,
               PMD_CFG_CHANGE_RUN, DMS_DFT_WT_EVICT_THREADS_MIN ) ;
      rdvMinMax( pEX, _wtEvictThreadsMin, DMS_MIN_WT_EVICT_THREADS_MIN,
                 DMS_MAX_WT_EVICT_THREADS_MIN, TRUE ) ;

      // --wtevictthreadsmax
      rdxUInt( pEX, PMD_OPTION_WT_EVICT_THREADS_MAX, _wtEvictThreadsMax, FALSE,
               PMD_CFG_CHANGE_RUN, DMS_DFT_WT_EVICT_THREADS_MAX ) ;
      rdvMinMax( pEX, _wtEvictThreadsMax, DMS_MIN_WT_EVICT_THREADS_MAX,
                 DMS_MAX_WT_EVICT_THREADS_MAX, TRUE ) ;

      // --wtcheckpointinterval
      rdxUInt( pEX, PMD_OPTION_WT_CHECK_POINT_INTERVAL, _wtCheckPointInterval, FALSE,
               PMD_CFG_CHANGE_RUN, DMS_DFT_WT_CHECK_POINT_INTERVAL ) ;
      rdvMinMax( pEX, _wtCheckPointInterval, DMS_MIN_WT_CHECK_POINT_INTERVAL,
                 DMS_MAX_WT_CHECK_POINT_INTERVAL, TRUE ) ;

      // end map

      return getResult () ;
   }

   INT32 _pmdOptionsMgr::postLoaded ( PMD_CFG_STEP step )
   {
      INT32 rc = SDB_OK ;
      SDB_ROLE dbRole = SDB_ROLE_STANDALONE ;
      ossPoolList< UINT8 > instanceList ;
      PMD_PREFER_INSTANCE_TYPE specInstance = PMD_PREFER_INSTANCE_TYPE_UNKNOWN ;
      PMD_PREFER_INSTANCE_MODE instanceMode = PMD_PREFER_INSTANCE_MODE_UNKNOWN ;
      PMD_PREFER_CONSTRAINT constraint = PMD_PREFER_CONSTRAINT_UNKNOWN ;
      BOOLEAN hasInvalidChar = FALSE ;

      rc = ossGetPort( _krcbSvcName, _krcbSvcPort ) ;
      if ( SDB_OK != rc )
      {
         std::cerr << "Invalid svcname: " << _krcbSvcName << endl ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( 0 != ossStrlen( _storageEngineName ) )
      {
         DMS_STORAGE_ENGINE_TYPE engineType = dmsGetStorageEngine( _storageEngineName ) ;
         if ( DMS_STORAGE_ENGINE_UNKNOWN == engineType )
         {
            std::cerr << "Invalid storage engine name: " << _storageEngineName << endl ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         _storageEngineType = engineType ;
      }
      else
      {
         _storageEngineType = DMS_STORAGE_ENGINE_MMAP ;
      }

      if ( DMS_STORAGE_ENGINE_WIREDTIGER == _storageEngineType )
      {
         // must enable transaction
         _transactionOn = TRUE ;
      }

      // logbuffsize check
      if ( _logBuffSize > _logFileNum * _logFileSz *
           ( 1048576 / DPS_DEFAULT_PAGE_SIZE ) )
      {
         std::cerr << "log buff size more than all log file size: "
                   << _logFileNum * _logFileSz << "MB" << std::endl ;
         _logBuffSize = _logFileNum * _logFileSz *
                        ( 1048576 / DPS_DEFAULT_PAGE_SIZE ) ;
         _invalidConfNum++ ;
      }

      // dbrole check
      if ( SDB_ROLE_MAX == ( dbRole = utilGetRoleEnum( _krcbRole ) ) )
      {
         std::cerr << "db role: " << _krcbRole << " error" << std::endl ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // replbucketsize check
      if ( !ossIsPowerOf2( _replBucketSize ) )
      {
         _replBucketSize = PMD_DFT_REPL_BUCKET_SIZE ;
         _invalidConfNum++ ;
      }

      // extendthreshold check
      if ( 0 != _extendThreshold && !ossIsPowerOf2( _extendThreshold ) )
      {
         _extendThreshold = PMD_DFT_EXTEND_THRESHOLD ;
         _invalidConfNum++ ;
      }

      // syncstrategy check
      if ( SDB_OK != clsString2Strategy( _syncStrategyStr, _syncStrategy ) )
      {
         std::cerr << PMD_OPTION_SYNC_STRATEGY << " value error, use default"
                   << endl ;
         _syncStrategy = CLS_SYNC_DTF_STRATEGY ;
         _invalidConfNum++ ;
      }
      _syncStrategyStr[0] = 0 ;

      // logwritemod check
      if ( SDB_OK != optString2LogMod( _logWriteModStr, _logWriteMod ) )
      {
         std::cerr << PMD_OPTION_LOGWRITEMOD << " value error, use default"
                   << endl ;
         _logWriteMod = DPS_LOG_WRITE_MOD_INCREMENT ;
         _invalidConfNum++ ;
      }
      _logWriteModStr[0] = 0 ;

      // audit mask check
      _auditMask = 0 ;
      if ( SDB_OK != pdString2AuditMask( _auditMaskStr, _auditMask, FALSE ) )
      {
         std::cerr << PMD_OPTION_AUDIT_MASK << " value error, use default"
                   << endl ;
         _auditMask = AUDIT_MASK_DEFAULT ;
         ossStrncpy( _auditMaskStr, AUDIT_MASK_DFT_STR, sizeof( _auditMaskStr ) ) ;
         _invalidConfNum++ ;
      }

      // ft mask check
      if ( SDB_OK != utilStrToFTMask( _ftMaskStr, _ftMask ) )
      {
         std::cerr << PMD_OPTION_FT_MASK << "value error, use default"
                   << endl ;
         _ftMask = PMD_FT_MASK_DFT ;
         _invalidConfNum++ ;
      }

      // mon group mask check
      _monGroupMask = 0 ;
      if ( SDB_OK != optString2MonGroupMask( _monGroupMaskStr, _monGroupMask ) )
      {
         std::cerr << PMD_OPTION_MON_GROUP_MASK << " value error, use default"
                   << endl ;
         _monGroupMask = MON_GROUP_MASK_DEFAULT ;
         _invalidConfNum++ ;
      }

      // preferedinstance
      rc = pmdParsePreferInstStr( _prefInstStr, instanceList, specInstance,
                                  hasInvalidChar ) ;
      if ( rc )
      {
         std::cerr << "Failed to parse preferd instance str, rc: "
                   << rc << endl ;
         goto error ;
      }

      // service mask check
      if ( SDB_OK != utilStrToServiceMask( _serviceMaskStr, _serviceMask ) )
      {
         std::cerr << PMD_OPTION_SERVICE_MASK << " value error" << endl ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( PMD_PREFER_INSTANCE_TYPE_UNKNOWN == specInstance &&
           instanceList.empty() )
      {
         std::cerr << PMD_OPTION_PREFINST << " value error, use default"
                   << endl ;
         ossStrncpy( _prefInstStr, PMD_DFT_PREFINST, sizeof( _prefInstStr ) ) ;
         _invalidConfNum++ ;
      }
      else
      {
         if( hasInvalidChar )
         {
            modifyPreferInstStr( instanceList, specInstance ) ;
            _invalidConfNum++ ;
         }
      }

      // preferedinstancemode
      rc = pmdParsePreferInstModeStr( _prefInstModeStr, instanceMode ) ;
      if ( rc )
      {
         std::cerr << "Failed to parse preferred instance mode str, rc: "
                   << rc << endl ;
         goto error ;
      }

      if ( PMD_PREFER_INSTANCE_MODE_UNKNOWN == instanceMode )
      {
         std::cerr << PMD_OPTION_PREFINST_MODE << " value error, use default"
                   << endl ;
         ossStrncpy( _prefInstModeStr, PMD_DFT_PREFINST_MODE,
                     sizeof( _prefInstModeStr ) ) ;
         _invalidConfNum++ ;
      }

      rc = pmdParsePreferConstraintStr( _prefConstraint, constraint ) ;
      if ( rc )
      {
         std::cerr << "Failed to parse preferred constraint, rc: "
                   << rc << endl ;
         goto error ;
      }

      if ( PMD_PREFER_CONSTRAINT_UNKNOWN == constraint )
      {
         std::cerr << PMD_OPTION_PREFERRED_CONSTRAINT
                   << " value error, use default" << endl ;
         ossStrncpy( _prefConstraint, PMD_DFT_PREF_CONSTRAINT,
                     sizeof( _prefConstraint ) ) ;
         _invalidConfNum++ ;
      }

      if ( 0 == ossStrlen( _replServiceName ) )
      {
         string port = boost::lexical_cast<string>( _krcbSvcPort
                                                    + PMD_REPL_PORT ) ;
         ossMemcpy( _replServiceName, port.c_str(), port.size() ) ;
      }
      if ( 0 == ossStrlen( _shardServiceName ) )
      {
         string port = boost::lexical_cast<string>( _krcbSvcPort
                                                    + PMD_SHARD_PORT ) ;
         ossMemcpy( _shardServiceName, port.c_str(), port.size() ) ;
      }
      if ( 0 == ossStrlen( _catServiceName ) )
      {
         string port = boost::lexical_cast<string>( _krcbSvcPort
                                                    + PMD_CAT_PORT ) ;
         ossMemcpy( _catServiceName, port.c_str(), port.size() ) ;
      }
      if ( 0 == ossStrlen( _restServiceName ) )
      {
         string port = boost::lexical_cast<string>( _krcbSvcPort
                                                    + PMD_REST_PORT ) ;
         ossMemcpy( _restServiceName, port.c_str(), port.size() ) ;
      }
      if ( 0 == ossStrlen( _omServiceName ) )
      {
         string port = boost::lexical_cast<string>( _krcbSvcPort
                                                    + PMD_OM_PORT ) ;
         ossMemcpy( _omServiceName, port.c_str(), port.size() ) ;
      }

      if ( 0 == _krcbIndexPath[0] )
      {
         ossStrcpy( _krcbIndexPath, _krcbDbPath ) ;
      }
      if ( 0 == _krcbDiagLogPath[0] )
      {
         if ( SDB_OK != utilBuildFullPath( _krcbDbPath, PMD_OPTION_DIAG_PATH,
                                           OSS_MAX_PATHSIZE,
                                           _krcbDiagLogPath ) ||
              SDB_OK != utilCatPath( _krcbDiagLogPath, OSS_MAX_PATHSIZE, "" ) )
         {
            std::cerr << "diaglog path is too long!" << endl ;
            rc = SDB_INVALIDPATH ;
            goto error ;
         }
      }
      if ( 0 == _krcbAuditLogPath[0] )
      {
         if ( SDB_OK != utilBuildFullPath( _krcbDbPath, PMD_OPTION_AUDIT_PATH,
                                           OSS_MAX_PATHSIZE,
                                           _krcbAuditLogPath ) ||
              SDB_OK != utilCatPath( _krcbAuditLogPath, OSS_MAX_PATHSIZE, "" ) )
         {
            std::cerr << "auditlog path is too long!" << endl ;
            rc = SDB_INVALIDPATH ;
            goto error ;
         }
      }
      if ( 0 == _krcbLogPath[0] )
      {
         if ( SDB_OK != utilBuildFullPath( _krcbDbPath, PMD_OPTION_LOG_PATH,
                                           OSS_MAX_PATHSIZE, _krcbLogPath ) ||
              SDB_OK != utilCatPath( _krcbLogPath, OSS_MAX_PATHSIZE, "" ) )
         {
            std::cerr << "repicalog path is too long!" << endl ;
            rc = SDB_INVALIDPATH ;
            goto error ;
         }
      }
      if ( 0 == _krcbBkupPath[0] )
      {
         if ( SDB_OK != utilBuildFullPath( _krcbDbPath, PMD_OPTION_BK_PATH,
                                           OSS_MAX_PATHSIZE, _krcbBkupPath ) ||
              SDB_OK != utilCatPath( _krcbBkupPath, OSS_MAX_PATHSIZE, "" ) )
         {
            std::cerr << "bakup path is too long!" << endl ;
            rc = SDB_INVALIDPATH ;
            goto error ;
         }
      }
      if ( 0 == _krcbWWWPath[0] )
      {
         CHAR wwwPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
         if ( !_exePath.empty() )
         {
            rc = utilBuildFullPath( _exePath.c_str(),
                                    ".." OSS_FILE_SEP PMD_OPTION_WWW_PATH_DIR,
                                    OSS_MAX_PATHSIZE, wwwPath ) ;
         }
         else
         {
            rc = utilBuildFullPath( PMD_CURRENT_PATH,
                                    PMD_OPTION_WWW_PATH_DIR,
                                    OSS_MAX_PATHSIZE, wwwPath ) ;
         }
         if ( SDB_OK != rc )
         {
            std::cerr << "www path is too long!" << endl ;
            rc = SDB_INVALIDPATH ;
            goto error ;
         }

         // get real path
         if ( NULL == ossGetRealPath( wwwPath, _krcbWWWPath,
                                      OSS_MAX_PATHSIZE ) )
         {
            ossPrintf( "Error: Failed to get real path for %s\n",
                       wwwPath ) ;
            rc = SDB_INVALIDPATH ;
            goto error ;
         }
         utilCatPath( _krcbWWWPath, OSS_MAX_PATHSIZE, "" ) ;
      }

      if ( 0 == _archivePath[0] )
      {
         if ( SDB_OK != utilBuildFullPath( _krcbDbPath, PMD_OPTION_ARCHIVE_LOG_PATH,
                                           OSS_MAX_PATHSIZE, _archivePath ) ||
              SDB_OK != utilCatPath( _archivePath, OSS_MAX_PATHSIZE, "" ) )
         {
            std::cerr << "archivelog path is too long!" << endl ;
            rc = SDB_INVALIDPATH ;
            goto error ;
         }
      }

      if ( 0 == _krcbLobPath[0] )
      {
         ossStrcpy( _krcbLobPath, _krcbDbPath ) ;
      }
      if ( 0 == _krcbLobMetaPath[0] )
      {
         ossStrcpy( _krcbLobMetaPath, _krcbLobPath ) ;
      }

      if ( 0 == _dmsTmpBlkPath[0] )
      {
         if ( SDB_OK != utilBuildFullPath( _krcbDbPath, PMD_OPTION_TMPBLK_PATH,
                                           OSS_MAX_PATHSIZE, _dmsTmpBlkPath ) ||
              SDB_OK != utilCatPath( _dmsTmpBlkPath, OSS_MAX_PATHSIZE, "" ) )
         {
            std::cerr << "diaglog path is too long!" << endl ;
            rc = SDB_INVALIDPATH ;
            goto error ;
         }
      }

      /// Check the tmp path is valid. Can't be the prefix or the same with
      /// other direcotorys
      if ( 0 == ossStrncmp( _krcbDbPath, _dmsTmpBlkPath,
                            ossStrlen( _dmsTmpBlkPath ) ) )
      {
         std::cerr << "tmp path and data path should not be the same"
                   << endl ;
         rc = SDB_INVALIDPATH ;
         goto error ;
      }
      if ( 0 == ossStrncmp( _krcbIndexPath, _dmsTmpBlkPath,
                            ossStrlen( _dmsTmpBlkPath ) ) )
      {
         std::cerr << "tmp path and index path should not be the same"
                   << endl ;
         rc = SDB_INVALIDPATH ;
         goto error ;
      }
      if ( 0 == ossStrncmp( _krcbLogPath, _dmsTmpBlkPath,
                            ossStrlen( _dmsTmpBlkPath ) ) )
      {
         std::cerr << "tmp path and log path should not be the same"
                   << endl ;
         rc = SDB_INVALIDPATH ;
         goto error ;
      }
      if ( 0 == ossStrncmp( _krcbBkupPath, _dmsTmpBlkPath,
                            ossStrlen( _dmsTmpBlkPath ) ) )
      {
         std::cerr << "tmp path and bkup path should not be the same"
                   << endl ;
         rc = SDB_INVALIDPATH ;
         goto error ;
      }
      if ( 0 == ossStrncmp( _krcbDiagLogPath, _dmsTmpBlkPath,
                            ossStrlen( _dmsTmpBlkPath ) ) )
      {
         std::cerr << "tmp path and diaglog path should not be the same"
                   << endl ;
         rc = SDB_INVALIDPATH ;
         goto error ;
      }
      if ( 0 == ossStrncmp( _krcbAuditLogPath, _dmsTmpBlkPath,
                            ossStrlen( _dmsTmpBlkPath ) ) )
      {
         std::cerr << "tmp path and auditlog path should not be the same"
                   << endl ;
         rc = SDB_INVALIDPATH ;
         goto error ;
      }
      if ( 0 == ossStrncmp( _krcbWWWPath, _dmsTmpBlkPath,
                            ossStrlen( _dmsTmpBlkPath ) ) )
      {
         std::cerr << "tmp path and www path should not be the same"
                   << endl ;
         rc = SDB_INVALIDPATH ;
         goto error ;
      }
      if ( 0 == ossStrncmp( _krcbLobPath, _dmsTmpBlkPath,
                            ossStrlen( _dmsTmpBlkPath ) ) )
      {
         std::cerr << "tmp path and lob path should not be the same"
                   << endl ;
         rc = SDB_INVALIDPATH ;
         goto error ;
      }
      if ( 0 == ossStrncmp( _krcbLobMetaPath, _dmsTmpBlkPath,
                            ossStrlen( _dmsTmpBlkPath ) ) )
      {
         std::cerr << "tmp path and lob meta path should not be the same"
                   << endl ;
         rc = SDB_INVALIDPATH ;
         goto error ;
      }
      if ( 0 == ossStrncmp( _archivePath, _dmsTmpBlkPath,
                            ossStrlen( _dmsTmpBlkPath ) ) )
      {
         std::cerr << "tmp path and archive path should not be the same"
                   << endl ;
         rc = SDB_INVALIDPATH ;
         goto error ;
      }

      if ( _archiveOn )
      {
         PD_CHECK( _logFileNum > 1, SDB_INVALIDARG, error, PDERROR,
                   "The value of parameter \"logfilenum\" must be greater than 1 "
                   "when configure archiveon" ) ;
      }

      // Enable transaction always for Catalog
      if ( SDB_ROLE_CATALOG == dbRole || SDB_ROLE_OM == dbRole )
      {
         _transactionOn = TRUE ;
         _transAllowLockEscalation = TRUE ;
         _transMaxLockNum = -1 ;
         _transMaxLogSpaceRatio = DPS_TRANS_MAXLOGSPACERATIO_MAX ;
      }

      if ( _transactionOn )
      {
         PD_CHECK( _logFileNum > 1, SDB_INVALIDARG, error, PDERROR,
                   "The value of parameter \"logfilenum\" must be greater than 1 "
                   "when configure transaction" ) ;
      }

      if ( _memDebugSize != 0 )
      {
         _memDebugSize = OSS_MIN ( _memDebugSize, SDB_MEMDEBUG_MAXGUARDSIZE ) ;
         _memDebugSize = OSS_MAX ( _memDebugSize, SDB_MEMDEBUG_MINGUARDSIZE ) ;
      }

      _memDebugMask = 0 ;
      if ( !ossString2MemDebugMask( _memDebugMaskStr, _memDebugMask ) )
      {
         _memDebugMask = OSS_MEMDEBUG_MASK_DFT ;
         ossStrncpy( _memDebugMaskStr, OSS_MEMDEBUG_MASK_DFT_STR,
                     PMD_MAX_LONG_STR_LEN ) ;
         _invalidConfNum++ ;
      }

      if ( 0 == _vecCat.size() )
      {
         rc = parseAddressLine( _catAddrLine, _vecCat ) ;
         if ( rc )
         {
            goto error ;
         }
      }

      // if start is stanalone, must enable dps local
      if ( SDB_ROLE_STANDALONE == dbRole &&
           ( _transactionOn || _archiveOn ) )
      {
         _dpslocal = TRUE ;
      }

      // om and catalog, prefetch and preload and multi-replsync not enable
      // and syncstrategy is none, and enablemixcmp should not be enabled
      if ( SDB_ROLE_CATALOG == dbRole || SDB_ROLE_OM == dbRole )
      {
         _numPreLoaders    = 0 ;
         _maxPrefPool      = 0 ;
         _maxSubQuery      = 0 ;
         _maxReplSync      = 0 ;
         _syncStrategy     = CLS_SYNC_NONE ;
         _recycleRecord    = TRUE ;

         if ( 0 == _syncInterval || _syncInterval > 60000 )
         {
            _syncInterval = PMD_DFT_SYNC_INTERVAL ;
         }
         if ( 0 == _syncRecordNum || _syncRecordNum > 1000 )
         {
            _syncRecordNum = 10 ;
         }

         _enableMixCmp = FALSE ;
      }

      if ( 0 == _vecOm.size() )
      {
         rc = parseAddressLine( _omAddrLine, _vecOm ) ;
         if ( rc )
         {
            goto error ;
         }
      }

      if ( SCHED_TYPE_NONE == _svcSchedulerType )
      {
         _svcMaxConcurrency = 0 ;
      }

      if ( _maxContextNum > 0 &&
           _maxContextNum < RTN_MAX_CTX_NUM_MIN )
      {
         // avoid the value is too small
         _maxContextNum = RTN_MAX_CTX_NUM_MIN ;
      }

      if ( _maxSessionContextNum > 0 &&
           _maxSessionContextNum < RTN_MAX_SESS_CTX_NUM_MIN )
      {
         // avoid the value is too small
         _maxSessionContextNum = RTN_MAX_SESS_CTX_NUM_MIN ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   string _pmdOptionsMgr::getCatAddr() const
   {
      return makeAddressLine( _vecCat ) ;
   }

   string _pmdOptionsMgr::getOmAddr() const
   {
      return makeAddressLine( _vecOm ) ;
   }

   INT32 _pmdOptionsMgr::preSaving ()
   {
      MAP_K2V::iterator it ;

      string addr = makeAddressLine( _vecCat ) ;
      ossStrncpy( _catAddrLine, addr.c_str(), OSS_MAX_PATHSIZE ) ;
      _catAddrLine[ OSS_MAX_PATHSIZE ] = 0 ;
      /// make sure hasField
      if ( 0 != _catAddrLine[ 0 ] )
      {
         _addToFieldMap( PMD_OPTION_CATALOG_ADDR, _catAddrLine, TRUE, TRUE ) ;
      }

      addr = makeAddressLine( _vecOm ) ;
      ossStrncpy( _omAddrLine, addr.c_str(), OSS_MAX_PATHSIZE ) ;
      _omAddrLine[ OSS_MAX_PATHSIZE ] = 0 ;
      /// make sure PMD_OPTION_OM_ADDR hasField
      if ( 0 != _omAddrLine[ 0 ] )
      {
         _addToFieldMap( PMD_OPTION_OM_ADDR, _omAddrLine, TRUE, TRUE ) ;
      }

      clsStrategy2String( _syncStrategy, _syncStrategyStr,
                          sizeof( _syncStrategyStr ) ) ;

      optLogMod2String( _logWriteMod, _logWriteModStr,
                        sizeof( _logWriteModStr ) ) ;

      return SDB_OK ;
   }

   INT32 _pmdOptionsMgr::initFromFile( const CHAR * pConfigFile,
                                       BOOLEAN allowFileNotExist )
   {
      INT32 rc = SDB_OK ;
      po::options_description desc ( "Command options" ) ;
      po::variables_map vm ;

      PMD_ADD_PARAM_OPTIONS_BEGIN( desc )
         PMD_COMMANDS_OPTIONS
         PMD_HIDDEN_COMMANDS_OPTIONS
         ( "*", po::value<string>(), "" )
      PMD_ADD_PARAM_OPTIONS_END

      rc = utilReadConfigureFile( pConfigFile, desc, vm ) ;
      if ( rc )
      {
         if ( SDB_FNE != rc || !allowFileNotExist )
         {
            PD_LOG( PDERROR, "Failed to read config from file[%s], rc: %d",
                    pConfigFile, rc ) ;
            goto error ;
         }
      }

      rc = pmdCfgRecord::init( &vm, NULL ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init config record failed, rc: %d", rc ) ;
         goto error ;
      }
      ossStrncpy( _krcbConfFile, pConfigFile, OSS_MAX_PATHSIZE ) ;
      if ( ossStrrchr( pConfigFile, OSS_FILE_SEP_CHAR ) )
      {
         const CHAR *pLastSep = ossStrrchr( pConfigFile, OSS_FILE_SEP_CHAR ) ;
         ossStrncpy( _krcbConfPath, pConfigFile,
                     OSS_MAX_PATHSIZE <= pLastSep - pConfigFile ?
                     OSS_MAX_PATHSIZE : pLastSep - pConfigFile ) ;
      }
      else
      {
         ossStrcpy( _krcbConfPath, PMD_CURRENT_PATH ) ;
      }
      /// update conf path
      _addToFieldMap( PMD_OPTION_CONFPATH, _krcbConfPath, TRUE, FALSE ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDOPTMGR_INIT, "_pmdOptionsMgr::init" )
   INT32 _pmdOptionsMgr::init( INT32 argc, CHAR **argv,
                               const std::string &exePath )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB__PMDOPTMGR_INIT ) ;

      CHAR cfgTempPath[ OSS_MAX_PATHSIZE + 1 ] = {0} ;
      po::options_description all ( "Command options" ) ;
      po::options_description display ( "Command options (display)" ) ;
      po::variables_map vm ;
      po::variables_map vm2 ;

      PMD_ADD_PARAM_OPTIONS_BEGIN( all )
         PMD_COMMANDS_OPTIONS
         PMD_HIDDEN_COMMANDS_OPTIONS
         ( PMD_OPTION_HELPFULL, "help all configs" ) \
         ( "*", po::value<string>(), "" )
      PMD_ADD_PARAM_OPTIONS_END

      PMD_ADD_PARAM_OPTIONS_BEGIN( display )
         PMD_COMMANDS_OPTIONS
      PMD_ADD_PARAM_OPTIONS_END

      _exePath = exePath ;

      rc = utilReadCommandLine( argc, argv, all, vm );
      JUDGE_RC( rc )

      /// read cmd first
      if ( vm.empty() )
      {
         std::cout << "sequoiadb: missing arguments" << std::endl ;
         std::cout << "Try 'sequoiadb --help' for more information." << std::endl ;
         rc = SDB_INVALIDARG ;
         utilRC2ShellRC ( rc ) ;
         goto error ;
      }

      if ( vm.count( PMD_OPTION_HELP ) )
      {
         std::cout << display << std::endl ;
         rc = SDB_PMD_HELP_ONLY ;
         goto done ;
      }
      if ( vm.count( PMD_OPTION_HELPFULL ) )
      {
         std::cout << all << std::endl ;
         rc = SDB_PMD_HELP_ONLY ;
         goto done ;
      }
      if ( vm.count( PMD_OPTION_VERSION ) )
      {
         ossPrintVersion( "SequoiaDB version" ) ;
         rc = SDB_PMD_VERSION_ONLY ;
         goto done ;
      }

      // --confpath
      if ( vm.count( PMD_OPTION_CONFPATH ) )
      {
         CHAR *p = ossGetRealPath( vm[PMD_OPTION_CONFPATH].as<string>().c_str(),
                                   cfgTempPath, OSS_MAX_PATHSIZE ) ;
         if ( NULL == p )
         {
            std::cerr << "ERROR: Failed to get real path for "
                      <<  vm[PMD_OPTION_CONFPATH].as<string>().c_str() << endl ;
            rc = SDB_INVALIDPATH ;
            goto error;
         }
      }
      else
      {
         CHAR *p = ossGetRealPath( PMD_CURRENT_PATH, cfgTempPath,
                                   OSS_MAX_PATHSIZE ) ;
         if ( NULL == p )
         {
            SDB_ASSERT( FALSE, "impossible" ) ;
            rc = SDB_INVALIDPATH ;
            goto error ;
         }
      }

      rc = utilBuildFullPath( cfgTempPath, PMD_DFT_CONF,
                              OSS_MAX_PATHSIZE, _krcbConfFile ) ;
      if ( rc )
      {
         std::cerr << "ERROR: Failed to make config file name: " << rc << endl ;
         goto error ;
      }
      rc = utilBuildFullPath( cfgTempPath, PMD_DFT_CAT,
                              OSS_MAX_PATHSIZE, _krcbCatFile ) ;
      if ( rc )
      {
         std::cerr << "ERROR: Failed to make cat file name: " << rc << endl ;
         goto error ;
      }

      rc = utilReadConfigureFile( _krcbConfFile, all, vm2 ) ;
      if ( SDB_OK != rc )
      {
         //if user set configure file,  but cann't read the configure file.
         //then we should exit.
         if ( vm.count( PMD_OPTION_CONFPATH ) )
         {
            std::cerr << "Read config file: " << _krcbConfFile
                      << " failed, rc: " << rc << std::endl ;
            goto error ;
         }
         // we should avoid exit when config file is not found or I/O error
         // we should continue run but use other arguments that uses input
         // from command line
         else if ( SDB_FNE != rc )
         {
            std::cerr << "Read default config file: " << _krcbConfFile
                      << " failed, rc: " << rc << std::endl ;
            goto error ;
         }
         else
         {
            std::cout << "Using default config" << std::endl ;
            rc = SDB_OK ;
         }
      }

      rc = pmdCfgRecord::init( &vm2, &vm ) ;
      if ( rc )
      {
         goto error ;
      }
      else
      {
         ossStrcpy( _krcbConfPath, cfgTempPath ) ;
         /// update conf path
         _addToFieldMap( PMD_OPTION_CONFPATH, _krcbConfPath, TRUE, FALSE ) ;
      }

      if( 0 != _invalidConfNum || hasAutoAdjust() )
      {
         rc = reflush2File() ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Reflush config to file failed, rc: %d", rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB__PMDOPTMGR_INIT, rc );
      return rc ;
   error:
      goto done ;
   }

   #define PMD_RMDIR_WITH_IGNORE_PARENTDIR_PERM( path )\
   {\
      if ( 0 != path[0] && 0 == ossAccess( path ) )\
      {\
         rc = ossDelete( path ) ;\
         if ( SDB_PERM == rc )\
         {\
            multimap<string, string> mapFiles ;\
            vector<string> dirs;\
            string tmpPath = path ;\
            rc = ossEnumFiles( tmpPath, mapFiles, NULL, 1 ) ;\
            if( rc || !mapFiles.empty() )\
            {\
               rc = SDB_PERM ;\
               PD_LOG( PDERROR, "Remove dir[%s] failed, rc: %d",\
                       path, SDB_PERM ) ;\
               goto error ;\
            }\
            rc = ossEnumSubDirs( tmpPath, dirs, 1 ) ;\
            if( rc || !dirs.empty() )\
            {\
               rc = SDB_PERM ;\
               PD_LOG( PDERROR, "Remove dir[%s] failed, rc: %d",\
                       path, SDB_PERM ) ;\
               goto error ;\
            }\
         }\
         else if ( rc )\
         {\
            PD_LOG( PDERROR, "Remove dir[%s] failed, rc: %d",\
                    path, rc ) ;\
            goto error ;\
         }\
      }\
   }

   INT32 _pmdOptionsMgr::removeAllDir()
   {
      INT32 rc = SDB_OK ;

      PMD_RMDIR_WITH_IGNORE_PARENTDIR_PERM( _archivePath ) ;

      PMD_RMDIR_WITH_IGNORE_PARENTDIR_PERM( _dmsTmpBlkPath ) ;

      PMD_RMDIR_WITH_IGNORE_PARENTDIR_PERM( _krcbBkupPath ) ;

      PMD_RMDIR_WITH_IGNORE_PARENTDIR_PERM( _krcbLogPath ) ;

      PMD_RMDIR_WITH_IGNORE_PARENTDIR_PERM( _krcbDiagLogPath ) ;

      PMD_RMDIR_WITH_IGNORE_PARENTDIR_PERM( _krcbAuditLogPath ) ;

      PMD_RMDIR_WITH_IGNORE_PARENTDIR_PERM( _krcbLobPath ) ;

      PMD_RMDIR_WITH_IGNORE_PARENTDIR_PERM( _krcbLobMetaPath ) ;

      PMD_RMDIR_WITH_IGNORE_PARENTDIR_PERM( _krcbIndexPath ) ;

      PMD_RMDIR_WITH_IGNORE_PARENTDIR_PERM( _krcbDbPath ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDOPTMGR__MKDIR, "_pmdOptionsMgr::makeAllDir" )
   INT32 _pmdOptionsMgr::makeAllDir()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__PMDOPTMGR__MKDIR );

      rc = ossMkdir( _krcbDbPath ) ;
      if ( rc && SDB_FE != rc )
      {
         std::cerr << "Failed to create database dir: " << _krcbDbPath <<
                      ", rc = " << rc << std::endl ;
         goto error ;
      }
      rc = ossMkdir( _krcbIndexPath ) ;
      if ( rc && SDB_FE != rc )
      {
         std::cerr << "Failed to create index dir: " << _krcbIndexPath <<
                      ", rc = " << rc << std::endl ;
         goto error ;
      }
      rc = ossMkdir( _krcbDiagLogPath ) ;
      if ( rc && SDB_FE != rc )
      {
         std::cerr << "Failed to create diaglog dir: " << _krcbDiagLogPath <<
                      ", rc = " << rc << std::endl ;
         goto error ;
      }
      rc = ossMkdir( _krcbAuditLogPath ) ;
      if ( rc && SDB_FE != rc )
      {
         std::cerr << "Failed to create auditlog dir: " << _krcbAuditLogPath <<
                      ", rc = " << rc << std::endl ;
         goto error ;
      }
      rc = ossMkdir( _krcbLogPath ) ;
      if ( rc && SDB_FE != rc )
      {
         std::cerr << "Failed to create repl-log dir: " << _krcbLogPath <<
                      ", rc = " << rc << std::endl ;
         goto error ;
      }

      rc = ossMkdir( _krcbBkupPath ) ;
      if ( rc && SDB_FE != rc )
      {
         std::cerr << "Failed to create backup dir: " << _krcbBkupPath <<
                      ", rc = " << rc << std::endl ;
         goto error ;
      }

      rc = ossMkdir( _dmsTmpBlkPath ) ;
      if ( rc && SDB_FE != rc )
      {
         std::cerr << "Failed to create tmp dir: " << _dmsTmpBlkPath <<
                      ", rc = " << rc << std::endl ;
         goto error ;
      }

      rc = ossMkdir( _archivePath ) ;
      if ( rc && SDB_FE != rc )
      {
         std::cerr << "Failed to create archivelog dir: " << _archivePath <<
                      ", rc = " << rc << std::endl ;
         goto error ;
      }

      rc = ossMkdir( _krcbLobPath ) ;
      if ( rc && SDB_FE != rc )
      {
         std::cerr << "Failed to create lob dir: " << _krcbLobPath <<
                      ", rc = " << rc << std::endl ;
         goto error ;
      }

      rc = ossMkdir( _krcbLobMetaPath ) ;
      if ( rc && SDB_FE != rc )
      {
         std::cerr << "Failed to create lob meta dir: " << _krcbLobMetaPath <<
                      ", rc = " << rc << std::endl ;
         goto error ;
      }

      rc = SDB_OK ;
   done:
      PD_TRACE_EXITRC ( SDB__PMDOPTMGR__MKDIR, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDOPTMGR_REFLUSH2FILE, "_pmdOptionsMgr::reflush2File" )
   INT32 _pmdOptionsMgr::reflush2File( UINT32 mask )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY (SDB__PMDOPTMGR_REFLUSH2FILE ) ;
      std::string line ;
      CHAR conf[ OSS_MAX_PATHSIZE + 1 ] = {0} ;

      rc = pmdCfgRecord::toString( line, mask ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get the line str, rc: %d", rc ) ;
         goto error ;
      }

      rc = utilBuildFullPath( _krcbConfPath, PMD_DFT_CONF,
                              OSS_MAX_PATHSIZE, conf ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to build full path of configure file, "
                 "rc: %d", rc ) ;
         goto error;
      }

      {
         ossScopedLock lock( &_mutex ) ;

         rc = utilWriteConfigFile( conf, line.c_str(), FALSE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to write config[%s], rc: %d",
                      conf, rc ) ;

         // save notify
         if ( getConfigHandler() )
         {
            getConfigHandler()->onConfigSave() ;
         }
      }

   done:
      PD_TRACE_EXIT ( SDB__PMDOPTMGR_REFLUSH2FILE) ;
      return rc ;
   error:
      goto done ;
   }

   void _pmdOptionsMgr::clearCatAddr()
   {
      _vecCat.clear() ;
   }

   void _pmdOptionsMgr::rmCatAddrItem( const CHAR *host,
                                       const CHAR *service )
   {
      vector< pmdAddrPair >::iterator it = _vecCat.begin() ;
      while( it != _vecCat.end() )
      {
         pmdAddrPair &item = *it ;
         if ( 0 == ossStrcmp( item._host, host ) &&
              0 == ossStrcmp( item._service, service ) )
         {
            _vecCat.erase( it ) ;
            break ;
         }
         ++it ;
      }
   }

   void _pmdOptionsMgr::setCatAddr( const CHAR *host,
                                    const CHAR *service )
   {
      BOOLEAN hasSet = FALSE ;

      for ( UINT32 i = 0 ; i < _vecCat.size() ; ++i )
      {
         if ( '\0' == _vecCat[i]._host[0] )
         {
            ossStrncpy( _vecCat[i]._host, host, OSS_MAX_HOSTNAME ) ;
            _vecCat[i]._host[ OSS_MAX_HOSTNAME ] = 0 ;
            ossStrncpy( _vecCat[i]._service, service, OSS_MAX_SERVICENAME ) ;
            _vecCat[i]._service[ OSS_MAX_SERVICENAME ] = 0 ;
            hasSet = TRUE ;
            break ;
         }
      }

      if ( !hasSet && _vecCat.size() < CATA_NODE_MAX_NUM )
      {
         pmdAddrPair addr ;
         ossStrncpy( addr._host, host, OSS_MAX_HOSTNAME ) ;
         addr._host[ OSS_MAX_HOSTNAME ] = 0 ;
         ossStrncpy( addr._service, service, OSS_MAX_SERVICENAME ) ;
         addr._service[ OSS_MAX_SERVICENAME ] = 0 ;
         _vecCat.push_back( addr ) ;
      }
   }

   void _pmdOptionsMgr::modifyPreferInstStr( ossPoolList< UINT8 > instanceList,
                                             PMD_PREFER_INSTANCE_TYPE specInstance )
   {
      stringstream ss ;
      if( specInstance != PMD_PREFER_INSTANCE_TYPE_UNKNOWN )
      {
         ss << pmdPreferInstInt2String( specInstance ) ;
      }

      if( !instanceList.empty() )
      {
         if( ss.str() != "" )
         {
            ss << "," ;
         }

         for ( ossPoolList< UINT8 >::const_iterator iter = instanceList.begin() ;
               iter != instanceList.end() ;
               iter ++ )
         {
            if( iter != instanceList.begin() )
            {
               ss << "," ;
            }
            ss << ( UINT32 )( *iter ) ;
         }
      }
      ossStrncpy( _prefInstStr, ss.str().c_str(), sizeof( _prefInstStr ) ) ;
   }

   INT32 optString2LogMod( const CHAR *str, UINT32 &value )
   {
      INT32 rc = SDB_OK ;
      if ( !str || !*str )
      {
         value = DPS_LOG_WRITE_MOD_INCREMENT ;
      }
      else
      {
         UINT32 len = ossStrlen( str ) ;
         if ( 0 == ossStrncasecmp( str, PMD_OPTION_LOG_WRITEMOD_INCREMENT_STR, len ) &&
              len == ossStrlen( PMD_OPTION_LOG_WRITEMOD_INCREMENT_STR ) )
         {
            value = DPS_LOG_WRITE_MOD_INCREMENT ;
         }
         else if ( 0 == ossStrncasecmp( str, PMD_OPTION_LOG_WRITEMOD_FULL_STR, len ) &&
                   len == ossStrlen( PMD_OPTION_LOG_WRITEMOD_FULL_STR ) )
         {
            value = DPS_LOG_WRITE_MOD_FULL ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
         }
      }

      return rc ;
   }

   INT32 optString2MonGroupMask( const CHAR *str, UINT32 &value )
   {
      INT32 rc = SDB_OK ;
      if ( !str || !*str )
      {
         value = MON_GROUP_MASK_DEFAULT ;
      }
      else
      {
         const CHAR *p = str ;
         CHAR *p1 = NULL ;
         UINT32 theMask = 0 ;

         while( p && *p )
         {
            p1 = (CHAR*)ossStrchr( p, '|' ) ;

            CHAR *token = NULL ;

            token = (CHAR*)ossStrchr( p, ':' ) ;

            if ( !token )
            {
               break ;
            }
            const CHAR *keyStart = p ;

            while( ' ' == *keyStart )
            {
               ++keyStart ;
            }

            if ( !*keyStart )
            {
               break ;
            }

            const CHAR *keyEnd = keyStart + ( token - keyStart ) ;

            while( keyEnd != keyStart && ' ' == *keyEnd )
            {
               --keyEnd ;
            }
            UINT32 keyLen = keyEnd - keyStart ;

            const CHAR *valueStart = token + 1 ;

            if ( !*valueStart )
            {
               break ;
            }
            while( ' ' == *valueStart )
            {
               ++valueStart ;
            }

            const CHAR *valueEnd = p1? p1: valueStart + ossStrlen( valueStart ) - 1 ;
            while( valueEnd != valueStart && ' ' == *valueEnd )
            {
               --valueEnd ;
            }
            UINT32 valueLen = valueEnd - valueStart + 1 ;
            /// compare
            if ( 0 == ossStrncasecmp( keyStart, "slowquery", keyLen ) )
            {
               if ( 0 == ossStrncasecmp( valueStart, "basic", valueLen ) )
               {
                  theMask |= MON_GROUP_QUERY_BASIC ;
                  theMask |= MON_GROUP_LATCH_BASIC ;
                  theMask |= MON_GROUP_LOCK_BASIC ;
               }
               else if ( 0 == ossStrncasecmp( valueStart, "detail", valueLen ) )
               {
                  theMask |= MON_GROUP_QUERY_DETAIL ;
                  theMask |= MON_GROUP_LATCH_DETAIL ;
                  theMask |= MON_GROUP_LOCK_DETAIL ;
               }
            }
            else if ( 0 == ossStrncasecmp( keyStart, "all", keyLen ) )
            {
               // this is the same as 'slowquery' right now because there is only one group
               if ( 0 == ossStrncasecmp( valueStart, "basic", valueLen ) )
               {
                  theMask |= MON_GROUP_QUERY_BASIC ;
                  theMask |= MON_GROUP_LATCH_BASIC ;
                  theMask |= MON_GROUP_LOCK_BASIC ;
               }
               else if ( 0 == ossStrncasecmp( valueStart, "detail", valueLen ) )
               {
                  theMask |= MON_GROUP_QUERY_DETAIL ;
                  theMask |= MON_GROUP_LATCH_DETAIL ;
                  theMask |= MON_GROUP_LOCK_DETAIL ;
               }
            }
            else if ( 0 == ossStrncasecmp( keyStart, "latch", keyLen ) )
            {
               if ( 0 == ossStrncasecmp( valueStart, "basic", valueLen ) )
               {
                  theMask |= MON_GROUP_LATCH_BASIC ;
               }
               else if ( 0 == ossStrncasecmp( valueStart, "detail", valueLen ) )
               {
                  theMask |= MON_GROUP_LATCH_DETAIL ;
               }
            }
            else if ( 0 == ossStrncasecmp( keyStart, "lock", keyLen ) )
            {
               if ( 0 == ossStrncasecmp( valueStart, "basic", valueLen ) )
               {
                  theMask |= MON_GROUP_LOCK_BASIC ;
               }
               else if ( 0 == ossStrncasecmp( valueStart, "detail", valueLen ) )
               {
                  theMask |= MON_GROUP_LOCK_DETAIL ;
               }
            }
            else if ( 0 == ossStrncasecmp( keyStart, "query", keyLen ) )
            {
               if ( 0 == ossStrncasecmp( valueStart, "basic", valueLen ) )
               {
                  theMask |= MON_GROUP_QUERY_BASIC ;
               }
               else if ( 0 == ossStrncasecmp( valueStart, "detail", valueLen ) )
               {
                  theMask |= MON_GROUP_QUERY_DETAIL ;
               }
            }
            value |= theMask ;

            p = p1? p1 + 1: NULL ;
         }
      }
      return rc ;
   }
   INT32 optLogMod2String( UINT32 value, CHAR *str, INT32 len )
   {
      INT32 rc = SDB_OK ;

      switch ( value )
      {
         case DPS_LOG_WRITE_MOD_INCREMENT :
            ossStrncpy( str, PMD_OPTION_LOG_WRITEMOD_INCREMENT_STR, len - 1 ) ;
            break ;

         case DPS_LOG_WRITE_MOD_FULL :
            ossStrncpy( str, PMD_OPTION_LOG_WRITEMOD_FULL_STR, len -1 ) ;
            break ;

         default :
            str[0] = 0 ;
            rc = SDB_INVALIDARG ;
      }
      str[ len -1 ] = 0 ;
      return rc ;
   }

}


