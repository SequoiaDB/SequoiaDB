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

   Source File Name = clsMainCLMonAggregator.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/15//2020 LSQ Initial Draft

   Last Changed =

*******************************************************************************/

#include "clsMainCLMonAggregator.hpp"
#include "utilMath.hpp"

using namespace bson ;

namespace engine
{
   /*
      TOOL Functions
   */
   /*
   INT32 clsParseShowMainCLModeHint( const BSONObj &hint,
                                     clsShowMainCLMode &mode )
   {
      INT32 rc = SDB_OK ;
      mode = SHOW_MODE_SUB ;
      try
      {
         BSONElement beOpt = hint.getField( "$"FIELD_NAME_OPTIONS ) ;
         if ( Object == beOpt.type() )
         {
            BSONObjIterator itr ( beOpt.embeddedObject() ) ;
            BSONElement elem ;
            while ( itr.more() )
            {
               elem = itr.next() ;
               if ( 0 == ossStrcasecmp( elem.fieldName(),
                                        FIELD_NAME_SHOW_MAIN_CL_MODE ) &&
                    String == elem.type() )
               {
                  if ( 0 == ossStrcasecmp( elem.valuestr(), VALUE_NAME_MAIN ) )
                  {
                     mode = SHOW_MODE_MAIN ;
                  }
                  else if ( 0 == ossStrcasecmp( elem.valuestr(), VALUE_NAME_SUB ) )
                  {
                     mode = SHOW_MODE_SUB ;
                  }
                  else if ( 0 == ossStrcasecmp( elem.valuestr(), VALUE_NAME_BOTH ) )
                  {
                     mode = SHOW_MODE_BOTH ;
                  }
               }
            }
         }
      }
      catch ( std::exception& e )
      {
         rc = ossException2RC( &e );
         PD_LOG( PDERROR, "Failed to parse option, received unexpected "
                 "error:%s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
   */

   /*
      _clsMainCLMonInfo implement
   */
   _clsMainCLMonInfo::_clsMainCLMonInfo( const clsCatalogSet *pCataSet,
                                         UINT32 subCLCount )
   {
      ossStrncpy( _name, pCataSet->name(), sizeof( _name ) ) ;
      _clUniqueID = pCataSet->clUniqueID() ;
      _totalSubCLCount = subCLCount ;
      _doneSubCLCount = 0 ;
      _totalLobCapacity = 0 ;

      // no meaning field
      _detail._blockID = (UINT16) -1 ;
      _detail._logicID = (UINT32) -1 ;
      _detail._flag = DMS_MB_FLAG_USED ;
      _detail._dictVersion = 0 ;
      _detail._dataCommitLSN = (UINT64) -1 ;
      _detail._idxCommitLSN = (UINT64) -1 ;
      _detail._lobCommitLSN = (UINT64) -1 ;

      // special field
      _detail._dataIsValid = TRUE ;
      _detail._idxIsValid = TRUE ;
      _detail._lobIsValid = TRUE ;
      _detail._dictCreated = TRUE ;
      _detail._currCompressRatio = 0 ;
      _detail._numIndexes = 0 ;
      _detail._pageSize = DMS_PAGE_SIZE_MAX ;
      _detail._lobPageSize = DMS_PAGE_SIZE512K ;
      _detail._attribute = pCataSet->getAttribute() ;
      _detail._compressType = pCataSet->getCompressType() ;

      _detail._createTime = pCataSet->getCreateTime() ;
      _detail._updateTime = pCataSet->getUpdateTime() ;
   }

