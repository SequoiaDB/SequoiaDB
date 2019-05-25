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

   Source File Name = qgmUtil.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef QGMUTIL_HPP_
#define QGMUTIL_HPP_

#include "core.hpp"
#include "qgmOptiTree.hpp"
#include "qgmPlan.hpp"
#include "qgmOptiAggregation.hpp"

#define TABLE_SCAN                  "NULL"
#define TABLE_SCAN_SIZE             ( sizeof( TABLE_SCAN ) - 1 )
#define TABLE_SCAN_LOWER            "null"
#define TABLE_SCAN_LOWER_SIZE       ( sizeof( TABLE_SCAN_LOWER ) - 1 )
#define FLG_SQL_UPDATE_KEEP_SK      "SQL_UPDATE_KEEP_SHARDINGKEY"
#define FLG_SQL_UPDATE_KEEP_SK_SIZE ( sizeof( FLG_SQL_UPDATE_KEEP_SK ) - 1 )

using namespace bson ;

namespace engine
{

   struct _qgmConditionNode ;

   BOOLEAN qgmUtilFirstDot( const CHAR *str, UINT32 len, UINT32 &pos ) ;
   BOOLEAN qgmUtilLastDot( const CHAR *str, UINT32 len, UINT32 &pos ) ;

   BOOLEAN qgmUtilSame( const CHAR *src, UINT32 srcLen,
                        const CHAR *dst, UINT32 dstLen ) ;

   INT32 qgmFindFieldFromFunc( const CHAR *str, UINT32 size,
                               _qgmField &func,
                               vector<qgmOpField> &params,
                               _qgmPtrTable *table,
                               BOOLEAN needRele ) ;

   BOOLEAN isFromOne( const qgmOpField &left, const qgmOPFieldVec &right,
                      BOOLEAN useAlias = TRUE, UINT32 *pPos = NULL ) ;
   BOOLEAN isSameFrom( const qgmOPFieldVec &left, const qgmOPFieldVec &right ) ;

   BOOLEAN isFrom( const qgmDbAttr &left, const qgmOpField &right,
                   BOOLEAN useAlias = TRUE ) ;
   BOOLEAN isFromOne( const qgmDbAttr &left, const qgmOPFieldVec &right,
                      BOOLEAN useAlias = TRUE, UINT32 *pPos = NULL ) ;

   BSONObj qgmMerge( const BSONObj &left, const BSONObj &right ) ;

   string qgmPlanType( QGM_PLAN_TYPE type ) ;


   BOOLEAN isWildCard( const qgmOPFieldVec &fields ) ;
   void  replaceFieldRele( qgmOPFieldVec &fields, const qgmField &newRele ) ;
   void  replaceAttrRele( qgmDbAttrPtrVec &attrs,  const qgmField &newRele ) ;
   void  replaceAttrRele( qgmDbAttrVec &attrs, const qgmField &newRele ) ;
   void  replaceAggrRele( qgmAggrSelectorVec &aggrs, const qgmField &newRele ) ;

   void  clearFieldAlias( qgmOPFieldVec &fields ) ;

   INT32 downFieldsByFieldAlias( qgmOPFieldVec &fields,
                                 const qgmOPFieldPtrVec & fieldAlias,
                                 BOOLEAN needCopyAlias,
                                 BOOLEAN isOptional ) ;
   INT32 downAttrsByFieldAlias( qgmDbAttrPtrVec &attrs,
                                const qgmOPFieldPtrVec & fieldAlias,
                                BOOLEAN isOptional ) ;
   INT32 downAttrsByFieldAlias( qgmDbAttrVec &attrs,
                                const qgmOPFieldPtrVec & fieldAlias,
                                BOOLEAN isOptional ) ;
   INT32 downAAttrByFieldAlias( qgmDbAttr &attr,
                                const qgmOPFieldPtrVec & fieldAlias,
                                BOOLEAN isOptional ) ;
   INT32 downAggrsByFieldAlias( qgmAggrSelectorVec &aggrs,
                                const qgmOPFieldPtrVec &fieldAlias,
                                BOOLEAN isOptional ) ;

   INT32 upFieldsByFieldAlias( qgmOPFieldVec &fields,
                               const qgmOPFieldPtrVec & fieldAlias,
                               BOOLEAN needClearAlias ) ;
   INT32 upAttrsByFieldAlias( qgmDbAttrPtrVec &attrs,
                              const qgmOPFieldPtrVec & fieldAlias ) ;
   INT32 upAttrsByFieldAlias( qgmDbAttrVec &attrs,
                              const qgmOPFieldPtrVec & fieldAlias ) ;
   INT32 upAAttrByFieldAlias( qgmDbAttr &attr,
                              const qgmOPFieldPtrVec & fieldAlias ) ;
   INT32 upAggrsByFieldAlias( qgmAggrSelectorVec &aggrs,
                              const qgmOPFieldPtrVec & fieldAlias ) ;

   string qgmHintToString( const QGM_HINS &hint ) ;

   BSONObj qgmUseIndexHintToBson( const qgmHint &h ) ;

   INT32 qgmUseHintToFlag( const qgmHint &h, INT32 &flag ) ;

   const CHAR* qgmGetNodeTypeStr( INT32 type ) ;

   INT32    qgmBuildANodeItem( BSONObjBuilder &bb,
                               const CHAR *pKeyName,
                               const _qgmConditionNode *node ) ;

   INT32    qgmParseValue( INT32 type,
                           const string &value,
                           BSONObjBuilder &builder,
                           const string &fieldName ) ;

   INT32    qgmParseValue( const qgmOpField &value,
                           BSONObjBuilder &builder,
                           const string &fieldName ) ;

   INT32    qgmParseValue( const SQL_CON_ITR &root,
                           BSONObjBuilder &builder,
                           const string &fieldName ) ;

   BOOLEAN  sqlIsCommonValue( INT32 type ) ;
   BOOLEAN  sqlIsNestedValue( INT32 type ) ;

}

#endif // QGMUTIL_HPP_

