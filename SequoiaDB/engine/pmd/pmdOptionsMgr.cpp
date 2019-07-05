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

#include "rtnSortDef.hpp"
#include "clsUtil.hpp"

#include <vector>
#include <boost/algorithm/string.hpp>

using namespace bson ;

namespace engine
{

   #define JUDGE_RC( rc ) if ( SDB_OK != rc ) { goto error ; }

   #define PMD_OPTION_BRK_TIME_DEFAULT (7000)
   #define PMD_OPTION_OPR_TIME_DEFAULT (300000)
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
   #define PMD_DFT_TRANS_TIMEOUT       (60)  // 1 minute
   #define PMD_DFT_OVERFLOW_RETIO      (12)
   #define PMD_DFT_EXTEND_THRESHOLD    (32)
   #define PMD_DFT_MAX_CACHE_JOB       (10)
   #define PMD_DFT_MAX_SYNC_JOB        (10)
   #define PMD_DFT_SYNC_INTERVAL       (10000)  // 10 seconds
   #define PMD_DFT_SYNC_RECORDNUM      (0)
   #define PMD_DFT_ARCHIVE_TIMEOUT     (600) // 10 minutes
   #define PMD_DFT_ARCHIVE_EXPIRED     (240) // 10 days
   #define PMD_DFT_ARCHIVE_QUOTA       (10)  // 10 GB
   #define PMD_DFT_DMS_CHK_INTERVAL    (0)   // disable
   #define PMD_DFT_CACHE_MERGE_SZ      (0)   // ms
   #define PMD_DFT_PAGE_ALLOC_TIMEOUT  (0)
   #define PMD_DFT_OPT_COST_THRESHOLD  (20)
   #define PMD_DFT_ENABLE_MIX_CMP      (FALSE)
   #define PMD_DFT_PREFINST            ( PREFER_INSTANCE_MASTER_STR )
   #define PMD_DFT_PREFINST_MODE       ( PREFER_INSTANCE_RANDOM_STR )
   #define PMD_DFT_INSTANCE_ID         ( NODE_INSTANCE_ID_UNKNOWN )
   #define PMD_DFT_MAX_CONN            (0)   // unlimited
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