   void _clsMainCLMonInfo::append( const monCollection &subCLInfo )
   {
      MON_CL_DETAIL_MAP::const_iterator itDetail ;
      for ( itDetail = subCLInfo._details.begin() ;
            itDetail != subCLInfo._details.end() ;
            ++itDetail )
      {
         const detailedInfo &sub = itDetail->second ;

         UINT32 dataPageMultiple = ( sub._pageSize / DMS_PAGE_SIZE_BASE ) ;
         UINT32 lobPageMultiple = ( sub._lobPageSize / DMS_PAGE_SIZE_BASE ) ;

         /// stat info
         _detail._totalRecords += sub._totalRecords ;
         _detail._totalLobs += sub._totalLobs ;
         _detail._totalDataPages += sub._totalDataPages * dataPageMultiple ;
         _detail._totalIndexPages += sub._totalIndexPages * dataPageMultiple ;
         _detail._totalLobPages += sub._totalLobPages * lobPageMultiple ;
         _detail._totalUsedLobSpace += sub._totalUsedLobSpace ;
         _detail._totalLobSize += sub._totalLobSize ;
         _detail._totalValidLobSize += sub._totalValidLobSize ;
         _detail._totalDataFreeSpace += sub._totalDataFreeSpace ;
         _detail._totalIndexFreeSpace += sub._totalIndexFreeSpace ;

         /// CRUD statistics
         _detail._crudCB._totalDataRead += sub._crudCB._totalDataRead ;
         _detail._crudCB._totalIndexRead += sub._crudCB._totalIndexRead ;
         _detail._crudCB._totalDataWrite += sub._crudCB._totalDataWrite ;
         _detail._crudCB._totalIndexWrite += sub._crudCB._totalIndexWrite ;
         _detail._crudCB._totalUpdate += sub._crudCB._totalUpdate ;
         _detail._crudCB._totalDelete += sub._crudCB._totalDelete ;
         _detail._crudCB._totalInsert += sub._crudCB._totalInsert ;
         _detail._crudCB._totalSelect += sub._crudCB._totalSelect ;
         _detail._crudCB._totalRead += sub._crudCB._totalRead ;
         _detail._crudCB._totalWrite += sub._crudCB._totalWrite ;
         _detail._crudCB._totalTbScan += sub._crudCB._totalTbScan ;
         _detail._crudCB._totalIxScan += sub._crudCB._totalIxScan ;
         _detail._crudCB._totalLobGet += sub._crudCB._totalLobGet ;
         _detail._crudCB._totalLobPut += sub._crudCB._totalLobPut ;
         _detail._crudCB._totalLobDelete += sub._crudCB._totalLobDelete ;
         _detail._crudCB._totalLobList += sub._crudCB._totalLobList ;
         _detail._crudCB._totalLobReadSize += sub._crudCB._totalLobReadSize ;
         _detail._crudCB._totalLobWriteSize += sub._crudCB._totalLobWriteSize ;
         _detail._crudCB._totalLobRead += sub._crudCB._totalLobRead ;
         _detail._crudCB._totalLobWrite += sub._crudCB._totalLobWrite ;
         _detail._crudCB._totalLobTruncate += sub._crudCB._totalLobTruncate ;
         _detail._crudCB._totalLobAddressing += sub._crudCB._totalLobAddressing ;
         _detail._crudCB._resetTimestamp =  sub._crudCB._resetTimestamp ;

         // special field
         if ( sub._pageSize < _detail._pageSize )
         {
            _detail._pageSize = sub._pageSize ;
         }
         if ( sub._lobPageSize < _detail._lobPageSize )
         {
            _detail._lobPageSize = sub._lobPageSize ;
         }
         if ( sub._numIndexes > _detail._numIndexes )
         {
            _detail._numIndexes = sub._numIndexes ;
         }
         if ( !sub._dataIsValid )
         {
            _detail._dataIsValid = FALSE ;
         }
         if ( !sub._idxIsValid )
         {
            _detail._idxIsValid = FALSE ;
         }
         if ( !sub._lobIsValid )
         {
            _detail._lobIsValid = FALSE ;
         }
         if ( !sub._dictCreated )
         {
            _detail._dictCreated = FALSE ;
         }
         if ( sub._createTime < _detail._createTime )
         {
            _detail._createTime = sub._createTime ;
         }
         if ( sub._updateTime > _detail._updateTime )
         {
            _detail._updateTime = sub._updateTime ;
         }
         _detail._currCompressRatio += sub._currCompressRatio ;
         _totalLobCapacity += (UINT64)( sub._totalUsedLobSpace / sub._usedLobSpaceRatio ) ;
      }
      ++_doneSubCLCount ;
   }

