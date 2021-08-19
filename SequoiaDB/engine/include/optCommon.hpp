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

   Source File Name = optCommon.hpp

   Descriptive Name = Optimizer Plan Common Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Optimizer component. This file contains common structure for
   plan, which is indicating how to run a given query.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/14/2017  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OPTCOMMON_HPP__
#define OPTCOMMON_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "msgDef.hpp"

namespace engine
{

   #define OPT_PLAN_MIN_CACHE_BUCKETS        ( 0 )
   #define OPT_PLAN_DEF_CACHE_BUCKETS        ( 500 )
   #define OPT_PLAN_MAX_CACHE_BUCKETS        ( 4096 )

   // When index scan is best enough, its cost is smaller than 1/10 of table
   // scan
   #define OPT_IDX_PREFERRED_RATE            ( 10 )

   // Maximum number of candidate plans
   #define OPT_MAX_CANDIDATE_COUNT           ( 5 )

   // Rate to convert IO cost to final cost
   #define OPT_IO_CPU_RATE                   ( 2000 )

   // CPU cost to extract a record from data page
   #define OPT_RECORD_CPU_COST               ( 4 )

   // CPU cost to extract a index item from index page
   #define OPT_IDX_CPU_COST                  ( 2 )

   // Base CPU cost to process a operator
   #define OPT_OPTR_BASE_CPU_COST            ( 1 )

   // IO Cost to sequential scan a 4k-size page
   #define OPT_SEQ_SCAN_IO_COST              ( 1 )

   // IO Cost to randomly scan a 4k-size page
   #define OPT_RANDOM_SCAN_IO_COST           ( 10 )

   #define OPT_DEF_COST_THRESHOLD            ( OPT_RANDOM_SCAN_IO_COST * 2 )

   // IO COST to sequential write a 4k-size page
   #define OPT_SEQ_WRITE_IO_COST             ( 2 )

   // Rate to convert cost to ms
   #define OPT_COST_TO_MS                    ( 0.0005 )

   // Rate to convert cost to sec
   #define OPT_COST_TO_SEC                   ( OPT_COST_TO_MS * 0.001 )

   // Default start cost of table scan
   #define OPT_TBSCAN_DEFAULT_START_COST     ( 0 )

   // Default start cost of indes scan
   #define OPT_IXSCAN_DEFAULT_START_COST     ( 0 )

   // Threshold selectivity of candidate index scan plans
   #define OPT_PRED_THRESHOLD_SELECTIVITY    ( 0.1 )

   // Default selectivity
   #define OPT_DEF_SELECTIVITY               ( 0.3333333333333333 )

   // Default selectivity of matcher
   #define OPT_MTH_DEFAULT_SELECTIVITY       ( 1.0 )

   // Default selectivity of operators in matcher
   #define OPT_MTH_OPTR_DEFAULT_SELECTIVITY  ( OPT_DEF_SELECTIVITY )

   // Default CPU cost of matcher
   #define OPT_MTH_DEFAULT_CPU_COST          ( 0 )

   // Base CPU cost to process a operator in matcher
   #define OPT_MTH_OPTR_BASE_CPU_COST        ( OPT_OPTR_BASE_CPU_COST )

   // CPU cost to extract a field from BSON
   #define OPT_MTH_FIELD_EXTRACT_CPU_COST    ( OPT_OPTR_BASE_CPU_COST * 2 )

   // CPU cost to process $regex in matcher
   #define OPT_MTH_REGEX_CPU_COST            ( OPT_MTH_OPTR_BASE_CPU_COST * 10 )

   // CPU cost to process a function in matcher
   #define OPT_MTH_FUNC_DEF_CPU_COST         ( OPT_MTH_OPTR_BASE_CPU_COST * 2 )

   // Default selectivity of a predicate
   #define OPT_PRED_DEFAULT_SELECTIVITY      ( 1.0 )

   // Default CPU cost of predicate
   #define OPT_PRED_DEFAULT_CPU_COST         ( 0 )

