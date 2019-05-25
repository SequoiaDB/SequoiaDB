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

   Source File Name = optPlanNode.hpp

   Descriptive Name = Optimizer Plan Node Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Optimizer component. This file contains structure for access
   plan, which is indicating how to run a given query.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/27/2017  HGM Split from optPlanPath.cpp

   Last Changed =

*******************************************************************************/

#ifndef OPTPLANNODE_HPP__
#define OPTPLANNODE_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "optCommon.hpp"
#include "optAccessPlanHelper.hpp"
#include "optStatUnit.hpp"
#include "monCB.hpp"
#include "rtnQueryOptions.hpp"
#include "utilAllocator.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{
   #define OPT_NODE_ALLOCATOR_SIZE        ( 1024 )

   enum OPT_PLAN_NODE_TYPE
   {
      OPT_PLAN_TB_SCAN,
      OPT_PLAN_IX_SCAN,
      OPT_PLAN_SORT,
      OPT_PLAN_MERGE,
      OPT_PLAN_COORD
   } ;

   #define OPT_PLAN_NODE_NAME_TBSCAN      "TBSCAN"
   #define OPT_PLAN_NODE_NAME_IXSCAN      "IXSCAN"
   #define OPT_PLAN_NODE_NAME_SORT        "SORT"
   #define OPT_PLAN_NODE_NAME_MERGE       "MERGE"
   #define OPT_PLAN_NODE_NAME_COORD       "COORD-MERGE"

   enum optScanType
   {
      TBSCAN = 0,
      IXSCAN,
      UNKNOWNSCAN
   } ;

   enum OPT_PLAN_PATH_PRIORITY
   {
      OPT_PLAN_IDX_REQUIRED,

      OPT_PLAN_SORTED_IDX_REQUIRED,

      OPT_PLAN_IDX_PREFERRED,

      OPT_PLAN_DEFAULT_PRIORITY
   } ;

   class _rtnContextBase ;
   typedef class _rtnContextBase rtnContext ;

   class _pmdEDUCB ;
   typedef class _pmdEDUCB pmdEDUCB ;

   typedef _utilAllocator< OPT_NODE_ALLOCATOR_SIZE > optPlanAllocator ;

   typedef _utilList< BSONObj > optExplainResultList ;

   typedef struct _optChildNodeSummary
   {
      _optChildNodeSummary ()
      : _name( NULL ),
        _estTotalCost( 0 ),
        _queryTime( 0.0 ),
        _waitTime( 0.0 )
      {
      }

      _optChildNodeSummary & operator = ( const _optChildNodeSummary & summary )
      {
         _name = summary._name ;
         _estTotalCost = summary._estTotalCost ;
         _queryTime = summary._queryTime ;
         _waitTime = summary._waitTime ;
         return (*this) ;
      }

      const CHAR *   _name ;
      UINT64         _estTotalCost ;
      double         _queryTime ;
      double         _waitTime ;
   } optChildNodeSummary ;

   typedef _utilList< optChildNodeSummary > optChildSummaryList ;

   /*
      _optPlanNode define
    */
   class _optPlanNode ;
   typedef class _optPlanNode optPlanNode ;

   typedef _utilList< optPlanNode * > optPlanNodeList ;

   class _optPlanNode : public SDBObject
   {
      public :
         _optPlanNode () ;

         _optPlanNode ( const optPlanNode & node,
                        const rtnContext * context ) ;

         virtual ~_optPlanNode () ;

         void * operator new ( size_t size, optPlanAllocator *pAllocator,
                               std::nothrow_t ) ;

         void operator delete ( void *p ) ;

         void operator delete ( void *p, optPlanAllocator *pAllocator,
                                std::nothrow_t ) ;

         virtual void release ( optPlanAllocator * pAllocator ) = 0 ;

         OSS_INLINE optPlanNodeList & getChildNodes ()
         {
            return _childNodes ;
         }

         OSS_INLINE const optPlanNodeList & getChildNodes () const
         {
            return _childNodes ;
         }

         void addChildNode ( _optPlanNode *pChildNode ) ;

         void deleteChildNodes ( optPlanAllocator *pAllocator ) ;

         OSS_INLINE virtual UINT32 getChildNodeNum () const
         {
            return (UINT32)_childNodes.size() ;
         }

         OSS_INLINE virtual BOOLEAN hasChildNodes () const
         {
            return getChildNodeNum() > 0 ;
         }

         OSS_INLINE BOOLEAN needReEvaluate () const
         {
            return _returnOptions.getSkip() > 0 ||
                   _returnOptions.getLimit() >= 0 ;
         }

         OSS_INLINE UINT64 getEstStartCost () const
         {
            return _estStartCost ;
         }

         OSS_INLINE UINT64 getEstRunCost () const
         {
            return _estRunCost ;
         }

         OSS_INLINE UINT64 getEstTotalCost () const
         {
            return _estTotalCost ;
         }

         OSS_INLINE UINT64 getOutputRecords () const
         {
            return _outputRecords ;
         }

         OSS_INLINE UINT32 getOutputRecordSize () const
         {
            return _outputRecordSize ;
         }

         OSS_INLINE UINT32 getOutputNumFields () const
         {
            return _outputNumFields ;
         }

         OSS_INLINE BOOLEAN isSorted () const
         {
            return _sorted ;
         }

         OSS_INLINE const monContextCB & getRuntimeMonitor () const
         {
            return _runtimeMonitor ;
         }

         OSS_INLINE monContextCB & getRuntimeMonitor ()
         {
            return _runtimeMonitor ;
         }

         OSS_INLINE void setRuntimeMonitor ( const monContextCB & monitor )
         {
            _runtimeMonitor = monitor ;
         }

         OSS_INLINE const rtnReturnOptions & getReturnOptions () const
         {
            return _returnOptions ;
         }

         OSS_INLINE rtnReturnOptions & getReturnOptions ()
         {
            return _returnOptions ;
         }

         OSS_INLINE virtual BOOLEAN isBlockNode () const
         {
            return FALSE ;
         }

         virtual OPT_PLAN_NODE_TYPE getType () const = 0 ;

         virtual const CHAR * getName () const = 0 ;

         virtual void evaluate () = 0 ;

         INT32 toBSON ( BSONObjBuilder & builder,
                        BOOLEAN needExpand,
                        BOOLEAN needFlatten,
                        UINT16 mask ) const ;

         INT32 fromBSON ( const BSONObj & object, UINT16 mask ) ;

         OSS_INLINE virtual INT32 toBSONEvaluation ( BSONObjBuilder & builder ) const
         {
            return SDB_OK ;
         }

      protected :
         void _setReturnSelector ( const rtnContext * context ) ;

         UINT64 _evaluateOutputRecords ( UINT64 inputRecords,
                                         UINT64 &outputSkipRecords ) ;

         OSS_INLINE virtual UINT16 _getExplainEstimateMask () const
         {
            return OPT_NODE_EXPLAIN_MASK_OUTPUT ;
         }

         OSS_INLINE virtual const CHAR * _getBSONChildArrayName () const
         {
            return OPT_FIELD_CHILD_NODES ;
         }

         virtual INT32 _toBSONBasic ( BSONObjBuilder & builder,
                                      UINT16 mask ) const = 0 ;

         virtual INT32 _fromBSONBasic ( const BSONObj & object ) = 0 ;

         virtual INT32 _toBSONEstimate ( BSONObjBuilder &builder,
                                         UINT16 mask ) const ;

         virtual INT32 _toBSONEstimateImpl ( BSONObjBuilder & builder ) const ;

         virtual INT32 _fromBSONEstimate ( const BSONObj & object,
                                           UINT16 mask ) ;

         virtual INT32 _fromBSONEstimateImpl ( const BSONObj & object ) ;

         virtual INT32 _toBSONEstimateInput ( BSONObjBuilder & builder ) const
         {
            return SDB_OK ;
         }

         virtual INT32 _toBSONEstimateFilter ( BSONObjBuilder & builder ) const
         {
            return SDB_OK ;
         }

         virtual INT32 _toBSONEstimateOutput ( BSONObjBuilder & builder ) const ;

         virtual INT32 _fromBSONEstimateOutput ( const BSONObj & object ) ;

         virtual INT32 _toBSONRun ( BSONObjBuilder & builder ) const ;

         virtual INT32 _toBSONRunImpl ( BSONObjBuilder & builder ) const ;

         virtual INT32 _toBSONChildNodes ( BSONObjBuilder & builder,
                                           BOOLEAN needExpand,
                                           UINT16 mask ) const ;

         OSS_INLINE virtual INT32 _toBSONChildNodesImpl ( BSONArrayBuilder & builder,
                                                          BOOLEAN needExpand,
                                                          UINT16 mask ) const
         {
            return SDB_OK ;
         }

         virtual INT32 _toBSONReturnOptions ( BSONObjBuilder & builder ) const ;

         virtual INT32 _fromBSONReturnOptions ( const BSONObj & object ) ;

         INT32 _toBSONFieldEval ( BSONObjBuilder & builder,
                                  const CHAR * outputName,
                                  const CHAR * outputFormula,
                                  const CHAR * outputEvaluate,
                                  INT32 outputResult ) const ;

         INT32 _toBSONFieldEval ( BSONObjBuilder & builder,
                                  const CHAR * outputName,
                                  const CHAR * outputFormula,
                                  const CHAR * outputEvaluate,
                                  INT64 outputResult ) const ;

         INT32 _toBSONTotalCostEval ( BSONObjBuilder & builder ) const ;

      protected :
         optPlanNodeList   _childNodes ;

         UINT64            _estStartCost ;

         UINT64            _estRunCost ;

         UINT64            _estTotalCost ;

         UINT64            _outputRecords ;

         UINT32            _outputRecordSize ;

         UINT32            _outputNumFields ;

         UINT64            _estIOCost ;
         UINT64            _estCPUCost ;

         BOOLEAN           _sorted ;

         rtnReturnOptions  _returnOptions ;
         monContextCB      _runtimeMonitor ;
   } ;

   /*
      _optScanNode define
    */
   class _optScanNode ;
   typedef class _optScanNode optScanNode ;

   class _optScanNode : public _optPlanNode
   {
      public :
         _optScanNode () ;

         _optScanNode ( const CHAR * pCollection,
                        INT32 estCacheSize ) ;

         _optScanNode ( const optScanNode & node,
                        const rtnContext * context ) ;

         virtual ~_optScanNode () ;

         OSS_INLINE const CHAR *getCollection () const
         {
            return _pCollection ;
         }

         OSS_INLINE void setCollection ( const CHAR *pCollection )
         {
            _pCollection = pCollection ;
         }

         OSS_INLINE UINT64 getInputRecords () const
         {
            return _inputRecords ;
         }

         OSS_INLINE UINT32 getInputPages () const
         {
            return _inputPages ;
         }

         OSS_INLINE UINT32 getInputRecordSize () const
         {
            return _inputRecordSize ;
         }

         OSS_INLINE INT32 getCostThreshold () const
         {
            return _estCacheSize ;
         }

         OSS_INLINE BOOLEAN needEvalIOCost () const
         {
            return ( _estCacheSize >= 0 &&
                     _inputPages > (UINT32)_estCacheSize ) ;
         }

         OSS_INLINE double getMthSelctivity () const
         {
            return _mthSelectivity ;
         }

         OSS_INLINE UINT32 getMthCPUCost () const
         {
            return _mthCPUCost ;
         }

         OSS_INLINE UINT32 getPageSize () const
         {
            return _pageSize ;
         }

         OSS_INLINE BOOLEAN isCandidate () const
         {
            return _isCandidate ;
         }

         OSS_INLINE virtual INT32 getDirection () const
         {
            return 1 ;
         }

         OSS_INLINE virtual BOOLEAN isMatchAll () const
         {
            return FALSE ;
         }

         OSS_INLINE virtual void setNeedMatch ( BOOLEAN needMatch )
         {
            _needMatch = needMatch ;
         }

         OSS_INLINE virtual BOOLEAN isNeedMatch () const
         {
            return _needMatch ;
         }

         OSS_INLINE virtual dmsExtentID getIndexExtID () const
         {
            return DMS_INVALID_EXTENT ;
         }

         OSS_INLINE virtual dmsExtentID getIndexLID () const
         {
            return DMS_INVALID_EXTENT ;
         }

         OSS_INLINE virtual BSONObj getKeyPattern () const
         {
            return BSONObj() ;
         }

         OSS_INLINE virtual BSONObj getMatcher () const
         {
            return _runtimeMatcher ;
         }

         OSS_INLINE virtual BSONObj getIXBound () const
         {
            return BSONObj() ;
         }

         OSS_INLINE virtual void setIXBound ( const BSONObj & ixBound )
         {
         }

         OSS_INLINE virtual double getScanSelectivity () const
         {
            return OPT_MTH_DEFAULT_SELECTIVITY ;
         }

         OSS_INLINE virtual double getPredSelectivity () const
         {
            return OPT_MTH_DEFAULT_SELECTIVITY ;
         }

         OSS_INLINE virtual const CHAR *getIndexName () const
         {
            return "" ;
         }

         virtual optScanType getScanType () const = 0 ;

      protected :
         void _preEvaluate ( const rtnQueryOptions &queryOptions,
                             optAccessPlanHelper &planHelper,
                             optCollectionStat *collectionStat ) ;

         void _evalOutRecordSize () ;

         OSS_INLINE UINT64 _evalScanIOCost ( UINT32 unitIOCost,
                                             UINT32 scanPages ) const
         {
            return needEvalIOCost() ?
                   ( unitIOCost * scanPages * _evalNormalizedPageRate() ) :
                   0 ;
         }

         OSS_INLINE UINT32 _evalNormalizedPageRate () const
         {
            return _pageSize / DMS_PAGE_SIZE_BASE ;
         }

      public :
         virtual INT32 toBSONCLStatInfo ( BSONObjBuilder & builder ) const ;

         OSS_INLINE virtual INT32 toBSONIXStatInfo ( BSONObjBuilder & builder ) const
         {
            return SDB_OK ;
         }

      protected :
         OSS_INLINE virtual UINT16 _getExplainEstimateMask () const
         {
            return ( OPT_NODE_EXPLAIN_MASK_OUTPUT |
                     OPT_NODE_EXPLAIN_MASK_FILTER |
                     OPT_NODE_EXPLAIN_MASK_INPUT ) ;
         }

         virtual INT32 _toBSONEstimateImpl ( BSONObjBuilder &builder ) const ;

         virtual INT32 _toBSONEstimateInput ( BSONObjBuilder & builder ) const ;

         virtual INT32 _toBSONEstimateFilter ( BSONObjBuilder & builder ) const ;

         virtual INT32 _toBSONRunImpl ( BSONObjBuilder & builder ) const ;

         INT32 _toBSONRunCostEval ( BSONObjBuilder & builder,
                                    BOOLEAN needIOCost ) const ;

         INT32 _toBSONPagesEval ( BSONObjBuilder & builder,
                                  const CHAR * outputName,
                                  const CHAR * inputName,
                                  const CHAR * selName,
                                  UINT32 inputValue,
                                  double selectivity,
                                  UINT32 outputValue ) const ;

         INT32 _toBSONRecordsEval ( BSONObjBuilder & builder,
                                    const CHAR * outputName,
                                    const CHAR * inputName,
                                    const CHAR * selName,
                                    UINT64 inputValue,
                                    double selectivity,
                                    UINT64 outputValue ) const ;

      protected :
         const CHAR *      _pCollection ;

         UINT32            _pageSize ;

         INT32             _estCacheSize ;

         double            _mthSelectivity ;

         UINT32            _mthCPUCost ;

         BOOLEAN           _needMatch ;

         UINT64            _inputRecords ;

         UINT32            _inputPages ;

         UINT32            _inputNumFields ;

         UINT32            _inputRecordSize ;

         UINT64            _readRecords ;

         UINT32            _readPages ;

         BOOLEAN           _clFromStat ;
         UINT64            _clStatTime ;

         BOOLEAN           _isCandidate ;

         BSONObj           _runtimeMatcher ;
   } ;

   /*
      _optTbScanNode define
    */
   class _optTbScanNode ;
   typedef class _optTbScanNode optTbScanNode ;

   class _optTbScanNode : public _optScanNode
   {
      public :
         _optTbScanNode () ;

         _optTbScanNode ( const CHAR * pCollection,
                          INT32 estCacheSize ) ;

         _optTbScanNode ( const optTbScanNode & node,
                          const rtnContext * context ) ;

         virtual ~_optTbScanNode () ;

         virtual void release ( optPlanAllocator * pAllocator ) ;

         OSS_INLINE virtual OPT_PLAN_NODE_TYPE getType () const
         {
            return OPT_PLAN_TB_SCAN ;
         }

         OSS_INLINE virtual const CHAR * getName () const
         {
            return OPT_PLAN_NODE_NAME_TBSCAN ;
         }

         OSS_INLINE virtual optScanType getScanType () const
         {
            return TBSCAN ;
         }

      public :
         void preEvaluate ( const rtnQueryOptions & queryOptions,
                            optAccessPlanHelper & planHelper,
                            optCollectionStat * collectionStat ) ;

         virtual void evaluate () ;

      protected :
         void _evalNoReturnRecords () ;

         void _evalNoReturnOptions () ;

         void _evalWithReturnOptions ( UINT64 noLimitRecords,
                                       UINT64 returnSkipRecords ) ;

      public :
         virtual INT32 toBSONEvaluation ( BSONObjBuilder & builder ) const ;

      protected :
         virtual INT32 _toBSONBasic ( BSONObjBuilder & builder,
                                      UINT16 mask ) const ;

         virtual INT32 _fromBSONBasic ( const BSONObj & object ) ;

         INT32 _toBSONIOCostEval ( BSONObjBuilder & builder ) const ;

         INT32 _toBSONCPUCostEval ( BSONObjBuilder & builder ) const ;

         INT32 _toBSONStartCostEval ( BSONObjBuilder & builder ) const ;

         INT32 _toBSONOutputRecordsEval ( BSONObjBuilder & builder ) const ;
   } ;

   /*
      _optIxScanNode define
    */
   class _optIxScanNode ;
   typedef class _optIxScanNode optIxScanNode ;

   class _optIxScanNode : public _optScanNode
   {
      public :
         _optIxScanNode () ;

         _optIxScanNode ( const CHAR * pCollection,
                          const ixmIndexCB & indexCB,
                          INT32 estCacheSize ) ;

         _optIxScanNode ( const optIxScanNode & node,
                          const rtnContext * context ) ;

         virtual ~_optIxScanNode () ;

         virtual void release ( optPlanAllocator * pAllocator ) ;

         OSS_INLINE virtual OPT_PLAN_NODE_TYPE getType () const
         {
            return OPT_PLAN_IX_SCAN ;
         }

         OSS_INLINE virtual const CHAR * getName () const
         {
            return OPT_PLAN_NODE_NAME_IXSCAN ;
         }

         OSS_INLINE virtual const CHAR * getIndexName () const
         {
            return _pIndexName ;
         }

         OSS_INLINE virtual INT32 getDirection () const
         {
            return _direction ;
         }

         OSS_INLINE virtual BOOLEAN isMatchAll () const
         {
            return _matchAll ;
         }

         OSS_INLINE virtual dmsExtentID getIndexExtID () const
         {
            return _indexExtID ;
         }

         OSS_INLINE void setIndexExtID ( dmsExtentID indexExtID )
         {
            _indexExtID = indexExtID ;
         }

         OSS_INLINE virtual dmsExtentID getIndexLID () const
         {
            return _indexLID ;
         }

         OSS_INLINE void setIndexLID ( dmsExtentID indexLID )
         {
            _indexLID = indexLID ;
         }

         OSS_INLINE virtual BSONObj getKeyPattern () const
         {
            return _keyPattern ;
         }

         OSS_INLINE virtual BSONObj getIXBound () const
         {
            return _runtimeIXBound ;
         }

         OSS_INLINE virtual void setIXBound ( const BSONObj & ixBound )
         {
            _runtimeIXBound = ixBound ;
         }

         OSS_INLINE virtual optScanType getScanType () const
         {
            return IXSCAN ;
         }

         OSS_INLINE virtual double getScanSelectivity () const
         {
            return _scanSelectivity ;
         }

         OSS_INLINE virtual double getPredSelectivity () const
         {
            return _predSelectivity ;
         }

      public :
         void preEvaluate ( const rtnQueryOptions & queryOptions,
                            optAccessPlanHelper & planHelper,
                            OPT_PLAN_PATH_PRIORITY priority,
                            optCollectionStat * collectionStat,
                            optIndexStat * indexStat ) ;

         virtual void evaluate () ;

      protected :
         void _evalPredEstimation ( optAccessPlanHelper & planHelper,
                                    const BSONObj & boOrder,
                                    BOOLEAN isBestIndex,
                                    const optIndexStat * indexStat ) ;

         void _evalNoReturnRecords () ;

         void _evalNoReturnOptions () ;

         void _evalWithReturnOptions ( UINT64 noLimitRecords,
                                       UINT64 returnSkipRecords ) ;

      public :
         virtual INT32 toBSONEvaluation ( BSONObjBuilder & builder ) const ;

         virtual INT32 toBSONIXStatInfo ( BSONObjBuilder & builder ) const ;

      protected :
         virtual INT32 _toBSONBasic ( BSONObjBuilder & builder,
                                      UINT16 mask ) const ;

         virtual INT32 _fromBSONBasic ( const BSONObj & object ) ;

         virtual INT32 _toBSONEstimateInput ( BSONObjBuilder & builder ) const ;

         virtual INT32 _toBSONEstimateFilter ( BSONObjBuilder & builder ) const ;

         virtual INT32 _toBSONRunImpl ( BSONObjBuilder & builder ) const ;

         INT32 _toBSONIOCostEval ( BSONObjBuilder & builder ) const ;

         INT32 _toBSONCPUCostEval ( BSONObjBuilder & builder ) const ;

         INT32 _toBSONStartCostEval ( BSONObjBuilder & builder ) const ;

         INT32 _toBSONOutputRecordsEval ( BSONObjBuilder & builder ) const ;

      protected :
         CHAR              _pIndexName [ IXM_INDEX_NAME_SIZE + 1 ] ;

         INT32             _direction ;

         BOOLEAN           _matchAll ;

         UINT32            _matchedFields ;

         UINT32            _matchedOrders ;

         dmsExtentID       _indexExtID ;
         dmsExtentID       _indexLID ;
         BSONObj           _keyPattern ;

         double            _scanSelectivity ;

         double            _predSelectivity ;

         UINT32            _predCPUCost ;

         UINT32            _indexPages ;

         UINT32            _indexLevels ;

         UINT64            _idxReadRecords ;

         UINT32            _idxReadPages ;

         BOOLEAN           _ixFromStat ;
         UINT64            _ixStatTime ;

         BSONObj           _runtimeIXBound ;
   } ;

   /*
      _optSortNode define
    */
   class _optSortNode ;
   typedef class _optSortNode optSortNode ;

   class _optSortNode : public _optPlanNode
   {
      protected :
         enum OPT_PLAN_SORT_TYPE
         {
            OPT_PLAN_IN_MEM_SORT,
            OPT_PLAN_EXT_SORT
         } ;

      public :
         _optSortNode () ;

         _optSortNode ( const optSortNode & node,
                        const rtnContext * context ) ;

         virtual ~_optSortNode () ;

         virtual void release ( optPlanAllocator * pAllocator ) ;

         OSS_INLINE virtual OPT_PLAN_NODE_TYPE getType () const
         {
            return OPT_PLAN_SORT ;
         }

         OSS_INLINE virtual const CHAR * getName () const
         {
            return OPT_PLAN_NODE_NAME_SORT ;
         }

         OSS_INLINE virtual BOOLEAN isBlockNode () const
         {
            return TRUE ;
         }

      public :
         void preEvaluate ( const rtnQueryOptions & queryOptions,
                            UINT64 sortBufferSize ) ;

         virtual void evaluate () ;

      protected :
         OSS_INLINE UINT64 _getInputSize () const
         {
            SDB_ASSERT( _childNodes.size() == 1, "Should have only one child node" ) ;
            const _optPlanNode *childNode = _childNodes.front() ;
            return ( childNode->getOutputRecords() *
                     childNode->getOutputRecordSize() ) ;
         }

         OSS_INLINE UINT32 _getInputPages ( UINT64 inputSize ) const
         {
            return OPT_ROUND_NUM( (UINT32)ceil( (double)inputSize /
                                                (double)DMS_PAGE_SIZE_BASE ) ) ;
         }

         OSS_INLINE UINT64 _getInputTotalCost () const
         {
            SDB_ASSERT( _childNodes.size() == 1, "Should have only one child node" ) ;
            const _optPlanNode *childNode = _childNodes.front() ;
            return childNode->getEstTotalCost() ;
         }

      public :
         virtual INT32 toBSONEvaluation ( BSONObjBuilder & builder ) const ;

      protected :
         virtual INT32 _toBSONBasic ( BSONObjBuilder & builder,
                                      UINT16 mask ) const ;

         virtual INT32 _fromBSONBasic ( const BSONObj & object ) ;

         virtual INT32 _toBSONEstimateImpl ( BSONObjBuilder & builder ) const ;

         virtual INT32 _toBSONRunImpl ( BSONObjBuilder & builder ) const ;

         virtual INT32 _toBSONChildNodesImpl ( BSONArrayBuilder & builder,
                                               BOOLEAN needExpand,
                                               UINT16 mask ) const ;

         INT32 _toBSONInputSizeEval ( BSONObjBuilder & builder,
                                      UINT64 records,
                                      UINT32 recordSize,
                                      UINT64 totalSize ) const ;

         INT32 _toBSONInputPageEval ( BSONObjBuilder & builder,
                                      UINT64 totalSize,
                                      UINT32 totalPages ) const ;

         INT32 _toBSONIOCostEval ( BSONObjBuilder & builder,
                                   UINT32 totalPages ) const ;

         INT32 _toBSONCPUCostEval ( BSONObjBuilder & builder,
                                    UINT64 records ) const ;

         INT32 _toBSONStartCostEval ( BSONObjBuilder & builder ) const ;

         INT32 _toBSONRunCostEval ( BSONObjBuilder & builder,
                                    UINT64 records ) const ;

         INT32 _toBSONOutputRecordsEval ( BSONObjBuilder & builder ) const ;

      protected :
         OPT_PLAN_SORT_TYPE   _estSortType ;
         OPT_PLAN_SORT_TYPE   _runtimeSortType ;
         BSONObj              _orderBy ;
         UINT64               _sortBufferSize ;
   } ;

   /*
      _optMergeNodeBase define
    */
   class _optMergeNodeBase ;
   typedef class _optMergeNodeBase optMergeNodeBase ;

   class _optMergeNodeBase : public _optPlanNode
   {
      public :
         _optMergeNodeBase () ;

         _optMergeNodeBase ( const rtnContext * context ) ;

         virtual ~_optMergeNodeBase () ;

         OSS_INLINE virtual UINT32 getChildNodeNum () const
         {
            return _childExplainList.size() ;
         }

         OSS_INLINE optExplainResultList & getChildExplains ()
         {
            return _childExplainList ;
         }

         INT32 addChildExplain ( optPlanAllocator * pAllocator,
                                 const BSONObj & childExplain,
                                 const ossTickDelta & queryTime,
                                 const ossTickDelta & waitTime,
                                 BOOLEAN needParse,
                                 BOOLEAN needChildExplain,
                                 UINT16 mask ) ;

      public :
         virtual void evaluate () ;

      protected :
         void _evaluateOutput () ;

         virtual void _evaluateEmptyRunCost () = 0 ;

         virtual void _evaluateOrderedRunCost () = 0 ;

         virtual void _evaluateNormalRunCost () = 0 ;

      public :
         virtual INT32 toSimpleBSON ( BSONObjBuilder & builder ) ;

      protected :
         virtual const CHAR * _getChildExplainName () const = 0 ;

         virtual const CHAR * _getChildListName () const = 0 ;

         virtual const CHAR * _getChildNumName () const = 0 ;

         virtual INT32 _toBSONBasic ( BSONObjBuilder & builder,
                                      UINT16 mask ) const ;

         virtual INT32 _fromBSONBasic ( const BSONObj & object ) ;

         virtual INT32 _toBSONChildList ( BSONObjBuilder & builder,
                                          UINT16 mask ) const ;

         INT32 _toBSONMergeChildNodes ( BSONArrayBuilder & builder,
                                        BOOLEAN needExpand,
                                        BOOLEAN needNodeInfo ) const ;

      protected :
         optChildSummaryList  _childSummary ;
         optExplainResultList _childExplainList ;
         optExplainResultList _childBackupList ;
         BOOLEAN              _needReorder ;
         BSONObj              _orderBy ;
         UINT64               _outputSkipRecords ;
         UINT64               _inputRecords ;
   } ;

   /*
      _optMainCLMergeNode define
    */
   class _optMainCLMergeNode ;
   typedef class _optMainCLMergeNode optMainCLMergeNode ;

   class _optMainCLMergeNode : public _optMergeNodeBase
   {
      public :
         _optMainCLMergeNode () ;

         _optMainCLMergeNode ( const rtnContext * context ) ;

         virtual ~_optMainCLMergeNode () ;

         virtual void release ( optPlanAllocator * pAllocator ) ;

         OSS_INLINE virtual OPT_PLAN_NODE_TYPE getType () const
         {
            return OPT_PLAN_MERGE ;
         }

         OSS_INLINE virtual const CHAR * getName () const
         {
            return OPT_PLAN_NODE_NAME_MERGE ;
         }

      protected :
         virtual void _evaluateEmptyRunCost () ;

         virtual void _evaluateOrderedRunCost () ;

         virtual void _evaluateNormalRunCost () ;

      protected :
         OSS_INLINE virtual const CHAR * _getBSONChildArrayName () const
         {
            return OPT_FIELD_SUB_COLLECTIONS ;
         }

         OSS_INLINE virtual const CHAR * _getChildExplainName () const
         {
            return OPT_FIELD_COLLECTION ;
         }

         OSS_INLINE virtual const CHAR * _getChildListName () const
         {
            return OPT_FIELD_SUB_COLLECTION_LIST ;
         }

         OSS_INLINE virtual const CHAR * _getChildNumName () const
         {
            return OPT_FILED_SUB_COLLECTION_NUM ;
         }

         OSS_INLINE virtual INT32 _toBSONChildNodesImpl ( BSONArrayBuilder & builder,
                                                          BOOLEAN needExpand,
                                                          UINT16 mask ) const
         {
            return _toBSONMergeChildNodes( builder, needExpand, FALSE ) ;
         }
   } ;

   /*
      _optCoordMergeNode define
    */
   class _optCoordMergeNode ;
   typedef class _optCoordMergeNode optCoordMergeNode ;

   class _optCoordMergeNode : public _optMergeNodeBase
   {
      public :
         _optCoordMergeNode () ;

         _optCoordMergeNode ( const rtnContext * context ) ;

         virtual ~_optCoordMergeNode () ;

         virtual void release ( optPlanAllocator * pAllocator ) ;

         OSS_INLINE virtual OPT_PLAN_NODE_TYPE getType () const
         {
            return OPT_PLAN_COORD ;
         }

         OSS_INLINE virtual const CHAR * getName () const
         {
            return OPT_PLAN_NODE_NAME_COORD ;
         }

      protected :
         virtual void _evaluateEmptyRunCost () ;

         virtual void _evaluateOrderedRunCost () ;

         virtual void _evaluateNormalRunCost () ;

      protected :
         OSS_INLINE virtual const CHAR * _getChildExplainName () const
         {
            return OPT_FIELD_NODE_NAME ;
         }

         OSS_INLINE virtual const CHAR * _getChildListName () const
         {
            return OPT_FIELD_DATA_NODE_LIST ;
         }

         OSS_INLINE virtual const CHAR * _getChildNumName () const
         {
            return OPT_FIELD_DATA_NODE_NUM ;
         }

         virtual INT32 _toBSONRunImpl ( BSONObjBuilder & builder ) const ;

         OSS_INLINE virtual INT32 _toBSONChildNodesImpl ( BSONArrayBuilder & builder,
                                                          BOOLEAN needExpand,
                                                          UINT16 mask ) const
         {
            return _toBSONMergeChildNodes( builder, needExpand, TRUE ) ;
         }
   } ;

}

#endif //OPTPLANNODE_HPP__