   void _clsMainCLMonInfo::get( monCollection &out )
   {
      // Calculate the total pages base on the min page size of all sub cl.
      _detail._totalDataPages /= ( _detail._pageSize / DMS_PAGE_SIZE_BASE ) ;
      _detail._totalIndexPages /= (_detail._pageSize / DMS_PAGE_SIZE_BASE ) ;
      _detail._totalLobPages /= ( _detail._lobPageSize / DMS_PAGE_SIZE_BASE ) ;
      // Calculate the average compression ratio
      _detail._currCompressRatio = _detail._currCompressRatio / _doneSubCLCount ;
      // Calculate the average of lob info
      _detail._usedLobSpaceRatio = utilPercentage( _detail._totalUsedLobSpace, _totalLobCapacity ) ;
      /// Because lob page 0 is unevenly distributed on data nodes, the
      /// _totalValidLobSize may be larger than the _totalUsedLobSpace,
      /// so use _totalLobSize / _totalUsedLobSpace in data nodes.
      _detail._lobUsageRate = utilPercentage( _detail._totalLobSize, _detail._totalUsedLobSpace ) ;
      if ( 0 < _detail._totalLobs )
      {
         _detail._avgLobSize = _detail._totalValidLobSize / _detail._totalLobs ;
      }
      else
      {
         _detail._avgLobSize = 0 ;
      }

      ossStrncpy( out._name, _name, sizeof( _name ) ) ;
      out._clUniqueID = _clUniqueID ;
      out._details.clear() ;
      out._details.insert( std::pair< UINT32, detailedInfo >( 1, _detail ) ) ;
   }

   /*
      _clsMainCLMonAggregator implement
   */