   // Default selectivity of a valid predicate
   #define OPT_PRED_DEF_SELECTIVITY          ( OPT_DEF_SELECTIVITY )

   // Default selectivity of a range predicate
   #define OPT_PRED_RANGE_DEF_SELECTIVITY    ( 0.05 )

   // Default selectivity of a $et predicate
   #define OPT_PRED_EQ_DEF_SELECTIVITY       ( 0.005 )

   // Mininum selectivity of a range predicate
   // NOTE: should be larger than $eq
   #define OPT_PRED_RANGE_MIN_SELECTIVITY    ( OPT_PRED_EQ_DEF_SELECTIVITY * 2 )
   #define OPT_PRED_GTORLT_MIN_SELECTIVITY   ( OPT_PRED_EQ_DEF_SELECTIVITY * 3 )

   #define OPT_ROUND( x, min, max )          ( OSS_MIN( OSS_MAX( ( x ), ( min ) ), ( max ) ) )

   // Selectivity should between 0.0 and 1.0
   #define OPT_ROUND_SELECTIVITY( x )        OPT_ROUND( x, 0.0, 1.0 )

   // Numbers ( number of records, pages ) should be larger than 1
   #define OPT_ROUND_NUM( x )              ( OSS_MAX( 1, ( x ) ) )
   #define OPT_ROUND_NUM_DEF( x, def )     ( OSS_MAX( ( def ) , ( x ) ) )

   #define OPT_LOG2( x )                     ( log( x ) / 0.693147180559945 )

   // Compare BSON numbers between -99999999.9 and 99999999.9
   #define OPT_BSON_NUM_MAX                  ( 99999999.9 )
   #define OPT_BSON_NUM_MIN                  ( -99999999.9 )
   #define OPT_ROUND_BSON_NUM( x )           OPT_ROUND( x, OPT_BSON_NUM_MIN, OPT_BSON_NUM_MAX )

   // Compare first 20 characters in BSON strings between ' ' to 127
   #define OPT_BSON_STR_MIN_LEN              ( 20 )
   #define OPT_BSON_STR_MIN                  ( (UINT8)' ' )
   #define OPT_BSON_STR_MAX                  ( 127 )

   /*
      OPT_PLAN_CACHE_LEVEL define
    */
   enum OPT_PLAN_CACHE_LEVEL
   {
      OPT_PLAN_NOCACHE = 0,
      OPT_PLAN_ORIGINAL,
      OPT_PLAN_NORMALIZED,
      OPT_PLAN_PARAMETERIZED,
      OPT_PLAN_FUZZYOPTR
   } ;

   /*
      Explain field masks
    */
   #define OPT_NODE_EXPLAIN_MASK_NONE        ( 0x0000 )
   #define OPT_NODE_EXPLAIN_MASK_ESTIMATE    ( 0x0001 )
   #define OPT_NODE_EXPLAIN_MASK_RUN         ( 0x0002 )
   #define OPT_NODE_EXPLAIN_MASK_INPUT       ( 0x0010 )
   #define OPT_NODE_EXPLAIN_MASK_FILTER      ( 0x0020 )
   #define OPT_NODE_EXPLAIN_MASK_OUTPUT      ( 0x0040 )
   #define OPT_NODE_EXPLAIN_MASK_ALL         ( 0xffff )

   #define OPT_EXPLAIN_MASK_NONE_NAME        "None"
   #define OPT_EXPLAIN_MASK_INPUT_NAME       OPT_FIELD_INPUT
   #define OPT_EXPLAIN_MASK_FILTER_NAME      OPT_FIELD_FILTER
   #define OPT_EXPLAIN_MASK_OUTPUT_NAME      OPT_FIELD_OUTPUT
   #define OPT_EXPLAIN_MASK_ALL_NAME         "All"

   /*
      Explain field names
    */

   // Explain for cache status
   #define OPT_FIELD_CACHE_STATUS         "CacheStatus"
   // Not cached
   #define OPT_VALUE_CACHE_STATUS_NOCACHE    "NoCache"
   // New created into cache
   #define OPT_VALUE_CACHE_STATUS_NEWCACHE   "NewCache"
   // Hit cache
   #define OPT_VALUE_CACHE_STATUS_HITCACHE   "HitCache"

