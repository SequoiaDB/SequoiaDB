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

   Source File Name = coordCommandStat.cpp

   Descriptive Name = Coord Commands

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   user command processing on coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#include "coordCommandStat.hpp"
#include "pmd.hpp"
#include "rtnCB.hpp"
#include "coordQueryOperator.hpp"
#include "msgMessage.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"
#include "clsMainCLMonAggregator.hpp"
#include "monDump.hpp"
#include "dmsStatUnit.hpp"
#include "utilMinHeap.hpp"

using namespace bson ;

namespace bson
{
   extern BSONObj staticNull ;
}

namespace engine
{

   /*
      _coordCMDStatisticsBase implement
   */
   _coordCMDStatisticsBase::_coordCMDStatisticsBase()
   {
   }

   _coordCMDStatisticsBase::~_coordCMDStatisticsBase()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_CMDSTATBASE_EXE, "_coordCMDStatisticsBase::execute" )
   INT32 _coordCMDStatisticsBase::execute( MsgHeader *pMsg,
                                           pmdEDUCB *cb,
                                           INT64 &contextID,
                                           rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_CMDSTATBASE_EXE ) ;

      SDB_RTNCB *pRtncb                = pmdGetKRCB()->getRTNCB() ;

      // fill default-reply(execute success)
      contextID                        = -1 ;

      coordQueryOperator queryOpr( isReadOnly() ) ;
      rtnContextCoord::sharePtr pContext ;
      coordQueryConf queryConf ;
      coordSendOptions sendOpt ;
      queryConf._openEmptyContext = openEmptyContext() ;

      const CHAR *pHint = NULL ;

      // extract request-message
      rc = msgExtractQuery( (const CHAR*)pMsg, NULL, NULL,
                            NULL, NULL, NULL, NULL,
                            NULL, &pHint );
      PD_RC_CHECK ( rc, PDERROR, "Execute failed, failed to parse query "
                    "request, rc: %d", rc ) ;

      try
      {
         BSONObj boHint( pHint ) ;
         //get collection name
         BSONElement ele = boHint.getField( FIELD_NAME_COLLECTION ) ;
         PD_CHECK ( ele.type() == String,
                    SDB_INVALIDARG, error, PDERROR,
                    "Execute failed, failed to get the field(%s)",
                    FIELD_NAME_COLLECTION ) ;
         queryConf._realCLName = ele.str() ;
      }
      catch( std::exception &e )
      {
         PD_RC_CHECK ( rc, PDERROR, "Execute failed, occured unexpected "
                       "error:%s", e.what() ) ;
      }

      if ( 0 == ossStrncmp( queryConf._realCLName.c_str(),
                            CMD_ADMIN_PREFIX SYS_VIRTUAL_CS".",
                            SYS_VIRTUAL_CS_LEN + 1 ) )
      {
         rc = _executeOnVCL( queryConf._realCLName.c_str(), cb, contextID ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Execute on VCL failed, rc: %d", rc ) ;
            goto error ;
         }
      }
      else
      {
         rc = queryOpr.init( _pResource, cb, getTimeout() ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Init query operator failed, rc: %d", rc ) ;
            goto error ;
         }

         rc = queryOpr.queryOrDoOnCL( pMsg, cb, &pContext,
                                      sendOpt, &queryConf, buf ) ;
         PD_RC_CHECK( rc, PDERROR, "Query failed, rc: %d", rc ) ;

         _cataPtr = queryOpr.getCataPtr() ;

         // statistics the result
         rc = generateResult( pContext, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to execute statistics, rc: %d", rc ) ;

         contextID = pContext->contextID() ;
         pContext->reopen() ;
      }

   done:
      PD_TRACE_EXITRC ( COORD_CMDSTATBASE_EXE, rc ) ;
      return rc ;
   error:
      if ( pContext )
      {
         pRtncb->contextDelete( pContext->contextID(), cb ) ;
      }
      goto done ;
   }

   INT32 _coordCMDStatisticsBase::_executeOnVCL( const CHAR *pCLName,
                                                 pmdEDUCB *cb,
                                                 INT64 &contextID )
   {
      INT32 rc = SDB_OK ;
      rtnContextCoord::sharePtr pContext ;
      SDB_RTNCB *pRtncb = pmdGetKRCB()->getRTNCB() ;
      rtnQueryOptions defaultOptions ;

      if ( 0 != ossStrcmp( pCLName, CMD_ADMIN_PREFIX SYS_CL_SESSION_INFO ) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = pRtncb->contextNew( RTN_CONTEXT_COORD,
                               pContext,
                               contextID, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Create context failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = pContext->open( defaultOptions, FALSE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Open context failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = generateVCLResult( pCLName, pContext, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Generate VCL result failed, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      if ( contextID != -1 )
      {
         pRtncb->contextDelete( contextID , cb ) ;
         contextID = -1 ;
      }
      goto done ;
   }

   INT32 _coordCMDStatisticsBase::generateVCLResult( const CHAR *pCLName,
                                                     rtnContext *pContext,
                                                     pmdEDUCB *cb )
   {
      return SDB_OK ;
   }

   /*
      _coordCMDGetIndexes implement
   */
   _coordCMDGetIndexesOldVersion::_coordCMDGetIndexesOldVersion()
   {
   }

   _coordCMDGetIndexesOldVersion::~_coordCMDGetIndexesOldVersion()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_GETINDEX_GENRESULT, "_coordCMDGetIndexesOldVersion::generateResult" )
   INT32 _coordCMDGetIndexesOldVersion::generateResult( rtnContext *pContext,
                                                        pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_GETINDEX_GENRESULT ) ;

      CoordIndexMap indexMap ;
      rtnContextBuf buffObj ;

      // get index from all nodes
      while( TRUE )
      {
         rc = pContext->getMore( 1, buffObj, cb ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }
            else
            {
               PD_LOG ( PDERROR, "Failed to get index data, rc: %d", rc ) ;
               goto error ;
            }
         }

         try
         {
            BSONObj boTmp( buffObj.data() ) ;
            BSONObj boIndexDef ;
            BSONElement ele ;
            string strIndexName ;
            CoordIndexMap::iterator iter ;

            ele = boTmp.getField( IXM_FIELD_NAME_INDEX_DEF ) ;
            PD_CHECK ( ele.type() == Object, SDB_INVALIDARG, error,
                       PDERROR, "Failed to get the field(%s)",
                       IXM_FIELD_NAME_INDEX_DEF ) ;

            boIndexDef = ele.embeddedObject() ;
            ele = boIndexDef.getField( IXM_NAME_FIELD ) ;
            PD_CHECK ( ele.type() == String, SDB_INVALIDARG, error,
                       PDERROR, "Failed to get the field(%s)",
                       IXM_NAME_FIELD ) ;

            strIndexName = ele.valuestr() ;
            iter = indexMap.find( strIndexName ) ;
            if ( indexMap.end() == iter )
            {
               indexMap[ strIndexName ] = boTmp.getOwned() ;
            }
            else
            {
               // check the index
               BSONObjIterator newIter( boIndexDef ) ;
               BSONObj boOldDef ;

               ele = iter->second.getField( IXM_FIELD_NAME_INDEX_DEF ) ;
               PD_CHECK ( ele.type() == Object, SDB_INVALIDARG, error,
                          PDERROR, "Failed to get the field(%s)",
                          IXM_FIELD_NAME_INDEX_DEF ) ;
               boOldDef = ele.embeddedObject() ;

               BSONElement beTmp1, beTmp2 ;
               while( newIter.more() )
               {
                  beTmp1 = newIter.next() ;
                  if ( 0 == ossStrcmp( beTmp1.fieldName(), "_id" ) )
                  {
                     continue ;
                  }
                  beTmp2 = boOldDef.getField( beTmp1.fieldName() ) ;
                  if ( 0 != beTmp1.woCompare( beTmp2 ) )
                  {
                     PD_LOG( PDWARNING, "Corrupted index(name:%s, define1:%s, "
                             "define2:%s)", strIndexName.c_str(),
                             boIndexDef.toString().c_str(),
                             boOldDef.toString().c_str() ) ;
                     break ;
                  }
               }
            }
         }
         catch ( std::exception &e )
         {
            PD_RC_CHECK( rc, PDERROR, "Failed to get index, occured unexpected"
                         "error:%s", e.what() ) ;
         }
      }

      {
         CoordIndexMap::iterator iterMap = indexMap.begin();
         while( iterMap != indexMap.end() )
         {
            rc = pContext->append( iterMap->second ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get index, append the data "
                         "failed(rc=%d)", rc ) ;
            ++iterMap ;
         }
      }

   done:
      PD_TRACE_EXITRC ( COORD_GETINDEX_GENRESULT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordCMDGetCount implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDGetCount,
                                      CMD_NAME_GET_COUNT,
                                      TRUE ) ;
   _coordCMDGetCount::_coordCMDGetCount()
   {
   }

   _coordCMDGetCount::~_coordCMDGetCount()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_GETCOUNT_GENRESULT, "_coordCMDGetCount::generateResult" )
   INT32 _coordCMDGetCount::generateResult( rtnContext *pContext,
                                            pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( COORD_GETCOUNT_GENRESULT ) ;

      SINT64 totalCount = 0 ;
      rtnContextBuf buffObj ;

      while( TRUE )
      {
         rc = pContext->getMore( 1, buffObj, cb ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }
            else
            {
               PD_LOG ( PDERROR, "Failed to generate count result"
                        "get data failed, rc: %d", rc ) ;
               goto error ;
            }
         }

         try
         {
            BSONObj boTmp( buffObj.data() ) ;
            BSONElement beTotal = boTmp.getField( FIELD_NAME_TOTAL ) ;
            PD_CHECK( beTotal.isNumber(), SDB_INVALIDARG, error,
                      PDERROR, "Failed to get the field(%s)",
                      FIELD_NAME_TOTAL ) ;
            totalCount += beTotal.number() ;
         }
         catch ( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_RC_CHECK( rc, PDERROR, "Failed to generate count result,"
                         "occured unexpected error:%s", e.what() );
         }
      }

      try
      {
         rc = pContext->append( BSON( FIELD_NAME_TOTAL << totalCount ) ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to generate count result,"
                      "append the data failed, rc: %d", rc ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_RC_CHECK( rc, PDERROR, "Failed to generate count result,"
                      "occured unexpected error:%s", e.what() ) ;
      }

   done:
      PD_TRACE_EXITRC ( COORD_GETCOUNT_GENRESULT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordCMDGetCount::generateVCLResult( const CHAR *pCLName,
                                               rtnContext *pContext,
                                               pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      rc = pContext->append( BSON( FIELD_NAME_TOTAL << (INT64)1 ) ) ;
      return rc ;
   }

   /*
      _coordCMDGetDatablocks implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDGetDatablocks,
                                      CMD_NAME_GET_DATABLOCKS,
                                      TRUE ) ;
   _coordCMDGetDatablocks::_coordCMDGetDatablocks()
   {
   }

   _coordCMDGetDatablocks::~_coordCMDGetDatablocks()
   {
   }

   INT32 _coordCMDGetDatablocks::generateResult( rtnContext *pContext,
                                                 pmdEDUCB *cb )
   {
      // don't merge data, do nothing
      return SDB_OK ;
   }

   /*
      _coordCMDGetQueryMeta implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDGetQueryMeta,
                                      CMD_NAME_GET_QUERYMETA,
                                      TRUE ) ;
   _coordCMDGetQueryMeta::_coordCMDGetQueryMeta()
   {
   }

   _coordCMDGetQueryMeta::~_coordCMDGetQueryMeta()
   {
   }

   /*
      _coordCMDGetCollectionDetail implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDGetCollectionDetail,
                                      CMD_NAME_GET_CL_DETAIL,
                                      TRUE ) ;
   _coordCMDGetCollectionDetail::_coordCMDGetCollectionDetail()
   {
   }

   _coordCMDGetCollectionDetail::~_coordCMDGetCollectionDetail()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_GET_CL_DETAIL_GENRESULT, "_coordCMDGetCollectionDetail::generateResult" )
   INT32 _coordCMDGetCollectionDetail::generateResult( rtnContext *pContext,
                                                       pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( COORD_GET_CL_DETAIL_GENRESULT ) ;

      ossPoolMap< ossPoolString, clsMainCLMonInfo* > groupName2MainCLInfo ;
      ossPoolMap< ossPoolString, clsMainCLMonInfo* >::iterator iter ;
      ossPoolMap< ossPoolString, ossPoolString > groupName2NodeName ;
      ossPoolMap< ossPoolString, ossPoolString >::iterator nameIter ;

      clsMainCLMonInfo *pNewInfo = NULL ;
      BSONObjBuilder builder ;

      try
      {
         rtnContextBuf buffObj ;
         CHAR clFullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;
         CHAR *separator = NULL ;

         if ( !_cataPtr->isMainCL() )
         {
            rc = SDB_OK ;
            goto done ;
         }

         // Aggregate the main collection detail. One group one record.
         while ( TRUE )
         {
            rc = pContext->getMore( 1, buffObj, cb ) ;
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to get more detail, rc: %d", rc ) ;

            BSONObj boTmp( buffObj.data() ) ;
            BSONObj boDetails = boTmp.getField( FIELD_NAME_DETAILS ).Obj() ;
            ossPoolString groupName ;
            ossPoolString nodeName ;

            monCollection subMonCL ;
            BSONObjIterator it( boDetails ) ;
            INT32 i = 0 ;
            while ( it.more() )
            {
               BSONObj obj = it.next().embeddedObject() ;
               groupName = obj.getField( FIELD_NAME_GROUPNAME ).valuestrsafe() ;
               nodeName = obj.getField( FIELD_NAME_NODE_NAME ).valuestrsafe() ;
               detailedInfo info ;
               rc = monDetailObj2Info( obj, info ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to convert detail BSON to struct, rc: %d", rc );
               subMonCL._details[i] = info ;
               ++i ;
            }

            iter = groupName2MainCLInfo.find( groupName ) ;
            if ( iter == groupName2MainCLInfo.end() )
            {
               pNewInfo = SDB_OSS_NEW clsMainCLMonInfo( _cataPtr->getCatalogSet(), 0 ) ;
               if ( NULL == pNewInfo )
               {
                  rc = SDB_OOM ;
                  PD_RC_CHECK( rc, PDERROR, "No memory to store main cl info" ) ;
               }
               pNewInfo->append( subMonCL ) ;
               groupName2NodeName[ groupName ] = nodeName ;
               groupName2MainCLInfo[ groupName ] = pNewInfo ;
               pNewInfo = NULL ;
            }
            else
            {
               iter->second->append( subMonCL ) ;
            }
         }

         ossStrcpy( clFullName, _cataPtr->getName() ) ;
         separator = ossStrchr( clFullName, '.' ) ;

         for ( iter = groupName2MainCLInfo.begin() ;
               iter != groupName2MainCLInfo.end() ;
               ++iter )
         {
            builder.reset() ;
            builder.append( FIELD_NAME_NAME, clFullName ) ;
            builder.append( FIELD_NAME_UNIQUEID, (INT64) _cataPtr->clUniqueID() ) ;

            *separator = '\0' ;
            builder.append( FIELD_NAME_COLLECTIONSPACE, clFullName ) ;
            *separator = '.' ;

            BSONArrayBuilder subBuilder(
                  builder.subarrayStart( FIELD_NAME_DETAILS ) ) ;
            monCollection mainMonCL ;
            iter->second->get( mainMonCL ) ;
            MON_CL_DETAIL_MAP::iterator monIt = mainMonCL._details.begin() ;
            nameIter = groupName2NodeName.find( iter->first ) ;

            while ( monIt != mainMonCL._details.end() )
            {
               BSONObjBuilder detailBuilder( subBuilder.subobjStart() ) ;
               // append location info
               detailBuilder.append( FIELD_NAME_NODE_NAME, nameIter->second ) ;
               detailBuilder.append( FIELD_NAME_GROUPNAME, nameIter->first ) ;
               // details
               rc = monDetailInfo2Obj( monIt->second, monIt->first, detailBuilder ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to convert cl detail to BSON "
                            "object, rc: %d", rc ) ;
               detailBuilder.done() ;
               ++monIt ;
            }
            subBuilder.done() ;

            rc = pContext->append( builder.done() ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to append cl detail to context, rc: %d", rc ) ;
         }
      }
      catch ( std::bad_alloc &ba )
      {
         rc = SDB_OOM ;
         PD_RC_CHECK( rc, PDERROR, "Out of memory when generating result" ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_RC_CHECK( rc, PDERROR, "Failed to generate detail result. "
                      "Occured unexpected error:%s", e.what() ) ;
      }

   done:
      if ( pNewInfo )
      {
         SDB_OSS_DEL pNewInfo ;
      }
      for ( iter = groupName2MainCLInfo.begin() ;
            iter != groupName2MainCLInfo.end() ;
            ++iter )
      {
         SDB_OSS_DEL iter->second ;
      }
      PD_TRACE_EXITRC ( COORD_GET_CL_DETAIL_GENRESULT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordCMDGetCLStat implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDGetCLStat,
                                      CMD_NAME_GET_CL_STAT,
                                      TRUE ) ;
   _coordCMDGetCLStat::_coordCMDGetCLStat()
   {
   }

   _coordCMDGetCLStat::~_coordCMDGetCLStat()
   {
   }

   // Here we merge two statistics info into one.
   void _coordCMDGetCLStat::_merge ( const collectionStatInfo &from,
                                     collectionStatInfo &to )
   {
      // For "Collection", always be the same. Ignore it.
      // For "TotalDataPages", "TotalDataSize", "SampleRecords", "TotalRecords","AvgNumFields",
      // we count the total.
      if ( from._isDefault )
      {
         to._isDefault = TRUE ;
      }
      if ( from._isExpired )
      {
         to._isExpired =  TRUE ;
      }
      to._avgNumFields += from._avgNumFields ;
      to._sampleRecords += from._sampleRecords ;
      to._totalRecords += from._totalRecords ;
      to._totalDataPages += from._totalDataPages ;
      to._totalDataSize += from._totalDataSize ;

   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_GET_CL_STAT_GENRESULT, "_coordCMDGetCLStat::generateResult" )
   INT32 _coordCMDGetCLStat::generateResult( rtnContext *pContext,
                                             pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_GET_CL_STAT_GENRESULT ) ;

      ossPoolString collectionName ;
      rtnContextBuf buffObj ;
      collectionStatInfo resStat ;
      const BSONObj obj ;
      BSONObjBuilder ob ;
      UINT32 resCount = 0 ;

      // Aggregate all statistics into one.
      while ( TRUE )
      {
         collectionStatInfo newStat ;
         rc = pContext->getMore( 1, buffObj, cb ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get more detail, rc: %d", rc ) ;

         ++resCount ;
         {
            BSONObj boTmp( buffObj.data() ) ;
            rc = monCollectionStatObj2Info( boTmp, newStat ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to initialize CL statistics from BSON, rc: %d", rc ) ;
         }
         if ( resStat.inited() )
         {
            _merge( newStat, resStat ) ;
         }
         else
         {
            resStat = newStat ;
         }
      }

      if ( 0 == resCount )
      {
         SDB_ASSERT( _cataPtr->isMainCL(), "must be main collection" ) ;
         rc = SDB_MAINCL_NOIDX_NOSUB ;
         PD_LOG( PDERROR, "Sub collection is not exist" ) ;
         goto done ;
      }

      // if CL is main_sub_CL
      if ( _cataPtr->isMainCL() )
      {
         resStat.setCollectionName( _cataPtr->getName() ) ;
      }

      // set avgNumFields
      resStat._avgNumFields /= resCount ;

      rc = monCollectionStatInfo2Obj( resStat, ob ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to convert CL statistics to BSON, rc: %d", rc ) ;

      rc = pContext->append( ob.obj() ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to append CL statistics to context, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC ( COORD_GET_CL_STAT_GENRESULT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      Assistant structure to handle index statistics aggregation.
   */
   class _coordIndexStat : public utilPooledObject
   {
      public:
         static INT32 mergeWithoutMCV( const _coordIndexStat &from,
                                       _coordIndexStat &to ) ;

         static INT32 mergeMCV( ossPoolVector< _coordIndexStat > &vec,
                                _coordIndexStat &result ) ;

         static INT32 merge( ossPoolVector< _coordIndexStat > &vec,
                             _coordIndexStat &result ) ;

      public:
         _coordIndexStat() ;

         ~_coordIndexStat() {}

         INT32 fromBSON( const bson::BSONObj &obj ) ;

         INT32 toBSON( bson::BSONObj &obj ) ;

         INT32 refine( const CoordCataInfoPtr &cataPtr ) ;

         BOOLEAN inited() const
         {
            return ( _index[0] != 0 ) ;
         }

      private:
         // 10000 is a reference to the maximum of MCV size of data node
         static const UINT32 MCV_SIZE_LIMIT = 10000 ;

         typedef std::pair< bson::BSONObj, UINT32 >   _coordValRecPair ;
         typedef ossPoolList< _coordValRecPair >      _coordMCVList ;
         typedef std::pair< UINT32, UINT32 >          _coordFrac2Dups ;
         typedef ossPoolMap< UINT32, UINT32 >         _coordFrac2DupsMap ;
         typedef std::pair< _coordMCVList*, _coordMCVList::iterator > _coordListItPair ;

         class _coordListItCmp
         {
         public:
            BOOLEAN operator()( const _coordListItPair l,
                                const _coordListItPair r ) const
            {
               const _coordMCVList::iterator &lIt = l.second ;
               const _coordMCVList::iterator &rIt = r.second ;
               return 0 > ( lIt->first.woCompare( rIt->first, BSONObj(), FALSE ) ) ;
            }
         } ;


      private:
         INT32 _updateTopFrac( const UINT32 finalMCVSize,
                               const UINT32 fracRec,
                               UINT32 &minFracRec,
                               _coordFrac2DupsMap &topFracMap ) ;

         INT32 _compressMCV( const UINT32 finalMCVSize,
                             _coordFrac2DupsMap &topFracMap ) ;

      private:
         CHAR           _collection[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] ;
         CHAR           _index[ IXM_INDEX_NAME_SIZE + 1 ] ;
         BOOLEAN        _isUnique ;
         bson::BSONObj  _keyPattern ;
         CHAR           _statTimestamp[ OSS_TIMESTAMP_STRING_LEN + 1 ] ;
         UINT32         _indexLevels ;
         UINT32         _indexPages ;
         bson::BSONObj  _distinctValNum ;
         bson::BSONObj  _minValue ;
         bson::BSONObj  _maxValue ;
         UINT64         _nullRecords ;
         UINT64         _undefRecords ;
         UINT64         _sampleRecords ;
         UINT64         _totalRecords ;
         _coordMCVList  _mcvList ;
         // Here maintains a counter instead of using std::list::size().
         // Because the low version(<=4.3) std::list::size() iterates all nodes
         // to get this size. That is quite slow!
         UINT32         _mcvListSize ;
         BOOLEAN        _hasMCV ;
         UINT32         _mcvSampleRecords ;
   } ;

   _coordIndexStat::_coordIndexStat()
   {
      ossMemset( _collection, 0, sizeof( _collection ) ) ;
      ossMemset( _index, 0, sizeof( _index ) ) ;
      _isUnique = FALSE ;
      ossMemset( _statTimestamp, 0, sizeof( _statTimestamp ) ) ;
      _indexLevels = 0 ;
      _indexPages = 0 ;
      _nullRecords = 0 ;
      _undefRecords = 0 ;
      _sampleRecords = 0 ;
      _totalRecords = 0 ;
      _mcvListSize = 0 ;
      _hasMCV = FALSE ;
      _mcvSampleRecords = 0 ;
   }

   INT32 _coordIndexStat::fromBSON( const BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      BSONObjIterator iter( obj ) ;
      INT32 nullFrac = 0 ;
      INT32 undefFrac = 0 ;
      BSONElement ele ;

      try
      {
         ele = obj.getField( FIELD_NAME_SAMPLE_RECORDS ) ;
         PD_CHECK( ele.isNumber(), SDB_INVALIDARG, error, PDERROR,
                   "Field '" FIELD_NAME_SAMPLE_RECORDS "' must be number" ) ;
         _sampleRecords = ele.numberLong() ;

         while ( iter.more() )
         {
            ele = iter.next() ;
            if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_COLLECTION ) )
            {
               PD_CHECK( String == ele.type(), SDB_INVALIDARG, error, PDERROR,
                         "Field '" FIELD_NAME_COLLECTION "' must be string" ) ;
               ossStrncpy( _collection, ele.valuestr(),
                           sizeof( _collection ) - 1 ) ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_STAT_TIMESTAMP ) )
            {
               PD_CHECK( String == ele.type(), SDB_INVALIDARG, error, PDERROR,
                         "Field '" FIELD_NAME_STAT_TIMESTAMP "' must be string" ) ;
               ossStrncpy( _statTimestamp, ele.valuestr(),
                           sizeof( _statTimestamp ) - 1 ) ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_INDEX ) )
            {
               PD_CHECK( String == ele.type(), SDB_INVALIDARG, error, PDERROR,
                         "Field '" FIELD_NAME_INDEX "' must be string" ) ;
               ossStrncpy( _index, ele.valuestr(), sizeof( _index ) - 1 ) ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTAL_IDX_LEVELS ) )
            {
               PD_CHECK( ele.isNumber(), SDB_INVALIDARG, error, PDERROR,
                         "Field '" FIELD_NAME_TOTAL_IDX_LEVELS "' must be number" ) ;
               _indexLevels = ele.numberInt() ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTAL_INDEX_PAGES ) )
            {
               PD_CHECK( ele.isNumber(), SDB_INVALIDARG, error, PDERROR,
                         "Field '" FIELD_NAME_TOTAL_INDEX_PAGES "' must be number" ) ;
               _indexPages = ele.numberInt() ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), IXM_FIELD_NAME_UNIQUE1 ) )
            {
               PD_CHECK( Bool == ele.type(), SDB_INVALIDARG, error, PDERROR,
                         "Field '" IXM_FIELD_NAME_UNIQUE1 "' must be bool" ) ;
               _isUnique = ele.boolean() ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_KEY_PATTERN ) )
            {
               PD_CHECK( Object == ele.type(), SDB_INVALIDARG, error, PDERROR,
                         "Field '" FIELD_NAME_KEY_PATTERN "' must be object" ) ;
               _keyPattern = ele.embeddedObject().getOwned() ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTAL_RECORDS ) )
            {
               PD_CHECK( ele.isNumber(), SDB_INVALIDARG, error, PDERROR,
                         "Field '" FIELD_NAME_TOTAL_RECORDS "' must be number" ) ;
               _totalRecords = ele.numberLong() ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_DISTINCT_VAL_NUM ) )
            {
               PD_CHECK( Array == ele.type(), SDB_INVALIDARG, error, PDERROR,
                         "Field '" FIELD_NAME_DISTINCT_VAL_NUM "' must be array" ) ;
               _distinctValNum = ele.embeddedObject().getOwned() ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_MIN_VALUE ) )
            {
               if ( Object == ele.type() )
               {
                  _minValue = ele.embeddedObject().getOwned() ;
               }
               else if ( jstNULL == ele.type() )
               {
                  _minValue = staticNull ;
               }
               else
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "Field '" FIELD_NAME_MIN_VALUE "' must be "
                          "object or null" ) ;
                  goto error ;
               }
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_MAX_VALUE ) )
            {
               if ( Object == ele.type() )
               {
                  _maxValue = ele.embeddedObject().getOwned() ;
               }
               else if ( jstNULL == ele.type() )
               {
                  _maxValue = staticNull ;
               }
               else
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "Field '" FIELD_NAME_MAX_VALUE "' must be "
                          "object or null" ) ;
                  goto error ;
               }
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_NULL_FRAC ) )
            {
               PD_CHECK( ele.isNumber(), SDB_INVALIDARG, error, PDERROR,
                         "Field '" FIELD_NAME_NULL_FRAC "' must be number" ) ;
               nullFrac = ele.numberInt() ;
               _nullRecords =
                     ( _sampleRecords * nullFrac ) / DMS_STAT_FRACTION_SCALE ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_UNDEF_FRAC ) )
            {
               PD_CHECK( ele.isNumber(), SDB_INVALIDARG, error, PDERROR,
                         "Field '" FIELD_NAME_UNDEF_FRAC "' must be number" ) ;
               undefFrac = ele.numberInt() ;
               _undefRecords =
                     ( _sampleRecords * undefFrac ) / DMS_STAT_FRACTION_SCALE ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_MCV ) )
            {
               PD_CHECK( Object == ele.type(), SDB_INVALIDARG, error, PDERROR,
                         "Field '" FIELD_NAME_MCV "' must be object" ) ;
               {
                  BSONObj mcvObj =  ele.embeddedObject() ;
                  BSONObj valueArray =
                        mcvObj.getField( FIELD_NAME_VALUES ).embeddedObject() ;
                  BSONObjIterator valueIt( valueArray ) ;
                  BSONObj fracArray =
                        mcvObj.getField( FIELD_NAME_FRAC ).embeddedObject() ;
                  BSONObjIterator fracIt( fracArray ) ;

                  while ( valueIt.more() && fracIt.more() )
                  {
                     BSONObj value = valueIt.next().embeddedObject().getOwned() ;
                     INT32 frac = fracIt.next().numberInt() ;
                     FLOAT64 recInDouble = ( ( FLOAT64 ) _sampleRecords * frac )
                           / DMS_STAT_FRACTION_SCALE ;
                     UINT32 records = floor( recInDouble + 0.5 ) ;
                     _coordValRecPair pair( value, records ) ;
                     _mcvList.push_back( pair ) ;
                     ++_mcvListSize ;
                  }

                  _hasMCV = TRUE ;
                  _mcvSampleRecords = _sampleRecords ;
               }
            }
         }
      }
      catch (std::exception &e)
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occured: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordIndexStat::toBSON( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      try
      {
         BSONObjBuilder ob ;
         ob.append( FIELD_NAME_COLLECTION, _collection ) ;
         ob.append( FIELD_NAME_INDEX, _index ) ;
         ob.appendBool( IXM_FIELD_NAME_UNIQUE1, _isUnique ) ;
         ob.append( FIELD_NAME_KEY_PATTERN, _keyPattern ) ;
         ob.append( FIELD_NAME_TOTAL_IDX_LEVELS, _indexLevels ) ;
         ob.append( FIELD_NAME_TOTAL_INDEX_PAGES, _indexPages ) ;
         ob.appendArray( FIELD_NAME_DISTINCT_VAL_NUM, _distinctValNum ) ;

         if ( !_minValue.equal( staticNull ) )
         {
            ob.append( FIELD_NAME_MIN_VALUE, _minValue ) ;
         }
         else
         {
            ob.appendNull( FIELD_NAME_MIN_VALUE ) ;
         }

         if ( !_maxValue.equal( staticNull ) )
         {
            ob.append( FIELD_NAME_MAX_VALUE, _maxValue ) ;
         }
         else
         {
            ob.appendNull( FIELD_NAME_MAX_VALUE ) ;
         }

         INT32 nullFrac = 0 ;
         INT32 undefFrac = 0 ;
         if ( _sampleRecords > 0 )
         {
            nullFrac =
                  ( _nullRecords * DMS_STAT_FRACTION_SCALE ) / _sampleRecords ;
            undefFrac =
                  ( _undefRecords * DMS_STAT_FRACTION_SCALE ) / _sampleRecords ;
         }
         ob.append( FIELD_NAME_NULL_FRAC, nullFrac ) ;
         ob.append( FIELD_NAME_UNDEF_FRAC, undefFrac ) ;

         if ( _hasMCV )
         {
            BSONObjBuilder mcvOB( ob.subobjStart( FIELD_NAME_MCV ) ) ;

            _coordMCVList::iterator it ;
            BSONArrayBuilder valueAB( mcvOB.subarrayStart( FIELD_NAME_VALUES ) ) ;
            for ( it = _mcvList.begin(); it != _mcvList.end(); ++it )
            {
               valueAB.append( it->first ) ;
            }
            valueAB.done() ;

            BSONArrayBuilder fracAB( mcvOB.subarrayStart( FIELD_NAME_FRAC ) ) ;
            for ( it = _mcvList.begin(); it != _mcvList.end(); ++it )
            {
               if ( _mcvSampleRecords > 0 )
               {
                  INT32 frac = ( it->second * DMS_STAT_FRACTION_SCALE ) /
                               _mcvSampleRecords ;
                  frac = OSS_MAX( frac, 1 ) ;
                  fracAB.append( frac ) ;
               }
               else
               {
                  fracAB.append( 0 ) ;
               }
            }
            fracAB.done() ;

            mcvOB.done() ;
         }

         ob.append( FIELD_NAME_SAMPLE_RECORDS, ( INT64 )_sampleRecords ) ;
         ob.append( FIELD_NAME_TOTAL_RECORDS, ( INT64 )_totalRecords ) ;
         ob.append( FIELD_NAME_STAT_TIMESTAMP, _statTimestamp ) ;
         obj = ob.obj() ;
      }
      catch ( std::bad_alloc &ba )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "No memory to build BSON object" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordIndexStat::merge( ossPoolVector< _coordIndexStat > &vec,
                                 _coordIndexStat &result )
   {
      INT32 rc = SDB_OK ;
      try
      {
         result = vec[ 0 ] ;
         for ( UINT32 i = 1; i < vec.size(); ++i )
         {
            rc = mergeWithoutMCV( vec[ i ], result ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to merge statistics, rc: %d", rc ) ;

            if ( vec[ i ]._hasMCV )
            {
               result._hasMCV = TRUE ;
            }
         }

         if ( result._hasMCV )
         {
            result._mcvList.clear() ;
            result._mcvSampleRecords = 0 ;

            rc = mergeMCV( vec, result ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to merge MCV, rc: %d", rc ) ;
         }

      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occured: %e", e.what() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   // Here we merge two statistics info into one. However, some field of it
   // can't easily be counted. We'll choose the max or min instead.
   // This merge doesn't handle the field "MCV"
   INT32 _coordIndexStat::mergeWithoutMCV( const _coordIndexStat &from,
                                           _coordIndexStat &to )
   {
      INT32 rc = SDB_OK ;
      try
      {
         // For "Collection", "Unique", "KeyPattern" and
         // "Index", they always be the same. Ignore them.
         // For "GroupName" and "NodeName", they are useless. Ignore them.

         // For "MaxValue" and "MinValue", we get the global max one and min one.
         if ( from._maxValue > to._maxValue )
         {
            to._maxValue = from._maxValue.getOwned() ;
         }
         if ( !from._minValue.equal( staticNull ) &&
              ( to._minValue.equal( staticNull ) ||
                from._minValue < to._minValue ) )
         {
            to._minValue = from._minValue.getOwned() ;
         }

         // For "StatTimestamp", we get the newest one.
         if ( ossStrcmp( from._statTimestamp, to._statTimestamp ) > 0 )
         {
            ossStrncpy( to._statTimestamp, from._statTimestamp,
                        sizeof( to._statTimestamp ) - 1 ) ;
         }

         // For "TotalIndexLevels", we get the max one.
         if ( from._indexLevels > to._indexLevels )
         {
            to._indexLevels = from._indexLevels ;
         }

         // For "NullFrac" and "UndefFrac", we count the total null records,
         // and finally calculate the fraction again.
         to._nullRecords += from._nullRecords ;
         to._undefRecords += from._undefRecords ;

         // For "TotalIndexPages", "SampleRecords", "TotalRecords", and
         // "DistinctValNum", we count the total.
         to._sampleRecords += from._sampleRecords ;
         to._totalRecords += from._totalRecords ;
         to._indexPages += from._indexPages ;
         {
            BSONObjIterator toIt( to._distinctValNum ) ;
            BSONObjIterator fromIt( from._distinctValNum ) ;
            while ( toIt.more() && fromIt.more() )
            {
               BSONElement toEle = toIt.next() ;
               BSONElement fromEle = fromIt.next() ;
               INT64 total = toEle.numberLong() + fromEle.numberLong() ;
               SDB_ASSERT( toEle.type() == NumberLong,
                           "DistinctValNum must be INT64" ) ;
               INT64 *pNum = (INT64 *) const_cast<char *>( toEle.value() ) ;
               *pNum = total ;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occured: %e", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // For "MCV", union the result.
   INT32 _coordIndexStat::mergeMCV( ossPoolVector< _coordIndexStat > &vec,
                                    _coordIndexStat &result )
   {
      INT32 rc = SDB_OK ;
      try
      {
         SDB_ASSERT( !vec.empty(), "Vector shouldn't be empty" ) ;

         _coordListItCmp cmp ;
         _utilMinHeap< _coordListItPair, _coordListItCmp > heap( cmp ) ;
         _coordMCVList::iterator it ;
         // Limits on the number of MCV sample can be set by node configuration "statmcvlimit"
         UINT32 statMCVLimit = pmdGetKRCB()->getOptionCB()->getStatMCVLimit() ;

         for ( UINT32 i = 0; i < vec.size(); ++i )
         {
            _coordMCVList *pList = &vec[ i ]._mcvList ;
            if ( pList->empty() )
            {
               continue ;
            }
            // Here set a limit to prevent the cost from becoming too expensive.
            if ( result._mcvSampleRecords > statMCVLimit )
            {
               pList->clear() ;
               vec[ i ]._mcvSampleRecords = 0 ;
               continue ;
            }

            it = pList->begin() ;
            _coordListItPair p( pList, it ) ;
            rc = heap.push( p ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to make heap, rc: %d", rc ) ;

            result._mcvSampleRecords += vec[ i ]._mcvSampleRecords ;
         }

         while ( heap.more() )
         {
            _coordListItPair p ;
            _coordMCVList::reverse_iterator last ;

            rc = heap.pop( p ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to pop from heap, rc: %d", rc ) ;

            it = p.second ;
            last = result._mcvList.rbegin() ;
            if ( last != result._mcvList.rend() &&
                 0 == last->first.woCompare( it->first, BSONObj(), FALSE ) )
            {
               last->second += it->second ;
            }
            else
            {
               result._mcvList.push_back( *it ) ;
               ++result._mcvListSize ;
            }

            it = p.first->erase( it ) ;
            if ( it != p.first->end() )
            {
               p.second = it ;
               heap.push( p ) ;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occured: %e", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /**
      1. Refine DistinctValNum by MCV info;
      2. Compress MCV info, to avoid the large result;
      3. Fix the name as main-CL name if it's main-sub-CL
   */
   INT32 _coordIndexStat::refine( const CoordCataInfoPtr &cataPtr )
   {
      INT32 rc = SDB_OK ;
      INT64 *distinctValNum = NULL ;
      try
      {
         INT32 keyFieldNum = 0 ;
         INT32 i = 0 ;
         ossPoolList< _coordValRecPair >::iterator mcvIt ;
         BSONObj *pLastValue = NULL ;

         _coordFrac2DupsMap topFracMap ;
         UINT32 finalMCVSize = OSS_MIN( _mcvListSize, MCV_SIZE_LIMIT ) ;
         UINT32 minFracRec = 0 ;

         if ( cataPtr->isMainCL() )
         {
            ossStrncpy( _collection, cataPtr->getName(), sizeof( _collection ) ) ;
         }

         if ( _mcvList.empty() )
         {
            goto done ;
         }

         keyFieldNum = _keyPattern.nFields() ;

         distinctValNum =
               (INT64 *) SDB_POOL_ALLOC( keyFieldNum * sizeof( INT64 ) ) ;
         if ( !distinctValNum )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "No memory to allocate the array" ) ;
            goto error ;
         }

         for ( i = 0 ; i < keyFieldNum ; ++i )
         {
            distinctValNum[i] = 1 ;
         }

         mcvIt = _mcvList.begin() ;
         pLastValue = &mcvIt->first ;
         for ( ; mcvIt != _mcvList.end(); ++mcvIt )
         {
            i = 0 ;
            BSONObj *pCurrValue = &mcvIt->first ;
            BSONObjIterator currIter( *pCurrValue ) ;
            BSONObjIterator lastIter( *pLastValue ) ;

            while ( currIter.more() )
            {
               BSONElement currEle = currIter.next() ;
               BSONElement lastEle = lastIter.next() ;
               if ( currEle.woCompare( lastEle, FALSE ) != 0 )
               {
                  break ;
               }
               ++i ;
            }

            while ( i < keyFieldNum )
            {
               ++distinctValNum[i] ;
               ++i ;
            }

            pLastValue = pCurrValue ;

            rc = _updateTopFrac( finalMCVSize, mcvIt->second, minFracRec,
                                 topFracMap ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to update top fraction, rc: %d",
                         rc ) ;
         }

         {
            BSONArrayBuilder ab( 32 );
            for (i = 0; i < keyFieldNum; ++i) {
               ab.append( distinctValNum[i] );
            }
            _distinctValNum = ab.arr();
         }

         rc = _compressMCV( finalMCVSize, topFracMap ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to compress MCV, rc: %d", rc ) ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occured: %e", e.what() ) ;
         goto error ;
      }
   done:
      if ( distinctValNum )
      {
         SDB_POOL_FREE( distinctValNum ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   /**
      If MCV is too big, find out the elements with top fraction to
      cut away the low fraction elements later. We only maintain the top N
      fraction (N = finalMCVSize) to save the memory.
   */
   INT32 _coordIndexStat::_updateTopFrac( const UINT32 finalMCVSize,
                                          const UINT32 fracRec,
                                          UINT32 &minFracRec,
                                          _coordFrac2DupsMap &topFracMap )
   {
      INT32 rc = SDB_OK ;
      try
      {
         if ( _mcvListSize <= finalMCVSize )
         {
            goto done ;
         }
         if ( fracRec < minFracRec )
         {
            goto done ;
         }
         {
            _coordFrac2Dups value( fracRec, 1 ) ;
            std::pair< _coordFrac2DupsMap::iterator, bool > ret ;
            ret = topFracMap.insert( value ) ;
            if ( !ret.second )
            {
               ret.first->second += 1 ;
            }
            if ( topFracMap.size() > finalMCVSize )
            {
               topFracMap.erase( topFracMap.begin() ) ;
               minFracRec = topFracMap.begin()->first ;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occured: %e", e.what() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /**
      If MCV is too big, we would cut it away until the size less than
      finalMCVSize. There are 2 principle:
      1. More frequent, more important. The biggest fraction go first.
      2. If fraction is the same, select them averagely.
   */
   INT32 _coordIndexStat::_compressMCV( const UINT32 finalMCVSize,
                                        _coordFrac2DupsMap &topFracMap )
   {
      INT32 rc = SDB_OK ;
      try
      {
         _coordFrac2DupsMap::reverse_iterator mapRit ;
         UINT32 currSize = 0 ;
         ossPoolList< _coordValRecPair >::iterator mcvIt ;

         if ( _mcvListSize  <= finalMCVSize )
         {
            goto done ;
         }

         for ( mapRit = topFracMap.rbegin();
               mapRit != topFracMap.rend();
               ++mapRit )
         {
            currSize += mapRit->second ;
            if ( currSize >= finalMCVSize )
            {
               break ;
            }
         }

         if ( currSize >= finalMCVSize )
         {
            UINT32 cutSize = currSize - finalMCVSize ;
            UINT32 boundFracRec = mapRit->second ;
            FLOAT64 stepLength =
                ((FLOAT64) boundFracRec / ( boundFracRec - cutSize )) ;
            FLOAT64 stepIter = 0 ;
            UINT32 cutBound = mapRit->first ;

            mcvIt = _mcvList.begin() ;
            INT32 i = 0 ;
            while ( mcvIt != _mcvList.end() &&
                    _mcvListSize > finalMCVSize )
            {
               UINT32 fracRec = mcvIt->second ;
               BOOLEAN shouldCut = FALSE ;
               if ( fracRec < cutBound )
               {
                  shouldCut = TRUE ;
               }
               else if ( fracRec == cutBound )
               {
                  if ( i != ( floor( stepIter + 0.5 ) ) )
                  {
                     shouldCut = TRUE ;
                  }
                  else
                  {
                     shouldCut = FALSE ;
                     stepIter += stepLength ;
                  }
                  ++i ;
               }

               if ( shouldCut )
               {
                  mcvIt = _mcvList.erase( mcvIt ) ;
                  --_mcvListSize ;
               }
               else
               {
                  ++mcvIt ;
               }
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occured: %e", e.what() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordCMDGetIndexStat implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDGetIndexStat,
                                      CMD_NAME_GET_INDEX_STAT,
                                      TRUE ) ;
   _coordCMDGetIndexStat::_coordCMDGetIndexStat()
   {
   }

   _coordCMDGetIndexStat::~_coordCMDGetIndexStat()
   {
   }

   INT32 _coordCMDGetIndexStat::_getResultCount( UINT32 &resCount, pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK;
      if ( _cataPtr->isMainCL() )
      {
         CoordSubCLlist subCLList ;
         CoordSubCLlist::iterator it ;
         rc = _cataPtr->getSubCLList( subCLList ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get sub cl list, rc: %d", rc ) ;

         for ( it = subCLList.begin(); it != subCLList.end(); ++it )
         {
            _coordCataSel subSel ;
            rc = subSel.bind( _pResource, it->c_str(), cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get sub cl[%s] catalog, "
                         "rc: %d", it->c_str(), rc ) ;
            resCount += subSel.getCataPtr()->getGroupNum() ;
         }
      }
      else
      {
         resCount = _cataPtr->getGroupNum() ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_GET_INDEX_STAT_GENRESULT, "_coordCMDGetIndexStat::generateResult" )
   INT32 _coordCMDGetIndexStat::generateResult( rtnContext *pContext,
                                                pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( COORD_GET_INDEX_STAT_GENRESULT ) ;

      rtnContextBuf buffObj ;
      _coordIndexStat resStat ;
      BSONObj obj ;
      UINT32 resCount = 0 ;
      UINT32 currCount = 0 ;
      ossPoolVector< _coordIndexStat > vec ;

      try
      {
         // calculate the expect sharding count to allocate vector
         rc = _getResultCount( resCount, cb ) ;
         vec.reserve( resCount ) ;

         // Aggregate all statistics into one.
         while ( TRUE )
         {
            rc = pContext->getMore( 1, buffObj, cb ) ;
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to get more detail, rc: %d", rc ) ;

            vec.push_back( _coordIndexStat() ) ;
            _coordIndexStat &stat = vec[ currCount ] ;
            BSONObj boTmp( buffObj.data() ) ;
            rc = stat.fromBSON( boTmp ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to initialize statistics "
                         "from BSON, rc: %d", rc ) ;
            ++currCount ;
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occured: %e", e.what() ) ;
         goto error ;
      }

      if ( 0 == currCount ) // Nothing was returned.
      {
         goto done ;
      }

      rc = _coordIndexStat::merge( vec, resStat ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to merge index statistics, rc: %d", rc ) ;

      rc = resStat.refine( _cataPtr ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to refine index statistics, rc: %d", rc ) ;

      rc = resStat.toBSON( obj ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to convert index statistics to BSON, rc: %d", rc ) ;

      rc = pContext->append( obj ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to append cl detail to context, rc: %d", rc ) ;
   done:
      PD_TRACE_EXITRC ( COORD_GET_INDEX_STAT_GENRESULT, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}