   /*
   _clsMainCLMonAggregator::~_clsMainCLMonAggregator()
   {
      MainCLInfoMap::iterator it ;
      for ( it = _infoMap.begin() ; it != _infoMap.end() ; ++it )
      {
         SDB_OSS_DEL it->second ;
      }
   }

   _clsMainCLMonAggregator::_clsMainCLMonAggregator( clsShowMainCLMode mode )
   {
      _mode = mode ;
      _pShdMgr = sdbGetShardCB() ;
      _pCatAgent = pmdGetKRCB()->getClsCB()->getCatAgent() ;
   }

   INT32 _clsMainCLMonAggregator::process( const monCollection &clIn,
                                           monCollection &clOut,
                                           UINT32 &resultFlag )
   {
      INT32 rc = SDB_OK ;
      resultFlag =  0 ;

      if ( SHOW_MODE_SUB == _mode )
      {
         goto done ;
      }

      try
      {
         MainCLInfoMap::iterator it ;
         _clsMainCLMonInfo *pMainCLInfo = NULL ;
         std::string mainCLName ;

         rc = _getMainCLName( clIn._name, mainCLName ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to get main cl name, rc=%d", rc );
         if ( mainCLName.empty() )
         {
            goto done ;
         }

         if ( SHOW_MODE_MAIN == _mode )
         {
            resultFlag |= FLAG_IGNORE ;
         }

         it = _infoMap.find( mainCLName ) ;
         if ( it != _infoMap.end() )
         {
            pMainCLInfo = it->second ;
         }
         else
         {
            std::pair<MainCLInfoMap::iterator, bool> ret ;
            rc = _createMainCLInfo( mainCLName.c_str(), &pMainCLInfo ) ;
            PD_RC_CHECK( rc, PDWARNING,
                         "Failed to create main cl[%s] mon info, rc=%d",
                         mainCLName.c_str(), rc ) ;

            ret = _infoMap.insert( MainCLInfoPair( mainCLName, pMainCLInfo ) ) ;
            it = ret.first ;
         }

         pMainCLInfo->append( clIn ) ;
         if ( pMainCLInfo->isFinished() )
         {
            pMainCLInfo->get( clOut ) ;
            SDB_OSS_DEL pMainCLInfo ;
            _infoMap.erase( it ) ;
            resultFlag |= FLAG_OUTPUT ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Failed to process cl info: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      // cl may be not exist for currently dropping.
      if ( SDB_DMS_NOTEXIST == rc )
      {
         rc = SDB_OK ;
      }
      goto done ;
   }

   INT32 _clsMainCLMonAggregator::outputDataInProcess( MON_CL_LIST &out )
   {
      int rc = SDB_OK ;
      try 
      {
         MainCLInfoMap::iterator it ;
         for ( it = _infoMap.begin() ; it != _infoMap.end(); ++it )
         {
            _clsMainCLMonInfo *pMainCLInfo = it->second ;
            monCollection monCL;
            pMainCLInfo->get( monCL ) ;
            out.insert( monCL );
            SDB_OSS_DEL pMainCLInfo ;
         }
         _infoMap.clear();
      } 
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Failed to out put data in process: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 _clsMainCLMonAggregator::_getMainCLName( const CHAR *clName,
                                                  std::string &mainCLName )
   {
      INT32 rc = SDB_OK ;
      _clsCatalogSet* pCatSet = NULL ;

      rc = _pShdMgr->getAndLockCataSet( clName, &pCatSet, TRUE ) ;
      if ( SDB_OK == rc && pCatSet )
      {
         mainCLName = pCatSet->getMainCLName() ;
      }
      _pShdMgr->unlockCataSet( pCatSet ) ;

      return rc ;
   }

   INT32 _clsMainCLMonAggregator::_createMainCLInfo( const CHAR *mainCLName,
                                                     _clsMainCLMonInfo **info )
   {
      INT32 rc = SDB_OK ;
      _clsCatalogSet* pMainCata = NULL ;
      _clsCatalogSet* pSubCata = NULL ;
      CLS_SUBCL_LIST subCLList ;
      CLS_SUBCL_LIST_IT iter ;
      UINT32 subCLCount = 0 ;

      rc = _pShdMgr->getAndLockCataSet( mainCLName, &pMainCata, TRUE ) ;
      if ( SDB_OK == rc && pMainCata )
      {
         SDB_ASSERT( pMainCata->isMainCL(), "cl must be main cl" ) ;
         pMainCata->getSubCLList( subCLList ) ;
         _pShdMgr->unlockCataSet( pMainCata ) ;
      }
      else
      {
         _pShdMgr->unlockCataSet( pMainCata ) ;
         PD_RC_CHECK( rc, PDWARNING, "can not find collection:%s", mainCLName ) ;
      }

      // Find out sub cl that in local.
      subCLCount = 0 ;
      iter = subCLList.begin() ;
      while( iter != subCLList.end() )
      {
         const CHAR *subCLName = (*iter).c_str() ;

         _pCatAgent->lock_r() ;
         pSubCata = _pCatAgent->collectionSet( subCLName ) ;
         if ( NULL == pSubCata )
         {
            _pCatAgent->release_r() ;
            rc = _pShdMgr->syncUpdateCatalog( subCLName ) ;
            PD_RC_CHECK( rc, PDWARNING,
                         "Failed to update catalog of collection[%s], rc=%d",
                         subCLName, rc ) ;
            continue ;
         }

         // Not on this node, kick it out.
         if ( 0 == pSubCata->groupCount() )
         {
            _pCatAgent->release_r() ;

            _pCatAgent->lock_w() ;
            _pCatAgent->clear( subCLName ) ;
            _pCatAgent->release_w() ;

            ++iter ;
            continue ;
         }

         _pCatAgent->release_r() ;
         ++iter ;
         ++subCLCount ;
      }

      rc = _pShdMgr->getAndLockCataSet( mainCLName, &pMainCata, TRUE ) ;
      if ( rc != SDB_OK || !pMainCata )
      {
         _pShdMgr->unlockCataSet( pMainCata ) ;
         PD_LOG( PDWARNING, "can not find collection:%s", mainCLName ) ;
         goto error ;
      }

      *info = SDB_OSS_NEW _clsMainCLMonInfo( pMainCata, subCLCount ) ;
      if ( NULL == *info )
      {
         _pShdMgr->unlockCataSet( pMainCata ) ;
         rc = SDB_OOM ;
         PD_RC_CHECK( rc, PDERROR, "No memory to store main cl info" ) ;
      }

      _pShdMgr->unlockCataSet( pMainCata ) ;
   done:
      return rc ;
   error:
      goto done ;
   }
   */

}