   // Explain for cache level ( --plancachelevel )
   #define OPT_FIELD_CACHE_LEVEL          "CacheLevel"
   #define OPT_VALUE_CACHE_NOCACHE         "OPT_PLAN_NOCACHE"
   #define OPT_VALUE_CACHE_ORIGINAL        "OPT_PLAN_ORIGINAL"
   #define OPT_VALUE_CACHE_NORMALIZED      "OPT_PLAN_NORMALZIED"
   #define OPT_VALUE_CACHE_PARAMETERIZED   "OPT_PLAN_PARAMETERIZED"
   #define OPT_VALUE_CACHE_FUZZYOPTR       "OPT_PLAN_FUZZYOPTR"

   #define OPT_FIELD_HASH_CODE            "HashCode"
   #define OPT_FIELD_SORT                 FIELD_NAME_SORT
   #define OPT_FIELD_HINT                 FIELD_NAME_HINT
   #define OPT_FIELD_SORTED_IDX_REQURED   "SortedIndexRequired"
   #define OPT_FIELD_PARAM_PLAN_VALID     "ParamPlanValid"
   #define OPT_FIELD_MAINCL_PLAN_VALID    "MainCLPlanValid"
   #define OPT_FIELD_VALID_PARAMS         "ValidParams"
   #define OPT_FIELD_VALID_SUBCLS         "ValidSubCLs"
   #define OPT_FIELD_PLAN_PATH            "PlanPath"
   #define OPT_FIELD_SEARCH               FIELD_NAME_SEARCH
   #define OPT_FIELD_SEARCH_PATHS         "SearchPaths"
   #define OPT_FIELD_SCAN_NODE            "ScanNode"
   #define OPT_FIELD_SORT_NODE            "SortNode"
   #define OPT_FIELD_PLAN_SCORE           "Score"
   #define OPT_FIELD_TOTAL_QUERY_TIME     "TotalQueryTime"
   #define OPT_FIELD_AVG_QUERY_TIME       "AvgQueryTime"
   #define OPT_FIELD_PLAN_REF_COUNT       "RefCount"
   #define OPT_FIELD_PLAN_ACCESS_COUNT    "AccessCount"
   #define OPT_FIELD_MAINCL_PLAN          "MainCLPlan"
   #define OPT_FIELD_DATA_READ            FIELD_NAME_DATAREAD
   #define OPT_FIELD_INDEX_READ           FIELD_NAME_INDEXREAD

   // Explain for query activity
   #define OPT_FIELD_MAX_QUERY            "MaxTimeSpentQuery"
   #define OPT_FIELD_MIN_QUERY            "MinTimeSpentQuery"
   #define OPT_FIELD_CONTEXT_ID           FIELD_NAME_CONTEXTID
   #define OPT_FIELD_QUERY_TYPE           "QueryType"
   #define OPT_VALUE_QUERY_TYPE_SELECT    "SELECT"
   #define OPT_VALUE_QUERY_TYPE_UPDATE    "UPDATE"
   #define OPT_VALUE_QUERY_TYPE_DELETE    "DELETE"
   #define OPT_FIELD_QUERY_TIME_SPENT     FIELD_NAME_QUERYTIMESPENT
   #define OPT_FIELD_EXECUTE_TIME_SPENT   "ExecuteTimeSpent"
   #define OPT_FIELD_WAIT_TIME_SPENT      "WaitTimeSpent"
   #define OPT_FIELD_QUERY_START_TIME     FIELD_NAME_STARTTIMESTAMP
   #define OPT_FIELD_HIT_END              "HitEnd"

