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

   Source File Name = qgmUtil.cpp

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

#include "qgmUtil.hpp"
#include "qgmConditionNode.hpp"
#include "pd.hpp"
#include "ossUtil.hpp"
#include "pdTrace.hpp"
#include "qgmTrace.hpp"
#include "msg.h"
#include "utilStr.hpp"

using namespace bson ;

namespace engine
{
   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMUTILFIRSTDOT, "qgmUtilFirstDot" )
   BOOLEAN qgmUtilFirstDot( const CHAR *str, UINT32 len, UINT32 &pos )
   {
      PD_TRACE_ENTRY( SDB__QGMUTILFIRSTDOT ) ;
      SDB_ASSERT( NULL != str, "impossible" ) ;
      BOOLEAN found = FALSE ;
      UINT32 tLen = 0 ;

      while ( tLen < len )
      {
         if ( '.' == str[ tLen ] )
         {
            pos = tLen ;
            found = TRUE ;
            break ;
         }
         ++tLen ;
      }
      PD_TRACE_EXIT( SDB__QGMUTILFIRSTDOT ) ;
      return found ;
   }

   BOOLEAN qgmUtilLastDot( const CHAR *str, UINT32 len, UINT32 &pos )
   {
      BOOLEAN found = FALSE ;
      UINT32 tLen = len ;

      while ( tLen > 0 )
      {
         --tLen ;
         if ( '.' == str[ tLen ] )
         {
            pos = tLen ;
            found = TRUE ;
            break ;
         }
      }
      return found ;
   }

   BOOLEAN qgmUtilSame( const CHAR *src, UINT32 srcLen,
                        const CHAR *dst, UINT32 dstLen )
   {
      return srcLen == dstLen ?
             SDB_OK == ossStrncmp( src, dst, srcLen ) : FALSE ;
   }

   enum QGM_FIND_FUNC_STATUS
   {
      FUNC = 0,
      FIELD,
   } ;

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMFINDFIELDFROMFUNC, "qgmFindFieldFromFunc" )
   INT32 qgmFindFieldFromFunc( const CHAR *str, UINT32 strSize,
                               _qgmField &func,
                               qgmOPFieldVec &params,
                               _qgmPtrTable *table,
                               BOOLEAN needRele )
   {
      PD_TRACE_ENTRY( SDB__QGMFINDFIELDFROMFUNC ) ;
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != str, "impossible" ) ;

      const CHAR *tmp = NULL ;
      UINT32 read = 0 ;
      UINT32 size = 0 ;
      UINT32 fieldNameSize = 0 ;
      const CHAR *begin = str ;
      const CHAR *fieldName = str ;
      QGM_FIND_FUNC_STATUS status = FUNC;
      while ( read < strSize )
      {
         tmp = str + read ;

         /// ignore all space
         if ( ' ' == *tmp || '\t' == *tmp )
         {
            ++read ;
            continue ;
         }
         else if ( FUNC == status )
         {
            if ( '(' == *tmp )
            {
               status = FIELD ;
               rc = table->getField( begin, size, func ) ;
               if ( SDB_OK != rc )
               {
                  goto error ;
               }
               size = 0 ;
               begin = NULL ;
            }
            else
            {
               ++size ;
            }
         }
         else
         {
            if ( ',' == *tmp )
            {
               qgmOpField field;
               if ( NULL == begin || NULL == fieldName )
               {
                  PD_LOG( PDERROR, "Aggr func param is null" ) ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
               rc = table->getAttr( begin, size, field.value ) ;
               if ( SDB_OK != rc )
               {
                  goto error ;
               }
               if ( needRele && field.value.relegation().empty() )
               {
                  goto error ;
               }

               field.alias = field.value.attr() ;

               params.push_back( field ) ;
               begin = NULL ;
               size = 0 ;
               fieldName = NULL ;
               fieldNameSize = 0 ;
            }
            else if ( ')' == *tmp )
            {
               if ( NULL != begin && NULL != fieldName )
               {
                  qgmOpField field ;
                  rc = table->getAttr( begin, size, field.value ) ;
                  if ( SDB_OK != rc )
                  {
                     goto error ;
                  }
                  if ( needRele && field.value.relegation().empty() )
                  {
                     goto error ;
                  }

                  field.alias = field.value.attr() ;
                  params.push_back( field ) ;
               }
               begin = NULL ;
               size = 0 ;
               fieldName = NULL ;
               fieldNameSize = 0 ;
            }
            else
            {
               if ( NULL == begin )
               {
                  begin = tmp ;
               }
               ++size ;
               if ( NULL == fieldName )
               {
                  fieldName = tmp;
               }
               ++fieldNameSize ;
               if ( '.' == *tmp )
               {
                  fieldName = NULL;
                  fieldNameSize = 0;
               }
            }
         }

         ++read ;
      }

   done:
      PD_TRACE_EXITRC( SDB__QGMFINDFIELDFROMFUNC, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMISFROMONE, "isFromOne")
   BOOLEAN isFromOne( const qgmOpField & left, const qgmOPFieldVec & right,
                      BOOLEAN useAlias, UINT32 *pPos )
   {
      PD_TRACE_ENTRY( SDB__QGMISFROMONE ) ;
      BOOLEAN ret = FALSE ;
      UINT32 i = 0 ;
      while ( i < right.size() )
      {
         if ( left.isFrom( right[i], useAlias ) )
         {
            if ( pPos )
            {
               *pPos = i ;
            }
            ret = TRUE ;
            goto error ;
         }
         ++i ;
      }

   done:
      PD_TRACE_EXIT( SDB__QGMISFROMONE ) ;
      return ret ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMISSAMEFROM, "isSameFrom" )
   BOOLEAN isSameFrom( const qgmOPFieldVec & left, const qgmOPFieldVec & right )
   {
      PD_TRACE_ENTRY( SDB__QGMISSAMEFROM ) ;
      BOOLEAN ret = TRUE ;
      UINT32 i = 0 ;

      if ( left.size() != right.size() )
      {
         ret = FALSE ;
         goto error ;
      }

      while ( i < left.size() )
      {
         qgmOpField field ;

         if ( ( !right[i].alias.empty() &&
                 left[i].value.attr() != right[i].alias ) ||
              ( right[i].alias.empty() &&
                left[i].value.attr() != right[i].value.attr() ) )
         {
            ret = FALSE ;
            goto error ;
         }
         ++i ;
      }

   done:
      PD_TRACE_EXIT( SDB__QGMISSAMEFROM ) ;
      return ret ;
   error:
      goto done ;
   }

   BOOLEAN isFrom( const qgmDbAttr &left, const qgmOpField &right,
                   BOOLEAN useAlias )
   {
      if ( ( useAlias && left.attr() == right.alias )
           || left == right.value || right.type == SQL_GRAMMAR::WILDCARD )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   BOOLEAN isFromOne( const qgmDbAttr &left, const qgmOPFieldVec &right,
                      BOOLEAN useAlias, UINT32 *pPos )
   {
      UINT32 i = 0 ;
      while ( i < right.size() )
      {
         if ( isFrom( left, right[i], useAlias ) )
         {
            if ( pPos )
            {
               *pPos = i ;
            }
            return TRUE ;
         }
         ++i ;
      }
      return FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMMERGE, "qgmMerge" )
   BSONObj qgmMerge( const BSONObj &left, const BSONObj &right )
   {
      PD_TRACE_ENTRY( SDB__QGMMERGE ) ;
      BSONObj obj ;
      BSONObjBuilder builder ;
      try
      {
         BSONObjIterator itr1( left ) ;
         while ( itr1.more() )
         {
            builder.append( itr1.next() ) ;
         }

         BSONObjIterator itr2( right ) ;
         while ( itr2.more() )
         {
            builder.append( itr2.next() ) ;
         }

         obj = builder.obj() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s",
                 e.what() ) ;
      }

      PD_TRACE_EXIT( SDB__QGMMERGE ) ;
      return obj ;
   }

   const CHAR* qgmPlanType( QGM_PLAN_TYPE type )
   {
      const CHAR *t = "" ;
      if ( QGM_PLAN_TYPE_RETURN == type )
      {
         t = "RETURN" ;
      }
      else if ( QGM_PLAN_TYPE_FILTER == type )
      {
         t = "FILTER" ;
      }
      else if ( QGM_PLAN_TYPE_SCAN == type )
      {
         t = "SCAN" ;
      }
      else if ( QGM_PLAN_TYPE_NLJOIN == type )
      {
         t = "NLJOIN" ;
      }
      else if ( QGM_PLAN_TYPE_INSERT == type )
      {
         t = "INSERT" ;
      }
      else if ( QGM_PLAN_TYPE_UPDATE == type )
      {
         t = "UPDATE" ;
      }
      else if ( QGM_PLAN_TYPE_AGGR == type )
      {
         t = "AGGREGATION" ;
      }
      else if ( QGM_PLAN_TYPE_SORT == type )
      {
         t = "SORT" ;
      }
      else if ( QGM_PLAN_TYPE_COMMAND == type )
      {
         t = "COMMAND" ;
      }
      else if ( QGM_PLAN_TYPE_DELETE == type )
      {
         t = "DELETE" ;
      }
      else
      {
         /// do noting.
      }

      return t ;
   }

   BOOLEAN isWildCard( const qgmOPFieldVec & fields )
   {
      if ( 1 == fields.size() && SQL_GRAMMAR::WILDCARD == fields[0].type )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMREPLACEFIELDRELE, "replaceFieldRele" )
   void replaceFieldRele( qgmOPFieldVec &fields, const qgmField &newRele )
   {
      PD_TRACE_ENTRY( SDB__QGMREPLACEFIELDRELE ) ;
      qgmOPFieldVec::iterator it = fields.begin() ;
      while ( it != fields.end() )
      {
         qgmOpField &field = *it ;
         if ( SQL_GRAMMAR::WILDCARD != field.type )
         {
            field.value.relegation() = newRele ;
         }
         ++it ;
      }

      PD_TRACE_EXIT( SDB__QGMREPLACEFIELDRELE ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMREPLACEATTRRELE, "replaceAttrRele" )
   void replaceAttrRele( qgmDbAttrPtrVec &attrs,  const qgmField &newRele )
   {
      PD_TRACE_ENTRY( SDB__QGMREPLACEATTRRELE ) ;
      qgmDbAttrPtrVec::iterator it = attrs.begin() ;
      while ( it != attrs.end() )
      {
         qgmDbAttr &attr = *(*it) ;
         attr.relegation() = newRele ;
         ++it ;
      }
      PD_TRACE_EXIT( SDB__QGMREPLACEATTRRELE ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMREPLACEATTRRELE2, "replaceAttrRele" )
   void replaceAttrRele( qgmDbAttrVec &attrs,  const qgmField &newRele )
   {
      PD_TRACE_ENTRY( SDB__QGMREPLACEATTRRELE2 ) ;
      qgmDbAttrVec::iterator it = attrs.begin() ;
      while ( it != attrs.end() )
      {
         qgmDbAttr &attr = *it ;
         attr.relegation() = newRele ;
         ++it ;
      }

      PD_TRACE_EXIT( SDB__QGMREPLACEATTRRELE2 ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMREPLACEATTRRELE3, "replaceAggrRele" )
   void replaceAggrRele( qgmAggrSelectorVec & aggrs, const qgmField & newRele )
   {
      PD_TRACE_ENTRY( SDB__QGMREPLACEATTRRELE3 ) ;
      qgmAggrSelectorVec::iterator it = aggrs.begin() ;
      while ( it != aggrs.end() )
      {
         qgmAggrSelector &selector = *it ;
         if ( SQL_GRAMMAR::FUNC == selector.value.type )
         {
            replaceFieldRele( selector.param, newRele ) ;
         }
         else if ( SQL_GRAMMAR::WILDCARD != selector.value.type )
         {
            selector.value.value.relegation() = newRele ;
         }
         ++it ;
      }

      PD_TRACE_EXIT( SDB__QGMREPLACEATTRRELE3 ) ;
      return ;
   }

   void clearFieldAlias( qgmOPFieldVec & fields )
   {
      UINT32 count = fields.size() ;
      UINT32 index = 0 ;
      while ( index < count )
      {
         fields[index].alias.clear() ;
         ++index ;
      }
   }

   static INT32 downAFieldByFieldAlias( qgmOpField &field,
                                        const qgmOPFieldPtrVec & fieldAlias,
                                        BOOLEAN needCopyAlias,
                                        BOOLEAN isOptional )
   {
      UINT32 subpos = 0 ;
      BOOLEAN find = FALSE ;
      qgmOPFieldPtrVec::const_iterator cit = fieldAlias.begin() ;
      while ( cit != fieldAlias.end() )
      {
         if ( ( (*cit)->alias.empty() &&
                field.value.attr().isSubfix( (*cit)->value.attr(),
                                             TRUE, &subpos ) ) ||
              ( !(*cit)->alias.empty() &&
                field.value.attr().isSubfix( (*cit)->alias,
                                             TRUE, &subpos ) ) )
         {
            if ( isOptional || subpos == _qgmField::npos )
            {
               field.value = (*cit)->value ;
            }
            else
            {
               field.value.relegation() = (*cit)->value.relegation() ;
               field.value.attr().replace( 0, subpos, (*cit)->value.attr() ) ;
            }

            if ( needCopyAlias && field.alias.empty() )
            {
               field.alias = (*cit)->alias ;
            }
            find = TRUE ;
            break ;
         }
         ++cit ;
      }

      if ( !find && isOptional )
      {
         field.value.attr() = field.value.attr().rootField() ;
      }

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QDMDOWNFIELDSBYFIELDALIAS, "downFieldsByFieldAlias" )
   INT32 downFieldsByFieldAlias( qgmOPFieldVec & fields,
                                 const qgmOPFieldPtrVec & fieldAlias,
                                 BOOLEAN needCopyAlias,
                                 BOOLEAN isOptional )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__QDMDOWNFIELDSBYFIELDALIAS ) ;
      qgmOPFieldVec::iterator it = fields.begin() ;
      while ( it != fields.end() )
      {
         if ( SQL_GRAMMAR::WILDCARD != (*it).type )
         {
            downAFieldByFieldAlias( *it, fieldAlias, needCopyAlias,
                                    isOptional ) ;
         }
         ++it ;
      }

      PD_TRACE_EXITRC( SDB__QDMDOWNFIELDSBYFIELDALIAS, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMDOWNATTRSBYFIELDALIAS, "downAttrsByFieldAlias" )
   INT32 downAttrsByFieldAlias( qgmDbAttrPtrVec & attrs,
                                const qgmOPFieldPtrVec & fieldAlias,
                                BOOLEAN isOptional )
   {
      PD_TRACE_ENTRY( SDB__QGMDOWNATTRSBYFIELDALIAS ) ;
      INT32 rc = SDB_OK ;
      UINT32 subpos = 0 ;
      BOOLEAN find = FALSE ;
      qgmDbAttrPtrVec::iterator it = attrs.begin() ;
      while ( it != attrs.end() )
      {
         qgmDbAttr &attr = *(*it) ;
         find = FALSE ;

         qgmOPFieldPtrVec::const_iterator cit = fieldAlias.begin() ;
         while ( cit != fieldAlias.end() )
         {
            if ( ( (*cit)->alias.empty() &&
                   attr.attr().isSubfix( (*cit)->value.attr(),
                                         TRUE, &subpos ) ) ||
                 ( !(*cit)->alias.empty() &&
                   attr.attr().isSubfix( (*cit)->alias,
                                         TRUE, &subpos ) ) )
            {
               if ( 0 == subpos || isOptional )
               {
                  attr = (*cit)->value ;
               }
               else
               {
                  attr.relegation() = (*cit)->value.relegation() ;
                  attr.attr().replace( 0, subpos, (*cit)->value.attr() ) ;
               }
               find = TRUE ;
               break ;
            }
            ++cit ;
         }

         if ( !find && isOptional )
         {
            attr.attr() = attr.attr().rootField() ;
         }
         ++it ;
      }

      PD_TRACE_EXITRC( SDB__QGMDOWNATTRSBYFIELDALIAS, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMDOWNATTRSBYFIELDALIAS2, "downAttrsByFieldAlias" )
   INT32 downAttrsByFieldAlias( qgmDbAttrVec & attrs,
                                const qgmOPFieldPtrVec & fieldAlias,
                                BOOLEAN isOptional )
   {
      PD_TRACE_ENTRY( SDB__QGMDOWNATTRSBYFIELDALIAS2 ) ;
      INT32 rc = SDB_OK ;
      UINT32 subpos = 0 ;
      BOOLEAN find = FALSE ;

      qgmDbAttrVec::iterator it = attrs.begin() ;
      while ( it != attrs.end() )
      {
         qgmDbAttr &attr = *it ;
         find = FALSE ;

         qgmOPFieldPtrVec::const_iterator cit = fieldAlias.begin() ;
         while ( cit != fieldAlias.end() )
         {
            if ( ( (*cit)->alias.empty() &&
                   attr.attr().isSubfix( (*cit)->value.attr(),
                                         TRUE, &subpos ) ) ||
                 ( !(*cit)->alias.empty() &&
                   attr.attr().isSubfix( (*cit)->alias,
                                         TRUE, &subpos ) ) )
            {
               if ( 0 == subpos || isOptional )
               {
                  attr = (*cit)->value ;
               }
               else
               {
                  attr.relegation() = (*cit)->value.relegation() ;
                  attr.attr().replace( 0, subpos, (*cit)->value.attr() ) ;
               }
               find = TRUE ;
               break ;
            }
            ++cit ;
         }
         if ( !find && isOptional )
         {
            attr.attr() = attr.attr().rootField() ;
         }
         ++it ;
      }

      PD_TRACE_EXITRC( SDB__QGMDOWNATTRSBYFIELDALIAS2, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMDOWNAATTRBYFIELDALIAS, "downAAttrByFieldAlias" )
   INT32 downAAttrByFieldAlias( qgmDbAttr & attr,
                                const qgmOPFieldPtrVec & fieldAlias,
                                BOOLEAN isOptional )
   {
      PD_TRACE_ENTRY( SDB__QGMDOWNAATTRBYFIELDALIAS ) ;
      INT32 rc = SDB_OK ;
      UINT32 subpos = 0 ;
      BOOLEAN find = FALSE ;

      qgmOPFieldPtrVec::const_iterator cit = fieldAlias.begin() ;
      while ( cit != fieldAlias.end() )
      {
         if ( ( (*cit)->alias.empty() &&
                attr.attr().isSubfix( (*cit)->value.attr(),
                                      TRUE, &subpos ) ) ||
              ( !(*cit)->alias.empty() &&
                attr.attr().isSubfix( (*cit)->alias,
                                      TRUE, &subpos ) ) )
         {
            if ( 0 == subpos || isOptional )
            {
               attr = (*cit)->value ;
            }
            else
            {
               attr.relegation() = (*cit)->value.relegation() ;
               attr.attr().replace( 0, subpos, (*cit)->value.attr() ) ;
            }
            find = TRUE ;
            break ;
         }
         ++cit ;
      }
      if ( !find && isOptional )
      {
         attr.attr() = attr.attr().rootField() ;
      }

      PD_TRACE_EXITRC( SDB__QGMDOWNAATTRBYFIELDALIAS, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMDOWNAGGRSBYFIELDALIAS, "downAggrsByFieldAlias" )
   INT32 downAggrsByFieldAlias( qgmAggrSelectorVec &aggrs,
                                const qgmOPFieldPtrVec &fieldAlias,
                                BOOLEAN isOptional )
   {
      PD_TRACE_ENTRY( SDB__QGMDOWNAGGRSBYFIELDALIAS ) ;
      INT32 rc = SDB_OK ;
      qgmAggrSelectorVec::iterator it = aggrs.begin() ;
      while ( it != aggrs.end() )
      {
         qgmAggrSelector &selector= *it ;

         if ( SQL_GRAMMAR::FUNC == selector.value.type )
         {
            qgmOPFieldVec::iterator iter = selector.param.begin();
            while ( iter != selector.param.end() )
            {
               downAAttrByFieldAlias( iter->value, fieldAlias, isOptional );
               ++iter;
            }
         }
         else if ( SQL_GRAMMAR::WILDCARD != selector.value.type )
         {
            downAFieldByFieldAlias( selector.value, fieldAlias, TRUE,
                                    isOptional ) ;
         }
         ++it ;
      }

      PD_TRACE_EXITRC( SDB__QGMDOWNAGGRSBYFIELDALIAS, rc ) ;
      return rc ;
   }

   static INT32 upAFieldByFieldAlias( qgmOpField &field,
                                      const qgmOPFieldPtrVec & fieldAlias,
                                      BOOLEAN needClearAlias )
   {
      if ( needClearAlias && field.alias.empty() )
      {
         return SDB_OK ;
      }

      UINT32 pos = 0 ;
      qgmOPFieldPtrVec::const_iterator cit = fieldAlias.begin() ;
      while ( cit != fieldAlias.end() )
      {
         if ( ( field.alias.empty() && field.value == (*cit)->value ) ||
              ( !field.alias.empty() && field == *(*cit) ) )
         {
            if ( (*cit)->alias.empty() )
            {
               field.value.relegation().clear() ;
            }
            else if ( field.value.attr().isSubfix( (*cit)->alias,
                                                   FALSE, &pos ) )
            {
               field.value.attr().replace( 0, pos, (*cit)->alias ) ;
            }
            else
            {
               field.value.attr() = (*cit)->alias ;
            }

            if ( needClearAlias && field.value.attr() == field.alias )
            {
               field.alias.clear() ;
            }
            break ;
         }

         ++cit ;
      }

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMUPFIELDSBYFIELDALIAS, "upFieldsByFieldAlias" )
   INT32 upFieldsByFieldAlias( qgmOPFieldVec & fields,
                               const qgmOPFieldPtrVec & fieldAlias,
                               BOOLEAN needClearAlias )
   {
      PD_TRACE_ENTRY( SDB__QGMUPFIELDSBYFIELDALIAS ) ;
      INT32 rc = SDB_OK ;
      qgmOPFieldVec::iterator it = fields.begin() ;
      while ( it != fields.end() )
      {
         if ( SQL_GRAMMAR::WILDCARD != (*it).type )
         {
            upAFieldByFieldAlias( *it, fieldAlias, needClearAlias ) ;
         }
         ++it ;
      }

      PD_TRACE_EXITRC( SDB__QGMUPFIELDSBYFIELDALIAS, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMUPATTRSBYFIELDALIAS, "upAttrsByFieldAlias" )
   INT32 upAttrsByFieldAlias( qgmDbAttrPtrVec & attrs,
                              const qgmOPFieldPtrVec & fieldAlias )
   {
      PD_TRACE_ENTRY( SDB__QGMUPATTRSBYFIELDALIAS ) ;
      INT32 rc = SDB_OK ;
      UINT32 pos = 0 ;
      qgmDbAttrPtrVec::iterator it = attrs.begin() ;
      while ( it != attrs.end() )
      {
         qgmDbAttr &attr = *(*it) ;

         qgmOPFieldPtrVec::const_iterator cit = fieldAlias.begin() ;
         while ( cit != fieldAlias.end() )
         {
            if ( attr == (*cit)->value )
            {
               if ( (*cit)->alias.empty() )
               {
                  attr.relegation().clear() ;
               }
               else if ( attr.attr().isSubfix( (*cit)->alias, FALSE, &pos ) )
               {
                  attr.attr().replace( 0, pos, (*cit)->alias ) ;
               }
               else
               {
                  attr.attr() = (*cit)->alias ;
               }
               break ;
            }
            ++cit ;
         }
         ++it ;
      }

      PD_TRACE_EXITRC( SDB__QGMUPATTRSBYFIELDALIAS, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMUPATTRSBYFIELDALIAS2, "upAttrsByFieldAlias" )
   INT32 upAttrsByFieldAlias( qgmDbAttrVec & attrs,
                              const qgmOPFieldPtrVec & fieldAlias )
   {
      PD_TRACE_ENTRY( SDB__QGMUPATTRSBYFIELDALIAS2 ) ;
      INT32 rc = SDB_OK ;
      UINT32 pos = 0 ;
      qgmDbAttrVec::iterator it = attrs.begin() ;
      while ( it != attrs.end() )
      {
         qgmDbAttr &attr = *it ;

         qgmOPFieldPtrVec::const_iterator cit = fieldAlias.begin() ;
         while ( cit != fieldAlias.end() )
         {
            if ( attr == (*cit)->value )
            {
               if ( (*cit)->alias.empty() )
               {
                  attr.relegation().clear() ;
               }
               else if ( attr.attr().isSubfix( (*cit)->alias, FALSE, &pos ) )
               {
                  attr.attr().replace( 0, pos, (*cit)->alias ) ;
               }
               else
               {
                  attr.attr() = (*cit)->alias ;
               }
               break ;
            }
            ++cit ;
         }
         ++it ;
      }

      PD_TRACE_EXITRC( SDB__QGMUPATTRSBYFIELDALIAS2, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMUPAATTRBYFIELDALIAS, "upAAttrByFieldAlias" )
   INT32 upAAttrByFieldAlias( qgmDbAttr & attr,
                              const qgmOPFieldPtrVec & fieldAlias )
   {
      PD_TRACE_ENTRY( SDB__QGMUPAATTRBYFIELDALIAS ) ;
      INT32 rc = SDB_OK ;
      UINT32 pos = 0 ;
      qgmOPFieldPtrVec::const_iterator cit = fieldAlias.begin() ;
      while ( cit != fieldAlias.end() )
      {
         if ( attr == (*cit)->value )
         {
            if ( (*cit)->alias.empty() )
            {
               attr.relegation().clear() ;
            }
            else if ( attr.attr().isSubfix( (*cit)->alias, FALSE, &pos ) )
            {
               attr.attr().replace( 0, pos, (*cit)->alias ) ;
            }
            else
            {
               attr.attr() = (*cit)->alias ;
            }
            break ;
         }
         ++cit ;
      }

      PD_TRACE_EXITRC( SDB__QGMUPAATTRBYFIELDALIAS, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMUPATTRSBYFIELDALIAS3, "upAggrsByFieldAlias" )
   INT32 upAggrsByFieldAlias( qgmAggrSelectorVec & aggrs,
                              const qgmOPFieldPtrVec & fieldAlias )
   {
      PD_TRACE_ENTRY( SDB__QGMUPATTRSBYFIELDALIAS3 ) ;
      INT32 rc = SDB_OK ;
      qgmAggrSelectorVec::iterator it = aggrs.begin() ;
      while ( it != aggrs.end() )
      {
         qgmAggrSelector &selector = *it ;

         if ( SQL_GRAMMAR::FUNC == selector.value.type )
         {
            qgmOPFieldVec::iterator iter = selector.param.begin();
            while ( iter != selector.param.end() )
            {
               upAAttrByFieldAlias( iter->value, fieldAlias );
               ++iter;
            }
         }
         else if ( SQL_GRAMMAR::WILDCARD != selector.value.type )
         {
            upAFieldByFieldAlias( selector.value, fieldAlias, TRUE ) ;
         }

         ++it ;
      }

      PD_TRACE_EXITRC( SDB__QGMUPATTRSBYFIELDALIAS3, rc ) ;
      return rc ;
   }

   ossPoolString qgmHintToString( const QGM_HINS &hint )
   {
      StringBuilder ss ;
      QGM_HINS::const_iterator i = hint.begin() ;
      for ( ; i != hint.end(); ++i )
      {
         ss << i->value.toString() << "(" ;
         qgmOPFieldVec::const_iterator j = i->param.begin() ;
         for ( ; j != i->param.end(); ++j )
         {
            ss << j->value.toString() << "," ;
         }
         ss << ") " ;
      }
      return ss.poolStr() ;
   }

   void qgmUseIndexHintToBson( const qgmHint &h, BSONObjBuilder &build )
   {
      qgmField f ;
      if ( 1 == h.param.size() )
      {
         f = h.param.at( 0 ).value.attr() ;
      }
      else if ( 2 == h.param.size() )
      {
         f = h.param.at( 1 ).value.attr() ;
      }
      else
      {
         goto done ;
      }

      if ( TABLE_SCAN_SIZE == f.size() &&
           0 == ossStrncmp( f.begin(), TABLE_SCAN, f.size() ) )
      {
         build.appendNull("") ;
         goto done ;
      }
      if ( TABLE_SCAN_LOWER_SIZE == f.size() &&
           0 == ossStrncmp( f.begin(), TABLE_SCAN_LOWER, f.size() ) )
      {
         build.appendNull("") ;
         goto done ;
      }
      build.appendStrWithNoTerminating( "", f.begin(), f.size() ) ;

   done:
      return ;
   }

   void qgmUseOptionToBson( const qgmHint &h, BSONObjBuilder &build )
   {
      qgmField key ;
      qgmField value ;

      if ( 2 == h.param.size() )
      {
         key = h.param.at(0).value.attr() ;
         value = h.param.at(1).value.attr() ;
      }
      else
      {
         goto done ;
      }

      build.appendStrWithNoTerminating( key.toString(), value.begin(),
                                        value.size() ) ;

   done:
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMUSEHINTTOFLAG, "qgmUseHintToFlag" )
   INT32 qgmUseHintToFlag( const qgmHint &h, INT32 &flag )
   {
      PD_TRACE_ENTRY( SDB__QGMUSEHINTTOFLAG ) ;
      INT32 rc = SDB_OK ;
      const CHAR *strFlag = NULL ;
      qgmField f ;

      if ( 1 == h.param.size() )
      {
         f = h.param.at( 0 ).value.attr() ;
      }
      else
      {
         goto done ;
      }

      // treat it as string flag
      if ( FLG_SQL_UPDATE_KEEP_SK_SIZE == f.size() &&
           0 == ossStrncmp( f.begin(), FLG_SQL_UPDATE_KEEP_SK, f.size() ) )
      {
         flag = FLG_UPDATE_KEEP_SHARDINGKEY ;
         goto done ;
      }

      // treat it as number flag
      strFlag = f.toString().c_str() ;
      rc = utilStr2Num( strFlag, flag );
      if ( rc )
      {
         PD_LOG( PDERROR, "Fail to convert %s to int flag, rc: %d",
                 strFlag, rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB__QGMUSEHINTTOFLAG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   const CHAR* qgmGetNodeTypeStr( INT32 type )
   {
      switch( type )
      {
         case SQL_GRAMMAR::EG:
            return "$et" ;
         case SQL_GRAMMAR::NE:
            return "$ne" ;
         case SQL_GRAMMAR::LT:
            return "$lt" ;
         case SQL_GRAMMAR::GT:
            return "$gt" ;
         case SQL_GRAMMAR::LTE:
            return "$lte" ;
         case SQL_GRAMMAR::GTE:
            return "$gte" ;
         case SQL_GRAMMAR::IS:
            return "$et" ;
         case SQL_GRAMMAR::ISNOT:
            return "$ne" ;
         case SQL_GRAMMAR::INN:
            return "$in" ;
         case SQL_GRAMMAR::LIKE:
            return "$regex" ;
         case SQL_GRAMMAR::AND:
            return "$and" ;
         case SQL_GRAMMAR::OR:
            return "$or" ;
         case SQL_GRAMMAR::NOT:
            return "$not" ;
         default:
            break ;
      }

      return "" ;
   }

   INT32 qgmBuildANodeItem( BSONObjBuilder &bb,
                            const CHAR *pKeyName,
                            const _qgmConditionNode *node,
                            BOOLEAN keepAlias )
   {
      INT32 rc = SDB_OK ;

      if ( sqlIsCommonValue( node->type ) )
      {
         rc = qgmParseValue( node->type, node->value.toString(),
                             bb, pKeyName ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Parse value failed, rc: %d", rc ) ;
         }
      }
      else if ( SQL_GRAMMAR::DBATTR == node->type )
      {
         BSONObjBuilder subBB( bb.subobjStart( pKeyName ) ) ;
         subBB.append( "$field", keepAlias ? node->value.toString() :
                                             node->value.attr().toString() ) ;
         subBB.done() ;
      }
      else if ( node->type > SQL_GRAMMAR::SQLMAX )
      {
         if ( NULL != node->var && !node->var->eoo() )
         {
            bb.appendAs( *(node->var), pKeyName ) ;
         }
         else
         {
            bb.append( pKeyName, "$var" ) ;
         }
      }
      else
      {
         PD_LOG( PDERROR, "Node[Type:%d] is unknow", node->type ) ;
         rc = SDB_INVALIDARG ;
      }

      return rc ;
   }

   INT32 qgmParseValue( INT32 type,
                        const ossPoolString &value,
                        BSONObjBuilder &builder,
                        const ossPoolString &fieldName )
   {
      INT32 rc = SDB_OK ;

      try
      {
         if ( SQL_GRAMMAR::DIGITAL == type )
         {
            BOOLEAN r = FALSE ;
            r = builder.appendAsNumber( fieldName, value ) ;
            if ( !r )
            {
               /// try decimal
               r = builder.appendDecimal( fieldName, value ) ;
            }

            if ( !r )
            {
               PD_LOG( PDERROR, "Failed to append as number: %s",
                       value.c_str() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }
         else if ( SQL_GRAMMAR::STR == type )
         {
            builder.append( fieldName, value ) ;
         }
         else if ( SQL_GRAMMAR::DATE == type )
         {
            bson::Date_t t ;
            UINT64 millis = 0 ;
            rc = utilStr2Date( value.c_str(), millis ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to parse to Date_t: %s",
                       value.c_str() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            t.millis = millis ;
            builder.appendDate( fieldName, t ) ;
         }
         else if ( SQL_GRAMMAR::TIMESTAMP == type )
         {
            time_t tTime = 0 ;
            UINT64 usec = 0 ;
            INT64 millsec = 0 ;
            rc = utilStr2TimeT( value.c_str(), tTime, &usec ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to parse to Time_t: %s",
                       value.c_str() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            millsec = tTime * 1000 ;
            builder.appendTimestamp( fieldName, millsec, (UINT32)usec ) ;
         }
         else if ( SQL_GRAMMAR::OID == type )
         {
            if ( !utilIsValidOID( value.c_str() ) )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Value[%s] is invalid oid type",
                       value.c_str() ) ;
               goto error ;
            }
            else
            {
               OID tmpOid ;
               tmpOid.init( value.c_str() ) ;
               builder.appendOID( fieldName, &tmpOid ) ;
            }
         }
         else if ( SQL_GRAMMAR::DECIMAL == type )
         {
            if ( !builder.appendDecimal( fieldName, value ) )
            {
               PD_LOG( PDERROR, "Failed to append decimal: %s",
                       value.c_str() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }
         else if ( SQL_GRAMMAR::NULLL == type )
         {
            builder.appendNull( fieldName ) ;
         }
         else if ( SQL_GRAMMAR::BOOL_TRUE == type )
         {
            builder.appendBool( fieldName, TRUE ) ;
         }
         else if ( SQL_GRAMMAR::BOOL_FALSE == type )
         {
            builder.appendBool( fieldName, FALSE ) ;
         }
         else
         {
            PD_LOG( PDERROR, "wrong type: %d", type ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened :%s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 qgmParseValue( const qgmOpField &value,
                        BSONObjBuilder &builder,
                        const ossPoolString &fieldName )
   {
      ossPoolString str = value.value.toString() ;
      return qgmParseValue( value.type, str, builder, fieldName ) ;
   }

   INT32 qgmParseValue( const SQL_CON_ITR &root,
                        BSONObjBuilder &builder,
                        const ossPoolString &fieldName )
   {
      INT32 type = (INT32)(root->value.id().to_long()) ;
      ossPoolString value ;

      if ( sqlIsNestedValue( type ) )
      {
         SDB_ASSERT( 1 == root->children.size(), "impossible" ) ;
         SQL_CON_ITR itr = root->children.begin() ;
         value = ossPoolString( itr->value.begin(), itr->value.end() ) ;
      }
      else
      {
         SDB_ASSERT( root->children.empty(), "impossible" ) ;
         value = ossPoolString( root->value.begin(), root->value.end() ) ;
      }

      return qgmParseValue( type, value, builder, fieldName ) ;
   }

   BOOLEAN  sqlIsCommonValue( INT32 type )
   {
      if ( SQL_GRAMMAR::DIGITAL == type ||
           SQL_GRAMMAR::STR == type ||
           SQL_GRAMMAR::NULLL == type ||
           SQL_GRAMMAR::OID == type ||
           SQL_GRAMMAR::DATE == type ||
           SQL_GRAMMAR::TIMESTAMP == type ||
           SQL_GRAMMAR::DECIMAL == type ||
           SQL_GRAMMAR::BOOL_FALSE == type ||
           SQL_GRAMMAR::BOOL_TRUE == type )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   BOOLEAN  sqlIsNestedValue( INT32 type )
   {
      if ( SQL_GRAMMAR::OID == type ||
           SQL_GRAMMAR::DECIMAL == type ||
           SQL_GRAMMAR::DATE == type ||
           SQL_GRAMMAR::TIMESTAMP == type )
      {
         return TRUE ;
      }
      return FALSE ;
   }

}