      SDB_ASSERT( _pMapKeyField, "Map key field can't be NULL" ) ;
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
      BOOLEAN fieldStatus = TRUE ;

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
         if ( value == readValue )
         {
            fieldStatus = FALSE ;
         }
         if ( PMD_CFG_STEP_PRECHG == _cfgStep )
         {
            (*_pMapKeyField)[ pFieldName ] = pmdParamValue( readValue, FALSE,
                                                            FALSE, changeLevel ) ;
         }
         else if ( PMD_CFG_STEP_CHG == _cfgStep )
         {
            if ( PMD_CFG_CHANGE_REBOOT == changeLevel )
            {
               (*_pMapKeyField)[ pFieldName ] = pmdParamValue( readValue, FALSE,
                                                               fieldStatus,
                                                               changeLevel ) ;
            }
            else if ( PMD_CFG_CHANGE_RUN == changeLevel )
            {
               value = readValue ;
               (*_pMapKeyField)[ pFieldName ] = pmdParamValue( readValue, TRUE,
                                                               fieldStatus,
                                                               changeLevel ) ;
            }
            else if ( PMD_CFG_CHANGE_FORBIDDEN == changeLevel )
            {
               MAP_K2V::iterator it = _pMapKeyField->find( pFieldName ) ;
               if ( _pMapKeyField->end() != it )
               {
                  it->second._hasField = FALSE ;
                  it->second._hasMapped = fieldStatus ;
               }
               else
               {
                  (*_pMapKeyField)[ pFieldName ] = pmdParamValue( readValue,
                                                                  fieldStatus,
                                                                  TRUE, 
                                                                  changeLevel ) ;
               }
            }
         }
         else
         {
            value = readValue ;
            (*_pMapKeyField)[ pFieldName ] = pmdParamValue( readValue, TRUE,
                                                            TRUE, changeLevel ) ;
         }
      }

      return rc ;
   }

   INT32 _pmdCfgExchange::readInt( const CHAR * pFieldName, INT32 &value,
                                   INT32 defaultValue, PMD_CFG_CHANGE changeLevel )
   {
      INT32 rc = SDB_OK ;

      if ( PMD_CFG_STEP_RESTORE == _cfgStep )
      {
         BOOLEAN fieldStatus = TRUE ;

         if ( hasField( pFieldName ) )
         {
            if ( value == defaultValue )
            {
               fieldStatus = FALSE ;
            }

            if ( PMD_CFG_CHANGE_REBOOT == changeLevel )
            {
               (*_pMapKeyField)[ pFieldName ] = pmdParamValue( defaultValue, FALSE,
                                                               fieldStatus,
                                                               changeLevel ) ;
            }
            else if ( PMD_CFG_CHANGE_RUN == changeLevel )
            {
               value = defaultValue ;
               (*_pMapKeyField)[ pFieldName ] = pmdParamValue( defaultValue, TRUE,
                                                               fieldStatus,
                                                               changeLevel ) ;
            }
            else if ( PMD_CFG_CHANGE_FORBIDDEN == changeLevel )
            {
               MAP_K2V::iterator it = _pMapKeyField->find( pFieldName ) ;
               if ( _pMapKeyField->end() != it )
               {
                  it->second._hasMapped = TRUE ;
                  it->second._hasField = fieldStatus ;
               }
               else
               {
                  (*_pMapKeyField)[ pFieldName ] = pmdParamValue( defaultValue, FALSE,
                                                                  fieldStatus, 
                                                                  changeLevel ) ;
               }
            }
         }
         goto done ;
      }

      rc = readInt( pFieldName, value, changeLevel ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         if ( PMD_CFG_STEP_PRECHG == _cfgStep ||
              ( PMD_CFG_STEP_CHG == _cfgStep &&
                PMD_CFG_CHANGE_FORBIDDEN == changeLevel ) )
         {
            rc = SDB_OK ;
            goto done ;
         }
         value = defaultValue ;
         rc = SDB_OK ;
         (*_pMapKeyField)[ pFieldName ] = pmdParamValue( value, TRUE, FALSE) ;
      }
done:
      return rc ;
   }

   INT32 _pmdCfgExchange::readString( const CHAR *pFieldName, CHAR *pValue,
                                      UINT32 len, PMD_CFG_CHANGE changeLevel )
   {
      INT32 rc = SDB_OK ;
      string readValue ;
      BOOLEAN fieldStatus = TRUE ;

      if ( PMD_CFG_DATA_BSON == _dataType )
      {
         BSONElement ele = _dataObj.getField( pFieldName ) ;
         if ( ele.eoo() )
         {
            rc = SDB_FIELD_NOT_EXIST ;
         }
         else if ( String != ele.type() )
         {
            PD_LOG( PDERROR, "Field[%s] type[%d] is not string", pFieldName,
                    ele.type() ) ;
            rc = SDB_INVALIDARG ;
         }
         else
         {
            readValue = ele.String() ;
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
         if ( 0 == ossStrcasecmp( pValue, readValue.c_str() ) )
         {
            fieldStatus = FALSE ;
         }
         if ( PMD_CFG_STEP_PRECHG == _cfgStep )
         {
            (*_pMapKeyField)[ pFieldName ] = pmdParamValue( readValue, FALSE,
                                                            FALSE, changeLevel ) ;
         }
         else if ( PMD_CFG_STEP_CHG == _cfgStep )
         {
            if ( PMD_CFG_CHANGE_REBOOT == changeLevel )
            {
               (*_pMapKeyField)[ pFieldName ] = pmdParamValue( readValue, FALSE,
                                                               fieldStatus,
                                                               changeLevel ) ;
            }
            else if ( PMD_CFG_CHANGE_RUN == changeLevel )
            {
               ossStrncpy( pValue, readValue.c_str(), len ) ;
               pValue[ len - 1 ] = 0 ;
               (*_pMapKeyField)[ pFieldName ] = pmdParamValue( readValue, TRUE,
                                                               fieldStatus,
                                                               changeLevel ) ;
            }
            else if ( PMD_CFG_CHANGE_FORBIDDEN == changeLevel )
            {
               MAP_K2V::iterator it = _pMapKeyField->find( pFieldName ) ;
               if ( _pMapKeyField->end() != it )
               {
                  it->second._hasField = FALSE ;
                  it->second._hasMapped = fieldStatus ;
               }
               else
               {
                  (*_pMapKeyField)[ pFieldName ] = pmdParamValue( readValue,
                                                                  fieldStatus,
                                                                  TRUE, 
                                                                  changeLevel ) ;
               }
            }
         }
         else
         {
            ossStrncpy( pValue, readValue.c_str(), len ) ;
            pValue[ len - 1 ] = 0 ;
            (*_pMapKeyField)[ pFieldName ] = pmdParamValue( readValue.c_str(), TRUE,
                                                            TRUE, changeLevel ) ;
         }
      }

      return rc ;
   }

   INT32 _pmdCfgExchange::readString( const CHAR *pFieldName,
                                      CHAR *pValue, UINT32 len,
                                      const CHAR *pDefault,
                                      PMD_CFG_CHANGE changeLevel )
   {
      INT32 rc = SDB_OK ;

      if ( PMD_CFG_STEP_RESTORE == _cfgStep )
      {
         if ( hasField( pFieldName ) )
         {
            BOOLEAN fieldStatus = TRUE ;
            if ( 0 == ossStrcasecmp( pValue, pDefault ) )
            {
               fieldStatus = FALSE ;
            }

            if ( PMD_CFG_CHANGE_REBOOT == changeLevel )
            {
               (*_pMapKeyField)[ pFieldName ] = pmdParamValue( pDefault, FALSE,
                                                               fieldStatus,
                                                               changeLevel ) ;
            }
            else if ( PMD_CFG_CHANGE_RUN == changeLevel )
            {
               ossStrncpy( pValue, pDefault, len ) ;
               pValue[ len - 1 ] = 0 ;
               (*_pMapKeyField)[ pFieldName ] = pmdParamValue( pDefault, TRUE,
                                                               fieldStatus,
                                                               changeLevel ) ;
            }
            else if ( PMD_CFG_CHANGE_FORBIDDEN == changeLevel )
            {
               MAP_K2V::iterator it = _pMapKeyField->find( pFieldName ) ;
               if ( _pMapKeyField->end() != it )
               {
                  it->second._hasMapped = TRUE ;
                  it->second._hasField = fieldStatus ;
               }
               else
               {
                  (*_pMapKeyField)[ pFieldName ] = pmdParamValue( pDefault, FALSE,
                                                                  fieldStatus, 
                                                                  changeLevel ) ;
               }
            }
         }
         goto done ;
      }

      rc = readString( pFieldName, pValue, len, changeLevel ) ;
      if ( SDB_FIELD_NOT_EXIST == rc && pDefault )
      {
         if ( PMD_CFG_STEP_PRECHG == _cfgStep ||
              ( PMD_CFG_STEP_CHG == _cfgStep &&
                PMD_CFG_CHANGE_FORBIDDEN == changeLevel ) )
         {
            rc = SDB_OK ;
            goto done ;
         }
         ossStrncpy( pValue, pDefault, len ) ;
         pValue[ len - 1 ] = 0 ;
         rc = SDB_OK ;

         (*_pMapKeyField)[ pFieldName ] = pmdParamValue( pValue, TRUE, FALSE) ;
      }
   done:
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
         _strStream << pFieldName << " = " << value << OSS_NEWLINE ;
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
         _strStream << pFieldName << " = " << pValue << OSS_NEWLINE ;
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
               _strStream << it->first << " = " << it->second._value
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
      while ( it != pVM->end() )
      {
         itKV = _pMapKeyField->find( it->first ) ;
         if ( itKV != _pMapKeyField->end() && TRUE == itKV->second._hasMapped )
         {
            ++it ;
            continue ;
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

      _mapKeyValue.clear() ;

      rc = doDataExchange( &ex ) ;
      if ( rc )
      {
         goto error ;
      }
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

      rc = toBSON( oldCfg, PMD_CFG_MASK_SKIP_UNFIELD ) ;
      PD_RC_CHECK( rc, PDERROR, "Save old config failed, rc: %d", rc ) ;

      _mutex.get() ;
      locked = TRUE ;

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

   INT32 _pmdCfgRecord::update( const BSONObj &fileConfig, 
                                const BSONObj &userConfig,
                                BOOLEAN useDefault,
                                BSONObj &errorObj,
                                string &confFileStr )
   {
      INT32 rc = SDB_OK ;
      BSONObj oldCfg ;
      MAP_K2V mapKeyField ;
      MAP_K2V::iterator it ;
      BOOLEAN locked = FALSE ;
      BSONObjBuilder errorObjBuilder ;
      BSONArrayBuilder rebootArrBuilder ;
      BSONArrayBuilder forbidArrBuilder ;
      stringstream strStream ;
      pmdCfgExchange ex1( &mapKeyField, fileConfig, TRUE, PMD_CFG_STEP_PRECHG ) ;
      pmdCfgExchange ex2( &mapKeyField, userConfig, TRUE, PMD_CFG_STEP_CHG ) ;
      pmdCfgExchange ex3( &mapKeyField, userConfig, TRUE, PMD_CFG_STEP_RESTORE ) ;

      rc = doDataExchange( &ex1 ) ;

      _mutex.get() ;
      locked = TRUE ;

      if ( !useDefault )
      {
         rc = doDataExchange( &ex2 ) ;
      }
      else
      {
         rc = doDataExchange( &ex3 ) ;
      }
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

      try
      {
         it = mapKeyField.begin() ;
         while ( it != mapKeyField.end() )
         {
            switch ( it->second._level )
            {
               case PMD_CFG_CHANGE_RUN :
                  strStream << it->first << "=" << it->second._value
                               << OSS_NEWLINE ;
                  break ;
               case PMD_CFG_CHANGE_REBOOT :
                  if ( TRUE == it->second._hasField )
                  {
                     rebootArrBuilder.append( it->first ) ;
                  }
                  strStream << it->first << "=" << it->second._value
                         << OSS_NEWLINE ;
                  break ;
               case PMD_CFG_CHANGE_FORBIDDEN :
                  if ( TRUE == it->second._hasField )
                  {
                     if ( TRUE == it->second._hasMapped )
                     {
                        forbidArrBuilder.append( it->first ) ;
                     }
                  }
                  else
                  {
                     if ( TRUE == it->second._hasMapped)
                     {
                        forbidArrBuilder.append( it->first ) ;
                     }
                     strStream << it->first << "=" << it->second._value
                               << OSS_NEWLINE ;
                  }
                  break ;
               default : break ;
            }

            if ( it->second._hasMapped )
            {
               _mapKeyValue[ it->first ] = it->second ;
            }
            ++it ;
         }

         confFileStr = strStream.str() ;

         errorObjBuilder.append( "Reboot", rebootArrBuilder.arr() ) ;
         errorObjBuilder.append( "Forbidden", forbidArrBuilder.arr() ) ;
         errorObj = errorObjBuilder.obj() ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDWARNING, "Exception during config update: %s",
                 e.what() ) ;
         goto error ;
      }

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
         INT32 rc = doDataExchange( &ex ) ;
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
            continue ;
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

         if ( pEX->isSkipUnField() )
         {
            MAP_K2V::iterator it = _mapKeyValue.find( pFieldName ) ;
            if ( it == _mapKeyValue.end() || !it->second._hasField )
            {
               goto done ;
            }
         }

         _result = pEX->writeString( pFieldName, pValue ) ;
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
         MAP_K2V::iterator it = _mapKeyValue.find( pFieldName ) ;
         if ( it != _mapKeyValue.end() )
         {
            it->second._value = pValue ;
         }
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

         MAP_K2V::iterator it = _mapKeyValue.find( pFieldName ) ;
         if ( it != _mapKeyValue.end() )
         {
            it->second._value = value ? "TRUE" : "FALSE" ;
         }
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

         if ( pEX->isSkipUnField() )
         {
            MAP_K2V::iterator it = _mapKeyValue.find( pFieldName ) ;
            if ( it == _mapKeyValue.end() || !it->second._hasField )
            {
               goto done ;
            }
         }

         _result = pEX->writeInt( pFieldName, value ) ;
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
         value = (UINT32)tmpValue ;
      }
      return _result ;
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
         value = (UINT16)tmpValue ;
      }
      return _result ;
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
         ossPrintf( "Waring: Field[%s] value[%u] is less than min value[%u]\n",
                    _curFieldName.c_str(), value, minV ) ;
         if ( autoAdjust )
         {
            value = minV ;
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
      ossMemset( _catAddrLine, 0, OSS_MAX_PATHSIZE + 1 ) ;
      ossMemset( _dmsTmpBlkPath, 0, OSS_MAX_PATHSIZE + 1 ) ;
      ossMemset( _krcbLobPath, 0, OSS_MAX_PATHSIZE + 1 ) ;
      ossMemset( _krcbLobMetaPath, 0, OSS_MAX_PATHSIZE + 1 ) ;
      ossMemset( _auditMaskStr, 0, sizeof( _auditMaskStr ) ) ;

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
      _memDebugSize        = 0 ;
      _indexScanStep       = PMD_DFT_INDEX_SCAN_STEP ;
      _dpslocal            = FALSE ;
      _traceOn             = FALSE ;
      _traceBufSz          = TRACE_DFT_BUFFER_SIZE ;
      _transactionOn       = FALSE ;
      _transTimeout        = PMD_DFT_TRANS_TIMEOUT ;
      _sharingBreakTime    = PMD_OPTION_BRK_TIME_DEFAULT ;
      _startShiftTime      = PMD_DFT_START_SHIFT_TIME ;
      _logBuffSize         = DPS_DFT_LOG_BUF_SZ ;
      _sortBufSz           = PMD_DEFAULT_SORTBUF_SZ ;
      _hjBufSz             = PMD_DEFAULT_HJ_SZ ;
      _dialogFileNum       = 0 ;
      _auditFileNum        = 0 ;
      _auditMask           = 0 ;
      _directIOInLob       = FALSE ;
      _sparseFile          = FALSE ;
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
      _planCacheLevel = OPT_PLAN_PARAMETERIZED ;
      _instanceID = PMD_DFT_INSTANCE_ID ;
      _maxconn = PMD_DFT_MAX_CONN;

      ossMemset( _krcbConfPath, 0, sizeof( _krcbConfPath ) ) ;
      ossMemset( _krcbConfFile, 0, sizeof( _krcbConfFile ) ) ;
      ossMemset( _krcbCatFile, 0, sizeof( _krcbCatFile ) ) ;
      _krcbSvcPort         = OSS_DFT_SVCPORT ;
   }

   _pmdOptionsMgr::~_pmdOptionsMgr()
   {
   }

   INT32 _pmdOptionsMgr::doDataExchange( pmdCfgExchange * pEX )
   {
      resetResult () ;

      rdxPath( pEX, PMD_OPTION_CONFPATH , _krcbConfPath, sizeof(_krcbConfPath),
               FALSE, PMD_CFG_CHANGE_FORBIDDEN, PMD_CURRENT_PATH ) ;
      rdxPath( pEX, PMD_OPTION_DBPATH, _krcbDbPath, sizeof(_krcbDbPath),
               FALSE, PMD_CFG_CHANGE_FORBIDDEN, PMD_CURRENT_PATH ) ;
      rdvNotEmpty( pEX, _krcbDbPath ) ;
      rdxPath( pEX, PMD_OPTION_IDXPATH, _krcbIndexPath, sizeof(_krcbIndexPath),
               FALSE, PMD_CFG_CHANGE_FORBIDDEN, "" ) ;
      rdxPath( pEX, PMD_OPTION_DIAGLOGPATH, _krcbDiagLogPath,
               sizeof(_krcbDiagLogPath), FALSE, PMD_CFG_CHANGE_REBOOT, "" ) ;
      rdxPath( pEX, PMD_OPTION_AUDITLOGPATH, _krcbAuditLogPath,
               sizeof(_krcbAuditLogPath), FALSE, PMD_CFG_CHANGE_REBOOT, "" ) ;
      rdxPath( pEX, PMD_OPTION_LOGPATH, _krcbLogPath, sizeof(_krcbLogPath),
               FALSE, PMD_CFG_CHANGE_FORBIDDEN, "" ) ;
      rdxPath( pEX, PMD_OPTION_BKUPPATH, _krcbBkupPath, sizeof(_krcbBkupPath),
               FALSE, PMD_CFG_CHANGE_REBOOT, "" ) ;
      rdxPath( pEX, PMD_OPTION_WWWPATH, _krcbWWWPath, sizeof(_krcbWWWPath),
               FALSE, PMD_CFG_CHANGE_FORBIDDEN, "", TRUE ) ;

      rdxPath( pEX, PMD_OPTION_LOBPATH, _krcbLobPath, sizeof(_krcbLobPath),
               FALSE, PMD_CFG_CHANGE_FORBIDDEN, "" ) ;
      rdxPath( pEX, PMD_OPTION_LOBMETAPATH, _krcbLobMetaPath, sizeof(_krcbLobMetaPath),
               FALSE, PMD_CFG_CHANGE_FORBIDDEN, "" ) ;

      rdxUInt( pEX, PMD_OPTION_MAXPOOL, _krcbMaxPool, FALSE, PMD_CFG_CHANGE_RUN,
               PMD_OPTION_DFT_MAXPOOL ) ;
      rdvMinMax( pEX, _krcbMaxPool, 0, 10000, TRUE ) ;

      rdxInt( pEX, PMD_OPTION_DIAGLOG_NUM, _dialogFileNum, FALSE, PMD_CFG_CHANGE_RUN,
              PD_DFT_FILE_NUM ) ;
      rdxInt( pEX, PMD_OPTION_AUDIT_NUM, _auditFileNum, FALSE, PMD_CFG_CHANGE_RUN,
              PD_DFT_FILE_NUM ) ;
      rdxString( pEX, PMD_OPTION_AUDIT_MASK, _auditMaskStr, sizeof(_auditMaskStr),
                 FALSE, PMD_CFG_CHANGE_RUN, AUDIT_MASK_DFT_STR ) ;
      rdxString( pEX, PMD_OPTION_SVCNAME, _krcbSvcName, sizeof(_krcbSvcName),
                 FALSE, PMD_CFG_CHANGE_FORBIDDEN,
                 boost::lexical_cast<string>(OSS_DFT_SVCPORT).c_str() ) ;
      rdvNotEmpty( pEX, _krcbSvcName ) ;
      rdxString( pEX, PMD_OPTION_REPLNAME, _replServiceName,
                 sizeof(_replServiceName), FALSE, PMD_CFG_CHANGE_FORBIDDEN, "" ) ;
      rdxString( pEX, PMD_OPTION_CATANAME, _catServiceName,
                 sizeof(_catServiceName), FALSE, PMD_CFG_CHANGE_FORBIDDEN, "" ) ;
      rdxString( pEX, PMD_OPTION_SHARDNAME, _shardServiceName,
                 sizeof(_shardServiceName), FALSE, PMD_CFG_CHANGE_FORBIDDEN, "" ) ;
      rdxString( pEX, PMD_OPTION_RESTNAME, _restServiceName,
                 sizeof(_restServiceName), FALSE, PMD_CFG_CHANGE_FORBIDDEN, "" ) ;
      rdxString( pEX, PMD_OPTION_OMNAME, _omServiceName,
                 sizeof(_omServiceName), FALSE, PMD_CFG_CHANGE_FORBIDDEN, "", TRUE ) ;
      rdxUShort( pEX, PMD_OPTION_DIAGLEVEL, _krcbDiagLvl, FALSE, PMD_CFG_CHANGE_RUN,
                 (UINT16)PDWARNING ) ;
      rdvMinMax( pEX, _krcbDiagLvl, PDSEVERE, PDDEBUG, TRUE ) ;
      rdxString( pEX, PMD_OPTION_ROLE, _krcbRole, sizeof(_krcbRole),
                 FALSE, PMD_CFG_CHANGE_FORBIDDEN, SDB_ROLE_STANDALONE_STR ) ;
      rdvNotEmpty( pEX, _krcbRole ) ;
      rdxUInt( pEX, PMD_OPTION_LOGFILESZ, _logFileSz, FALSE, PMD_CFG_CHANGE_FORBIDDEN,
               PMD_DFT_LOG_FILE_SZ ) ;
      rdvMinMax( pEX, _logFileSz, PMD_MIN_LOG_FILE_SZ, PMD_MAX_LOG_FILE_SZ,
                 TRUE ) ;
      rdxUInt( pEX, PMD_OPTION_LOGFILENUM, _logFileNum, FALSE, PMD_CFG_CHANGE_FORBIDDEN,
               PMD_DFT_LOG_FILE_NUM ) ;
      rdvMinMax( pEX, _logFileNum, 1, 60000, TRUE ) ;
      rdxUInt( pEX, PMD_OPTION_LOGBUFFSIZE, _logBuffSize, FALSE, PMD_CFG_CHANGE_REBOOT,
               DPS_DFT_LOG_BUF_SZ ) ;
      rdvMinMax( pEX, _logBuffSize, 512, 1024000, TRUE ) ;
      rdxUInt( pEX, PMD_OPTION_NUMPRELOAD, _numPreLoaders, FALSE, PMD_CFG_CHANGE_REBOOT, 0 ) ;
      rdvMinMax( pEX, _numPreLoaders, 0, 100, TRUE ) ;
      rdxUInt( pEX, PMD_OPTION_MAX_PREF_POOL, _maxPrefPool, FALSE, PMD_CFG_CHANGE_REBOOT,
               PMD_MAX_PREF_POOL ) ;
      rdvMinMax( pEX, _maxPrefPool, 0, 1000, TRUE ) ;
      rdxUInt( pEX, PMD_OPTION_MAX_SUB_QUERY, _maxSubQuery, FALSE, PMD_CFG_CHANGE_RUN,
               PMD_MAX_SUB_QUERY <= _maxPrefPool ?
               PMD_MAX_SUB_QUERY : _maxPrefPool ) ;
      rdvMinMax( pEX, _maxSubQuery, 0, _maxPrefPool, TRUE ) ;
      rdxUInt( pEX, PMD_OPTION_MAX_REPL_SYNC, _maxReplSync, FALSE, PMD_CFG_CHANGE_RUN,
               PMD_DEFAULT_MAX_REPLSYNC ) ;
      rdvMinMax( pEX, _maxReplSync, 0, 200, TRUE ) ;
      rdxUInt( pEX, PMD_OPTION_REPL_BUCKET_SIZE, _replBucketSize, FALSE,
               PMD_CFG_CHANGE_REBOOT, PMD_DFT_REPL_BUCKET_SIZE, TRUE ) ;
      rdvMinMax( pEX, _replBucketSize, 1, 4096, TRUE ) ;
      rdxString( pEX, PMD_OPTION_SYNC_STRATEGY, _syncStrategyStr,
                 sizeof( _syncStrategyStr ), FALSE, PMD_CFG_CHANGE_RUN, "", FALSE ) ;
      rdxString( pEX, PMD_OPTION_PREFINST, _prefInstStr,
                 sizeof( _prefInstStr ), FALSE, PMD_CFG_CHANGE_RUN, PMD_DFT_PREFINST ) ;
      rdxString( pEX, PMD_OPTION_PREFINST_MODE, _prefInstModeStr,
                 sizeof( _prefInstModeStr ), FALSE, PMD_CFG_CHANGE_RUN,
                 PMD_DFT_PREFINST_MODE ) ;
      rdxUInt( pEX, PMD_OPTION_INSTANCE_ID, _instanceID, FALSE, PMD_CFG_CHANGE_REBOOT,
               PMD_DFT_INSTANCE_ID, FALSE ) ;
      rdvMinMax( pEX, _instanceID, NODE_INSTANCE_ID_UNKNOWN,
                 NODE_INSTANCE_ID_MAX - 1, TRUE ) ;
      rdxUInt( pEX, PMD_OPTION_DATAERROR_OP, _dataErrorOp,
               FALSE, PMD_CFG_CHANGE_RUN, PMD_OPT_VALUE_FULLSYNC, FALSE ) ;
      rdvMinMax( pEX, _dataErrorOp, PMD_OPT_VALUE_NONE,
                 PMD_OPT_VALUE_SHUTDOWN, TRUE ) ;
      rdxBooleanS( pEX, PMD_OPTION_MEMDEBUG, _memDebugEnabled, FALSE,
                   PMD_CFG_CHANGE_REBOOT, FALSE, TRUE ) ;
      rdxUInt( pEX, PMD_OPTION_MEMDEBUGSIZE, _memDebugSize, FALSE,
               PMD_CFG_CHANGE_REBOOT, 0, TRUE ) ;
      rdxUInt( pEX, PMD_OPTION_INDEX_SCAN_STEP, _indexScanStep, FALSE,
               PMD_CFG_CHANGE_RUN, PMD_DFT_INDEX_SCAN_STEP, TRUE ) ;
      rdvMinMax( pEX, _indexScanStep, 1, 10000, TRUE ) ;
      rdxBooleanS( pEX, PMD_OPTION_DPSLOCAL, _dpslocal, FALSE,
                   PMD_CFG_CHANGE_REBOOT, FALSE, TRUE ) ;
      rdxBooleanS( pEX, PMD_OPTION_TRACEON, _traceOn, FALSE,
                   PMD_CFG_CHANGE_REBOOT, FALSE, TRUE ) ;
      rdxUInt( pEX, PMD_OPTION_TRACEBUFSZ, _traceBufSz, FALSE,
               PMD_CFG_CHANGE_REBOOT, TRACE_DFT_BUFFER_SIZE, TRUE ) ;
      rdvMinMax( pEX, _traceBufSz, TRACE_MIN_BUFFER_SIZE,
                 TRACE_MAX_BUFFER_SIZE, TRUE ) ;
      rdxBooleanS( pEX, PMD_OPTION_TRANSACTIONON, _transactionOn, FALSE,
                   PMD_CFG_CHANGE_REBOOT, FALSE ) ;
      rdxUInt( pEX, PMD_OPTION_TRANSTIMEOUT, _transTimeout, FALSE, PMD_CFG_CHANGE_RUN,
               PMD_DFT_TRANS_TIMEOUT, TRUE ) ;
      rdvMinMax( pEX, _transTimeout, 0, 3600, TRUE ) ;
      rdxUInt( pEX, PMD_OPTION_SHARINGBRK, _sharingBreakTime, FALSE, PMD_CFG_CHANGE_RUN,
               PMD_OPTION_BRK_TIME_DEFAULT, TRUE ) ;
      rdvMinMax( pEX, _sharingBreakTime, 5000, 300000, TRUE ) ;
      rdxUInt( pEX, PMD_OPTION_START_SHIFT_TIME, _startShiftTime, FALSE, PMD_CFG_CHANGE_RUN,
               PMD_DFT_START_SHIFT_TIME, TRUE ) ;
      rdvMinMax( pEX, _startShiftTime, 0, 7200, TRUE ) ;
      rdxString( pEX, PMD_OPTION_CATALOG_ADDR, _catAddrLine,
                 sizeof(_catAddrLine), FALSE, PMD_CFG_CHANGE_FORBIDDEN, "" ) ;

      rdxPath( pEX, PMD_OPTION_DMS_TMPBLKPATH, _dmsTmpBlkPath,
               sizeof(_dmsTmpBlkPath), FALSE, PMD_CFG_CHANGE_REBOOT, "" ) ;

      rdxUInt( pEX, PMD_OPTION_SORTBUF_SIZE, _sortBufSz,
               FALSE, PMD_CFG_CHANGE_RUN, PMD_DEFAULT_SORTBUF_SZ ) ;
      rdvMinMax( pEX, _sortBufSz, PMD_MIN_SORTBUF_SZ,
                 -1, TRUE ) ;

      rdxUInt( pEX, PMD_OPTION_HJ_BUFSZ, _hjBufSz,
               FALSE, PMD_CFG_CHANGE_RUN, PMD_DEFAULT_HJ_SZ ) ;
      rdvMinMax( pEX, _hjBufSz, PMD_MIN_HJ_SZ,
                 -1, TRUE ) ;

      rdxBooleanS( pEX, PMD_OPTION_DIRECT_IO_IN_LOB, _directIOInLob,
                   FALSE, PMD_CFG_CHANGE_RUN, FALSE, FALSE ) ;

      rdxBooleanS( pEX, PMD_OPTION_SPARSE_FILE, _sparseFile,
                   FALSE, PMD_CFG_CHANGE_RUN, FALSE, FALSE ) ;

      rdxUInt( pEX, PMD_OPTION_WEIGHT, _weight,
               FALSE, PMD_CFG_CHANGE_RUN, 10, FALSE ) ;
      rdvMinMax( pEX, _weight, 1, 100, TRUE ) ;

      rdxBooleanS( pEX, PMD_OPTION_AUTH, _auth,
                   FALSE, PMD_CFG_CHANGE_RUN, TRUE, FALSE ) ;
      rdxUInt( pEX, PMD_OPTION_PLAN_BUCKETS, _planBucketNum,
               FALSE, PMD_CFG_CHANGE_RUN, OPT_PLAN_DEF_CACHE_BUCKETS, FALSE ) ;
      rdvMinMax( pEX, _planBucketNum, OPT_PLAN_MIN_CACHE_BUCKETS,
                 OPT_PLAN_MAX_CACHE_BUCKETS, TRUE ) ;
      rdxUInt( pEX, PMD_OPTION_OPERATOR_TIMEOUT, _oprtimeout, FALSE,
               PMD_CFG_CHANGE_RUN, PMD_OPTION_OPR_TIME_DEFAULT, FALSE ) ;
      rdxUInt( pEX, PMD_OPTION_OVER_FLOW_RATIO, _overflowRatio, FALSE,
               PMD_CFG_CHANGE_RUN, PMD_DFT_OVERFLOW_RETIO, FALSE ) ;
      rdvMinMax( pEX, _overflowRatio, 0, 10000, TRUE ) ;
      rdxUInt( pEX, PMD_OPTION_EXTEND_THRESHOLD, _extendThreshold, FALSE,
               PMD_CFG_CHANGE_RUN, PMD_DFT_EXTEND_THRESHOLD, TRUE ) ;
      rdvMinMax( pEX, _extendThreshold, 0, 128, TRUE ) ;
      rdxInt( pEX, PMD_OPTION_SIGNAL_INTERVAL, _signalInterval, FALSE,
              PMD_CFG_CHANGE_RUN, 0, TRUE ) ;
      rdxUInt( pEX, PMD_OPTION_MAX_CACHE_SIZE, _maxCacheSize, FALSE,
               PMD_CFG_CHANGE_RUN,0, FALSE ) ;
      rdxUInt( pEX, PMD_OPTION_MAX_CACHE_JOB, _maxCacheJob, FALSE,
               PMD_CFG_CHANGE_RUN, PMD_DFT_MAX_CACHE_JOB, FALSE ) ;
      rdvMinMax( pEX, _maxCacheJob, 2, 200, TRUE ) ;
      rdxUInt( pEX, PMD_OPTION_MAX_SYNC_JOB, _maxSyncJob, FALSE,
               PMD_CFG_CHANGE_RUN, PMD_DFT_MAX_SYNC_JOB, FALSE ) ;
      rdvMinMax( pEX, _maxSyncJob, 2, 200, TRUE ) ;
      rdxUInt( pEX, PMD_OPTION_SYNC_INTERVAL, _syncInterval, FALSE,
               PMD_CFG_CHANGE_RUN, PMD_DFT_SYNC_INTERVAL, FALSE ) ;
      rdxUInt( pEX, PMD_OPTION_SYNC_RECORDNUM, _syncRecordNum, FALSE,
               PMD_CFG_CHANGE_RUN, PMD_DFT_SYNC_RECORDNUM, FALSE ) ;
      rdxBooleanS( pEX, PMD_OPTION_SYNC_DEEP, _syncDeep, FALSE,
                   PMD_CFG_CHANGE_RUN, FALSE, FALSE ) ;

      rdxBooleanS( pEX, PMD_OPTION_ARCHIVE_ON, _archiveOn,
                   FALSE, PMD_CFG_CHANGE_REBOOT, FALSE, FALSE ) ;

      rdxBooleanS( pEX, PMD_OPTION_ARCHIVE_COMPRESS_ON, _archiveCompressOn,
                   FALSE, PMD_CFG_CHANGE_RUN, TRUE, FALSE ) ;

      rdxPath( pEX, PMD_OPTION_ARCHIVE_PATH, _archivePath, sizeof(_archivePath),
               FALSE, PMD_CFG_CHANGE_FORBIDDEN, "" ) ;

      rdxUInt( pEX, PMD_OPTION_ARCHIVE_TIMEOUT, _archiveTimeout, FALSE,
               PMD_CFG_CHANGE_RUN, PMD_DFT_ARCHIVE_TIMEOUT, FALSE ) ;

      rdxUInt( pEX, PMD_OPTION_ARCHIVE_EXPIRED, _archiveExpired, FALSE,
               PMD_CFG_CHANGE_RUN, PMD_DFT_ARCHIVE_EXPIRED, FALSE ) ;

      rdxUInt( pEX, PMD_OPTION_ARCHIVE_QUOTA, _archiveQuota, FALSE,
               PMD_CFG_CHANGE_RUN, PMD_DFT_ARCHIVE_QUOTA, FALSE ) ;

      rdxString( pEX, PMD_OPTION_OM_ADDR, _omAddrLine,
                 sizeof(_omAddrLine), FALSE, PMD_CFG_CHANGE_FORBIDDEN, "" ) ;

      rdxUInt( pEX, PMD_OPTION_DMS_CHK_INTERVAL, _dmsChkInterval,
               FALSE, PMD_CFG_CHANGE_RUN, PMD_DFT_DMS_CHK_INTERVAL, TRUE ) ;
      rdvMinMax( pEX, _dmsChkInterval, 0, 240, TRUE ) ;

      rdxUInt( pEX, PMD_OPTION_CACHE_MERGE_SIZE, _cacheMergeSize,
               FALSE, PMD_CFG_CHANGE_RUN, PMD_DFT_CACHE_MERGE_SZ, TRUE ) ;
      rdvMinMax( pEX, _cacheMergeSize, 0, 64, TRUE ) ;

      rdxUInt( pEX, PMD_OPTION_PAGE_ALLOC_TIMEOUT, _pageAllocTimeout,
               FALSE, PMD_CFG_CHANGE_RUN, PMD_DFT_PAGE_ALLOC_TIMEOUT, TRUE ) ;
      rdvMinMax( pEX, _pageAllocTimeout, 0, 3600000, TRUE ) ;

      rdxBooleanS( pEX, PMD_OPTION_PERF_STAT, _perfStat, FALSE,
                   PMD_CFG_CHANGE_RUN, FALSE, TRUE ) ;

      rdxInt( pEX, PMD_OPTION_OPT_COST_THRESHOLD, _optCostThreshold, FALSE,
              PMD_CFG_CHANGE_RUN, PMD_DFT_OPT_COST_THRESHOLD, TRUE ) ;
      rdvMinMax( pEX, _optCostThreshold, -1, INT_MAX, TRUE ) ;

      rdxBooleanS( pEX, PMD_OPTION_ENABLE_MIX_CMP, _enableMixCmp, FALSE,
                   PMD_CFG_CHANGE_RUN, PMD_DFT_ENABLE_MIX_CMP, TRUE ) ;

      rdxUInt( pEX, PMD_OPTION_PLAN_CACHE_LEVEL, _planCacheLevel, FALSE,
               PMD_CFG_CHANGE_RUN, OPT_PLAN_PARAMETERIZED, FALSE ) ;
      rdvMinMax( pEX, _planCacheLevel, OPT_PLAN_NOCACHE, OPT_PLAN_FUZZYOPTR,
                 TRUE ) ;

      rdxUInt( pEX, PMD_OPTION_MAX_CONN,_maxconn, FALSE,
               PMD_CFG_CHANGE_RUN, PMD_DFT_MAX_CONN, FALSE ) ;
      rdvMinMax( pEX, _maxconn, 0, 30000, TRUE ) ;

      return getResult () ;
   }

   INT32 _pmdOptionsMgr::postLoaded ( PMD_CFG_STEP step )
   {
      INT32 rc = SDB_OK ;
      SDB_ROLE dbRole = SDB_ROLE_STANDALONE ;

      rc = ossGetPort( _krcbSvcName, _krcbSvcPort ) ;
      if ( SDB_OK != rc )
      {
         std::cerr << "Invalid svcname: " << _krcbSvcName << endl ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( _logBuffSize > _logFileNum * _logFileSz *
           ( 1048576 / DPS_DEFAULT_PAGE_SIZE ) )
      {
         std::cerr << "log buff size more than all log file size: "
                   << _logFileNum * _logFileSz << "MB" << std::endl ;
         _logBuffSize = _logFileNum * _logFileSz *
                        ( 1048576 / DPS_DEFAULT_PAGE_SIZE ) ;
      }

      if ( SDB_ROLE_MAX == ( dbRole = utilGetRoleEnum( _krcbRole ) ) )
      {
         std::cerr << "db role: " << _krcbRole << " error" << std::endl ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !ossIsPowerOf2( _replBucketSize ) )
      {
         _replBucketSize = PMD_DFT_REPL_BUCKET_SIZE ;
      }

      if ( 0 != _extendThreshold && !ossIsPowerOf2( _extendThreshold ) )
      {
         _extendThreshold = PMD_DFT_EXTEND_THRESHOLD ;
      }

      if ( SDB_OK != clsString2Strategy( _syncStrategyStr, _syncStrategy ) )
      {
         std::cerr << PMD_OPTION_SYNC_STRATEGY << " value error, use default"
                   << endl ;
         _syncStrategy = CLS_SYNC_DTF_STRATEGY ;
      }
      _syncStrategyStr[0] = 0 ;

      _auditMask = 0 ;
      if ( SDB_OK != pdString2AuditMask( _auditMaskStr, _auditMask ) )
      {
         std::cerr << PMD_OPTION_AUDIT_MASK << " value error, use default"
                   << endl ;
         _auditMask = AUDIT_MASK_DEFAULT ;
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
                                    ".."OSS_FILE_SEP PMD_OPTION_WWW_PATH_DIR,
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

      if ( SDB_ROLE_CATALOG == dbRole || SDB_ROLE_OM == dbRole )
      {
         _transactionOn = TRUE ;
      }

      if ( _transactionOn )
      {
         PD_CHECK( _logFileNum >= 5, SDB_INVALIDARG, error, PDERROR,
                   "The value of parameter \"logfilenum\" must be greater than 5 "
                   "when configure transaction" ) ;
      }

      if ( _memDebugSize != 0 )
      {
         _memDebugSize = OSS_MIN ( _memDebugSize, SDB_MEMDEBUG_MAXGUARDSIZE ) ;
         _memDebugSize = OSS_MAX ( _memDebugSize, SDB_MEMDEBUG_MINGUARDSIZE ) ;
      }

      if ( 0 == _vecCat.size() )
      {
         rc = parseAddressLine( _catAddrLine, _vecCat ) ;
         if ( rc )
         {
            goto error ;
         }
      }

      if ( SDB_ROLE_STANDALONE == dbRole &&
           ( _transactionOn || _archiveOn ) )
      {
         _dpslocal = TRUE ;
      }

      if ( SDB_ROLE_CATALOG == dbRole || SDB_ROLE_OM == dbRole )
      {
         _numPreLoaders    = 0 ;
         _maxPrefPool      = 0 ;
         _maxSubQuery      = 0 ;
         _maxReplSync      = 0 ;
         _syncStrategy     = CLS_SYNC_NONE ;

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
      if ( 0 != _catAddrLine[ 0 ] )
      {
         _addToFieldMap( PMD_OPTION_CATALOG_ADDR, _catAddrLine, TRUE, TRUE ) ;
      }

      addr = makeAddressLine( _vecOm ) ;
      ossStrncpy( _omAddrLine, addr.c_str(), OSS_MAX_PATHSIZE ) ;
      _omAddrLine[ OSS_MAX_PATHSIZE ] = 0 ;
      if ( 0 != _omAddrLine[ 0 ] )
      {
         _addToFieldMap( PMD_OPTION_OM_ADDR, _omAddrLine, TRUE, TRUE ) ;
      }

      clsStrategy2String( _syncStrategy, _syncStrategyStr,
                          sizeof( _syncStrategyStr ) ) ;

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
         if ( vm.count( PMD_OPTION_CONFPATH ) )
         {
            std::cerr << "Read config file: " << _krcbConfFile
                      << " failed, rc: " << rc << std::endl ;
            goto error ;
         }
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
         _addToFieldMap( PMD_OPTION_CONFPATH, _krcbConfPath, TRUE, FALSE ) ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDOPTMGR_REFLUSH2FILE, "_pmdOptionsMgr::reflush2file" )
   INT32 _pmdOptionsMgr::reflush2File( UINT32 mask )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY (SDB__PMDOPTMGR_REFLUSH2FILE ) ;
      std::string line ;
      CHAR conf[ OSS_MAX_PATHSIZE + 1 ] = {0} ;

      rc = pmdCfgRecord::toString( line, mask ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get the line str:%d", rc ) ;
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

   INT32 _pmdOptionsMgr::changeConfig( const BSONObj &objData,
                                       BSONObj &errorObj,
                                       BOOLEAN useDefault )
   {
      INT32 rc = SDB_OK ;

      pmdOptionsCB tmpCB ;
      BSONObj configObj ;
      string confFileStr ;
      CHAR conf[ OSS_MAX_PATHSIZE + 1 ] = {0} ;

      rc = tmpCB.initFromFile( getConfFile(), FALSE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Error while Reading from config file[%s], rc: %d",
                 getConfFile(), rc ) ;
         goto error ;
      }

      rc = tmpCB.toBSON( configObj, PMD_CFG_MASK_SKIP_UNFIELD ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Convert config to bson failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = update( configObj, objData, useDefault, errorObj, confFileStr ) ;
      if ( SDB_OK != rc )
      {
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

         rc = utilWriteConfigFile( conf, confFileStr.c_str(), FALSE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to write config[%s], rc: %d",
                      conf, rc ) ;

         if ( getConfigHandler() )
         {
            getConfigHandler()->onConfigSave() ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _pmdOptionsMgr::clearCatAddr()
   {
      _vecCat.clear() ;
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

}