   // Explain details
   #define OPT_FIELD_ROLE                 FIELD_NAME_ROLE
   #define OPT_FIELD_NODE_NAME            FIELD_NAME_NODE_NAME
   #define OPT_FIELD_GROUP_NAME           FIELD_NAME_GROUPNAME
   #define OPT_FIELD_NAME                 FIELD_NAME_NAME
   #define OPT_FIELD_SCAN_TYPE            FIELD_NAME_SCANTYPE
   #define OPT_VALUE_TBSCAN               VALUE_NAME_TBSCAN
   #define OPT_VALUE_IXSCAN               VALUE_NAME_IXSCAN
   #define OPT_FIELD_ACCESSPLAN_ID        FIELD_NAME_ACCESSPLAN_ID
   #define OPT_FIELD_USE_EXT_SORT         FIELD_NAME_USE_EXT_SORT
   #define OPT_FIELD_OPERATOR             "Operator"
   #define OPT_FIELD_ESTIMATE             FIELD_NAME_ESTIMATE
   #define OPT_FIELD_RUN                  FIELD_NAME_RUN
   #define OPT_FIELD_INPUT                "Input"
   #define OPT_FIELD_OUTPUT               "Output"
   #define OPT_FIELD_FILTER               "Filter"
   #define OPT_FIELD_CHILD_NODES          "ChildOperators"
   #define OPT_FIELD_SUB_COLLECTIONS      FIELD_NAME_SUB_COLLECTIONS
   #define OPT_FIELD_SELECTOR             FIELD_NAME_SELECTOR
   #define OPT_FIELD_SKIP                 FIELD_NAME_SKIP
   #define OPT_FIELD_RETURN               FIELD_NAME_RETURN
   #define OPT_FIELD_COLLECTION           FIELD_NAME_COLLECTION
   #define OPT_FIELD_COLLECTION_SPACE     FIELD_NAME_COLLECTIONSPACE
   #define OPT_FIELD_INDEX                FIELD_NAME_INDEX
   #define OPT_FIELD_INDEX_NAME           FIELD_NAME_INDEXNAME
   #define OPT_FIELD_QUERY                FIELD_NAME_QUERY
   #define OPT_FIELD_FLAG                 "Flag"
   #define OPT_FIELD_IX_BOUND             FIELD_NAME_IX_BOUND
   #define OPT_FIELD_DIRECTION            FIELD_NAME_DIRECTION
   #define OPT_FIELD_NEED_MATCH           FIELD_NAME_NEED_MATCH
   #define OPT_FIELD_PAGES                "Pages"
   #define OPT_FIELD_PAGE_SIZE            FIELD_NAME_PAGE_SIZE
   #define OPT_FIELD_RECORDS              "Records"
   #define OPT_FIELD_RECORD_SIZE          "RecordSize"
   #define OPT_FIELD_RECORD_TOTAL_SIZE    "RecordTotalSize"
   #define OPT_FIELD_SORTED               "Sorted"
   #define OPT_FIELD_READ_RECORDS         "ReadRecords"
   #define OPT_FIELD_READ_PAGES           "ReadPages"
   #define OPT_FIELD_OUTPUT_RECORDS       "OutputRecords"
   #define OPT_FIELD_MTH_SEL              "MthSelectivity"
   #define OPT_FIELD_MTH_COST             "MthCPUCost"
   #define OPT_FIELD_SCAN_SEL             "IXScanSelectivity"
   #define OPT_FIELD_PRED_SEL             "IXPredSelectivity"
   #define OPT_FIELD_PRED_COST            "PredCPUCost"
   #define OPT_FIELD_SORT_TYPE            "SortType"
   #define OPT_VALUE_SORT_IN_MEM          "InMemory"
   #define OPT_VALUE_SORT_EXTERNAL        "External"
   #define OPT_FIELD_START_COST           "StartCost"
   #define OPT_FIELD_RUN_COST             "RunCost"
   #define OPT_FIELD_TOTAL_COST           "TotalCost"
   #define OPT_FIELD_CL_STAT_EST          "CLEstFromStat"
   #define OPT_FIELD_CL_STAT_TIME         "CLStatTime"
   #define OPT_FIELD_IX_STAT_EST          "IXEstFromStat"
   #define OPT_FIELD_IX_STAT_TIME         "IXStatTime"
   #define OPT_FIELD_EST_FROM_STAT        "EstFromStat"
   #define OPT_FIELD_IS_CANDIDATE         "IsCandidate"
   #define OPT_FIELD_IS_USED              "IsUsed"
   #define OPT_FIELD_INDEX_PAGES          "IndexPages"
   #define OPT_FIELD_INDEX_LEVELS         "IndexLevels"
   #define OPT_FIELD_INDEX_READ_RECORDS   "IndexReadRecords"
   #define OPT_FIELD_INDEX_READ_PAGES     "IndexReadPages"
   #define OPT_FIELD_GETMORES             "GetMores"
   #define OPT_FIELD_RETURN_NUM           FIELD_NAME_RETURN_NUM
   #define OPT_FIELD_ELAPSED_TIME         FIELD_NAME_ELAPSED_TIME
   #define OPT_FIELD_USERCPU              FIELD_NAME_USERCPU
   #define OPT_FIELD_SYSCPU               FIELD_NAME_SYSCPU
   #define OPT_FIELD_IO_COST              "IOCost"
   #define OPT_FIELD_CPU_COST             "CPUCost"
   #define OPT_FIELD_RAN_IO_COST          "RandomReadIOCostUnit"
   #define OPT_FIELD_SEQ_IO_COST          "SeqReadIOCostUnit"
   #define OPT_FIELD_SEQ_WRITE_IO_COST    "SeqWrtIOCostUnit"
   #define OPT_FIELD_PAGE_UINT            "PageUnit"
   #define OPT_FIELD_IDX_CPU_COST         "IXExtractCPUCost"
   #define OPT_FIELD_REC_CPU_COST         "RecExtractCPUCost"
   #define OPT_FIELD_IX_START_COST        "IXScanStartCost"
   #define OPT_FIELD_TB_START_COST        "TBScanStartCost"
   #define OPT_FIELD_IO_CPU_RATE          "IOCPURate"
   #define OPT_FIELD_SORT_FIELDS          "SortFields"
   #define OPT_FIELD_OPTR_CPU_COST        "OptrCPUCost"
   #define OPT_FIELD_CHILD_TOTAL_COST     "ChildTotalCost"
   #define OPT_FIELD_CONSTANTS            "Constants"
   #define OPT_FIELD_OPTIONS              FIELD_NAME_OPTIONS
   #define OPT_FIELD_NEED_EVAL_IO         "NeedEvalIO"
   #define OPT_FIELD_OPT_COST_THRESHOLD   PMD_OPTION_OPT_COST_THRESHOLD
   #define OPT_FIELD_SORT_BUFF_SIZE       PMD_OPTION_SORTBUF_SIZE
   #define OPT_FIELD_NEED_REORDER         "NeedReorder"
   #define OPT_FIELD_SUB_COLLECTION_LIST  "SubCollectionList"
   #define OPT_FILED_SUB_COLLECTION_NUM   "SubCollectionNum"
   #define OPT_FIELD_DATA_NODE_LIST       "DataNodeList"
   #define OPT_FIELD_DATA_NODE_NUM        "DataNodeNum"
   #define OPT_FIELD_SUMMARY_NAME         FIELD_NAME_NAME
   #define OPT_FIELD_SUMMARY_EST_COST     "Est" OPT_FIELD_TOTAL_COST

   // Mask for explain info
   #define OPT_EXPINFO_MASK_RETURN_NUM    ( 0x0001 )
   #define OPT_EXPINFO_MASK_ELAPSED_TIME  ( 0x0002 )
   #define OPT_EXPINFO_MASK_INDEX_READ    ( 0x0004 )
   #define OPT_EXPINFO_MASK_DATA_READ     ( 0x0008 )
   #define OPT_EXPINFO_MASK_USERCPU       ( 0x0010 )
   #define OPT_EXPINFO_MASK_SYSCPU        ( 0x0020 )
   #define OPT_EXPINFO_MASK_NONE          ( 0x0000 )
   #define OPT_EXPINFO_MASK_ALL           ( 0xFFFF )

}

#endif //OPTCOMMON_HPP__
