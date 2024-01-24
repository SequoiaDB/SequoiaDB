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

   Source File Name = optPlanPath.hpp

   Descriptive Name = Optimizer Plan Path Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Optimizer component. This file contains structure for access
   plan, which is indicating how to run a given query.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/14/2017  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OPTPLANPATH_HPP__
#define OPTPLANPATH_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "optCommon.hpp"
#include "optPlanNode.hpp"
#include "ossMemPool.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{

   enum OPT_EXPLAIN_PATH_TYPE
   {
      OPT_EXPLAIN_EMPTY_PATH,
      OPT_EXPLAIN_SCAN_PATH,
      OPT_EXPLAIN_MERGE_PATH,
      OPT_EXPLAIN_COORD_PATH
   } ;

   /*
      _optPlanPath define
    */
   class _optPlanPath ;
   typedef class _optPlanPath optPlanPath ;

   class _optPlanPath : public SDBObject
   {
      public :
         _optPlanPath () ;

         virtual ~_optPlanPath () ;

         OSS_INLINE const optPlanNode * getRootNode () const
         {
            return _pRootNode ;
         }

         OSS_INLINE optPlanNode * getRootNode ()
         {
            return _pRootNode ;
         }

         OSS_INLINE BOOLEAN isEmpty () const
         {
            return _pRootNode == NULL ;
         }

         OSS_INLINE UINT64 getEstTotalCost () const
         {
            return _pRootNode ?
                   _pRootNode->getEstTotalCost() : OSS_UINT64_MAX ;
         }

         OSS_INLINE virtual void clearPath ()
         {
            _deleteNodes () ;
         }

      protected :
         void _deleteNodes () ;

         void _setRootNode ( optPlanNode * pNode ) ;

         void _setPath ( const optPlanPath & path, BOOLEAN takeOwned ) ;

      protected :
         mutable BOOLEAN      _ownedPath ;
         optPlanNode *        _pRootNode ;
   } ;

   /*
      _optScanPath define
    */
   class _optScanPath ;
   typedef class _optScanPath optScanPath ;

   // Store searched paths during optimization
   typedef ossPoolList< optScanPath> optScanPathList ;

   class _optScanPath : public _optPlanPath
   {
      public :
         _optScanPath () ;

         _optScanPath ( const _optScanPath & path ) ;

         virtual ~_optScanPath () ;

         optScanPath & operator = ( const optScanPath & path ) ;

         OSS_INLINE const optScanNode * getScanNode () const
         {
            return _pScanNode ;
         }

         OSS_INLINE optScanNode * getScanNode ()
         {
            return _pScanNode ;
         }

         OSS_INLINE BOOLEAN isSortRequired () const
         {
            return _sortRequired ;
         }

         INT32 createTbScan ( const CHAR * pCollection,
                              const rtnQueryOptions & queryOptions,
                              optAccessPlanHelper & planHelper,
                              optCollectionStat * collectionStat ) ;

         INT32 createIxScan ( const CHAR * pCollection,
                              const ixmIndexCB & indexCB,
                              const rtnQueryOptions & queryOptions,
                              optAccessPlanHelper & planHelper,
                              OPT_PLAN_PATH_PRIORITY priority,
                              optCollectionStat * collectionStat,
                              optIndexStat * indexStat ) ;

         INT32 createSortNode ( const rtnQueryOptions & queryOptions,
                                UINT64 sortBufferSize ) ;

         OSS_INLINE BOOLEAN isCandidate () const
         {
            return NULL != _pScanNode && _pScanNode->isCandidate() ;
         }

         // indicates the plan is a good candidate in default priority
         OSS_INLINE BOOLEAN isGoodCandidate() const
         {
            return NULL != _pScanNode && _pScanNode->isGoodCandidate() ;
         }

         OSS_INLINE BSONObj getIXBound () const
         {
            return ( NULL != _pScanNode ) ? _pScanNode->getIXBound() :
                                            BSONObj() ;
         }

         OSS_INLINE BOOLEAN isMatchAll () const
         {
            return ( NULL != _pScanNode ) ? _pScanNode->isMatchAll() : FALSE ;
         }

         OSS_INLINE BOOLEAN isNeedMatch () const
         {
            return ( NULL != _pScanNode ) ? _pScanNode->isNeedMatch() : FALSE ;
         }

         OSS_INLINE BOOLEAN isIndexCover () const
         {
            return ( NULL != _pScanNode ) ? _pScanNode->isIndexCover() : FALSE ;
         }

         OSS_INLINE BOOLEAN notArray () const
         {
            return ( NULL != _pScanNode ) ? _pScanNode->notArray() : FALSE ;
         }

         OSS_INLINE INT32 getDirection () const
         {
            return ( NULL != _pScanNode ) ? _pScanNode->getDirection() : 1 ;
         }

         OSS_INLINE dmsExtentID getIndexExtID () const
         {
            return ( NULL != _pScanNode ) ? _pScanNode->getIndexExtID() :
                                            DMS_INVALID_EXTENT ;
         }

         OSS_INLINE dmsExtentID getIndexLID () const
         {
            return ( NULL != _pScanNode ) ? _pScanNode->getIndexLID() :
                                            DMS_INVALID_EXTENT ;
         }

         OSS_INLINE BSONObj getKeyPattern () const
         {
            return ( NULL != _pScanNode ) ? _pScanNode->getKeyPattern() :
                                            BSONObj() ;
         }

         OSS_INLINE double getSelectivity () const
         {
            return isIxScan() ? _pScanNode->getScanSelectivity() :
                               ( ( NULL != _pScanNode ) ?
                                 _pScanNode->getMthSelctivity() :
                                 OPT_MTH_DEFAULT_SELECTIVITY ) ;
         }

         OSS_INLINE UINT32 getMatchedFields () const
         {
            return ( NULL != _pScanNode ) ? _pScanNode->getMatchedFields() :
                                            0 ;
         }

         OSS_INLINE const CHAR *getIndexName () const
         {
            return ( NULL != _pScanNode ) ? _pScanNode->getIndexName() : "" ;
         }

         OSS_INLINE optScanType getScanType () const
         {
            return ( NULL != _pScanNode ) ? _pScanNode->getScanType() :
                                            UNKNOWNSCAN ;
         }

         OSS_INLINE UINT32 getInputPages () const
         {
            return ( NULL != _pScanNode ) ? _pScanNode->getInputPages() : 0 ;
         }

         OSS_INLINE UINT32 getInputRecordSize() const
         {
            return ( NULL != _pScanNode ) ? _pScanNode->getInputRecordSize() : 0 ;
         }

         OSS_INLINE UINT64 getInputRecords () const
         {
            return ( NULL != _pScanNode ) ? _pScanNode->getInputRecords() : 0 ;
         }

         OSS_INLINE BOOLEAN isEstimatedFromStat () const
         {
            return NULL != _pScanNode && _pScanNode->isEstimatedFromStat() ;
         }

         INT32 evaluate ( const rtnQueryOptions & options,
                          UINT64 sortBufferSize ) ;

         void setPath ( const optScanPath & path, BOOLEAN takeOwned ) ;

         INT32 copyPath ( const optScanPath & path ) ;

         INT32 clearScanInfo () ;

         OSS_INLINE virtual void clearPath ()
         {
            _optPlanPath::clearPath() ;
            _pScanNode = NULL ;
            _sortRequired = FALSE ;
         }

         OSS_INLINE BOOLEAN isTbScan () const
         {
            return ( NULL != _pScanNode &&
                     OPT_PLAN_TB_SCAN == _pScanNode->getType() ) ;
         }

         OSS_INLINE BOOLEAN isIxScan () const
         {
            return ( NULL != _pScanNode &&
                     OPT_PLAN_IX_SCAN == _pScanNode->getType() ) ;
         }

         INT32 toBSONSearchPath ( BSONArrayBuilder & builder,
                                  BOOLEAN isUsed,
                                  BOOLEAN needEvaluate ) const ;

         OSS_INLINE BOOLEAN isBetterPath ( const _optScanPath & otherPlan ) const
         {
            // If current plan is a candidate plan, and matched one of
            // below case, make current plan a better plan
            // 1. other plan is empty
            // 2. other plan has more cost
            // 3. other plan has the same cost but less matched fields
            // 4. other plan has the same cost and matched fields, but is sort
            //    required
            return ( isCandidate() &&
                     ( otherPlan.isEmpty() ||
                       getEstTotalCost() < otherPlan.getEstTotalCost() ||
                       ( getEstTotalCost() == otherPlan.getEstTotalCost() &&
                         ( getMatchedFields() > otherPlan.getMatchedFields() ||
                           ( getMatchedFields() == otherPlan.getMatchedFields() &&
                             !isSortRequired() && otherPlan.isSortRequired() ) ) ) ) ) ;
         }

      protected :
         INT32 _setScanNode ( optPlanNode * pNode ) ;

      protected :
         optScanNode *     _pScanNode ;
         BOOLEAN           _sortRequired ;
   } ;

   /*
      _optExplainPath define
    */
   class _optExplainPath ;
   typedef class _optExplainPath optExplainPath ;

   class _optExplainPath : public _optPlanPath
   {
      public :
         _optExplainPath () ;

         virtual ~_optExplainPath () ;

         virtual OPT_EXPLAIN_PATH_TYPE getPathType () const = 0 ;

         OSS_INLINE const CHAR * getCollectionName () const
         {
            return _clFullName ;
         }

         void setCollectionName ( const CHAR * clFullName ) ;

         virtual void clearPath () ;

         virtual INT32 toBSON ( BSONObjBuilder & builder,
                                const rtnExplainOptions &expOptions ) const ;

         INT32 setExplainStart ( pmdEDUCB * cb ) ;

         INT32 setExplainEnd ( rtnContext * context, pmdEDUCB * cb ) ;

         INT32 toBSONExplainInfo ( BSONObjBuilder & builder,
                                   UINT16 mask ) const ;

      protected :
         CHAR                 _clFullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] ;

         /// info before explain
         UINT64               _beginDataRead ;
         UINT64               _beginIndexRead ;
         ossTimestamp         _beginTime ;
         FLOAT64              _beginUsrCpu ;
         FLOAT64              _beginSysCpu ;

         /// info after explain
         UINT64               _endDataRead ;
         UINT64               _endIndexRead ;
         ossTimestamp         _endTime ;
         FLOAT64              _endUsrCpu ;
         FLOAT64              _endSysCpu ;
   } ;

   /*
      _optExplainScanPath define
    */
   class _optExplainScanPath ;
   typedef class _optExplainScanPath optExplainScanPath ;

   class _optExplainScanPath : public _optExplainPath,
                               public _mthMatchConfigHolder,
                               public _optAccessPlanConfigHolder
   {
      public :
         _optExplainScanPath () ;

         virtual ~_optExplainScanPath () ;

         OSS_INLINE virtual OPT_EXPLAIN_PATH_TYPE getPathType () const
         {
            return OPT_EXPLAIN_SCAN_PATH ;
         }

         OSS_INLINE BOOLEAN getIsNewPlan () const
         {
            return _isNewPlan ;
         }

         OSS_INLINE void setIsNewPlan ( BOOLEAN isNewPlan )
         {
            _isNewPlan = isNewPlan ;
         }

         OSS_INLINE void setIsMainCLPlan ( BOOLEAN isMainCLPlan )
         {
            _isMainCLPlan = isMainCLPlan ;
         }

         OSS_INLINE OPT_PLAN_CACHE_LEVEL getCacheLevel () const
         {
            return _cacheLevel ;
         }

         OSS_INLINE void setCacheLevel ( OPT_PLAN_CACHE_LEVEL cacheLevel )
         {
            _cacheLevel = cacheLevel ;
         }

         OSS_INLINE void setParameters ( const BSONObj & parameters )
         {
            _parameters = parameters ;
         }

         OSS_INLINE void setSearchPaths ( const optScanPathList * searchPaths )
         {
            _searchPaths = searchPaths ;
         }

         OSS_INLINE void setSearchOptions ( BOOLEAN needSearch,
                                            BOOLEAN needEvaluate )
         {
            _needSearch = needSearch ;
            _needEvaluate = needEvaluate ;
         }

         virtual void clearPath () ;

         INT32 copyScanPath ( const optScanPath & scanPath,
                              const rtnContext * context ) ;

      protected :
         INT32 _copyTbScanNode ( const optPlanNode * srcNode,
                                 const rtnContext * srcContext,
                                 optPlanNode ** dstNode ) ;

         INT32 _copyIxScanNode ( const optPlanNode * srcNode,
                                 const rtnContext * srcContext,
                                 optPlanNode ** dstNode ) ;

         INT32 _copySortNode ( const optPlanNode * srcNode,
                               const rtnContext * srcContext,
                               optPlanNode ** dstNode,
                               const rtnContext ** subContext ) ;

         INT32 _setScanNode ( optPlanNode * pNode ) ;

      public :
         virtual INT32 toBSON ( BSONObjBuilder & builder,
                                const rtnExplainOptions &expOptions ) const ;

         INT32 toBSONBasic ( BSONObjBuilder & builder,
                             const rtnExplainOptions &expOptions ) const ;

      protected :
         INT32 _toBSONPlanInfo ( BSONObjBuilder & builder,
                                 const rtnExplainOptions &expOptions ) const ;

         INT32 _toBSONSearchParameters ( BSONObjBuilder & builder ) const ;

         INT32 _toBSONSearchConstants ( BSONObjBuilder & builder ) const ;

         INT32 _toBSONSearchOptions ( BSONObjBuilder & builder ) const ;

         INT32 _toBSONSearchInputs ( BSONObjBuilder & builder ) const ;

      protected :
         optScanNode *        _pScanNode ;
         BOOLEAN              _isNewPlan ;
         BOOLEAN              _isMainCLPlan ;
         OPT_PLAN_CACHE_LEVEL _cacheLevel ;
         BSONObj              _parameters ;
         const optScanPathList * _searchPaths ;
         BOOLEAN              _needSearch ;
         BOOLEAN              _needEvaluate ;
   } ;

   /*
      _optExplainMergePathBase define
    */
   class _optExplainMergePathBase ;
   typedef class _optExplainMergePathBase optExplainMergePathBase ;

   class _optExplainMergePathBase : public _optExplainPath
   {
      public :
         _optExplainMergePathBase () ;

         virtual ~_optExplainMergePathBase () ;

         virtual void clearPath () ;

         OSS_INLINE INT32 addChildExplain ( const BSONObj & childExplain,
                                            const ossTickDelta & queryTime,
                                            const ossTickDelta & waitTime,
                                            BOOLEAN needParse,
                                            BOOLEAN needChildExplain,
                                            const rtnExplainOptions &expOptions )
         {
            SDB_ASSERT( NULL != _pMergeNode, "merge node is invalid" ) ;
            return _pMergeNode->addChildExplain(
                        childExplain, queryTime, waitTime,
                        needParse, needChildExplain, expOptions ) ;
         }

         OSS_INLINE optExplainResultList & getChildExplains ()
         {
            SDB_ASSERT( NULL != _pMergeNode, "merge node is invalid" ) ;
            return _pMergeNode->getChildExplains() ;
         }

         OSS_INLINE virtual INT32 sortChildExplains ()
         {
            return SDB_OK ;
         }

         OSS_INLINE const monContextCB & getContextMonitor () const
         {
            SDB_ASSERT( NULL != _pMergeNode, "_pMergeNode is invalid" ) ;
            return _pMergeNode->getRuntimeMonitor() ;
         }

         OSS_INLINE monContextCB & getContextMonitor ()
         {
            SDB_ASSERT( NULL != _pMergeNode, "_pMergeNode is invalid" ) ;
            return _pMergeNode->getRuntimeMonitor() ;
         }

         OSS_INLINE optMergeNodeBase * getMergeNode ()
         {
            return _pMergeNode ;
         }

         virtual INT32 evaluate () ;

      protected :
         INT32 _setMergeNode ( optPlanNode * pNode ) ;

      protected :
         optMergeNodeBase * _pMergeNode ;
   } ;

   /*
      _optExplainMergePath define
    */
   class _optExplainMergePath ;
   typedef class _optExplainMergePath optExplainMergePath ;

   class _optExplainMergePath : public _optExplainMergePathBase
   {
      public :
         _optExplainMergePath () ;

         virtual ~_optExplainMergePath () ;

         OSS_INLINE virtual OPT_EXPLAIN_PATH_TYPE getPathType () const
         {
            return OPT_EXPLAIN_MERGE_PATH ;
         }

      public :
         INT32 createMergePath ( const rtnContext * context ) ;

      protected :
         INT32 _createMergeNode ( const rtnContext * srcContext,
                                  optPlanNode ** dstNode ) ;

      public :
         INT32 toSimpleBSON ( BSONObjBuilder & builder ) ;
   } ;

   /*
      _optExplainCoordPath define
    */
   class _optExplainCoordPath ;
   typedef class _optExplainCoordPath optExplainCoordPath ;

   class _optExplainCoordPath : public _optExplainMergePathBase
   {
      public :
         _optExplainCoordPath () ;

         virtual ~_optExplainCoordPath () ;

         OSS_INLINE virtual OPT_EXPLAIN_PATH_TYPE getPathType () const
         {
            return OPT_EXPLAIN_COORD_PATH ;
         }

      public :
         INT32 createCoordPath ( const rtnContext * context ) ;

         OSS_INLINE virtual INT32 sortChildExplains () ;

      protected :
         INT32 _createCoordNode ( const rtnContext * srcContext,
                                  optPlanNode ** dstNode ) ;
   } ;

}

#endif //OPTPLANPATH_HPP__
