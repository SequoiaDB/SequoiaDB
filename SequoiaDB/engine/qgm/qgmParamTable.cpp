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

   Source File Name = qgmParamTable.cpp

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

*******************************************************************************/

#include "qgmParamTable.hpp"
#include "pd.hpp"
#include "utilStr.hpp"
#include "qgmUtil.hpp"
#include "pdTrace.hpp"
#include "qgmTrace.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{
   _qgmParamTable::_qgmParamTable()
   {

   }

   _qgmParamTable::~_qgmParamTable()
   {
      _const.clear() ;
      _var.clear() ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMPARAMTABLE_ADDCONST, "_qgmParamTable::addConst" )
   INT32 _qgmParamTable::addConst( const qgmOpField &value,
                                   const BSONElement *&out )
   {
      PD_TRACE_ENTRY( SDB__QGMPARAMTABLE_ADDCONST ) ;
      INT32 rc = SDB_OK ;

      BSONObjBuilder builder ;
      _qgmBsonPair bPair ;

      rc = qgmParseValue( value, builder, "$const" ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Parse value[%d,%s] failed, rc: %d",
                 value.type, value.value.toString().c_str(), rc ) ;
         goto error ;
      }

      bPair.obj = builder.obj() ;
      _const.push_back( bPair ) ;

      {
         _qgmBsonPair &p = *( _const.rbegin() ) ;
         p.ele = p.obj.firstElement() ;
         out = &(p.ele) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__QGMPARAMTABLE_ADDCONST, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMPARAMTABLE_ADDCONST2, "_qgmParamTable::addConst" )
   INT32 _qgmParamTable::addConst( const BSONObj &obj,
                                   const BSONElement *&out )
   {
      PD_TRACE_ENTRY( SDB__QGMPARAMTABLE_ADDCONST2 ) ;
      INT32 rc = SDB_OK ;
      _qgmBsonPair bPair ;
      try
      {
         bPair.obj = obj.getOwned() ;
         _const.push_back( bPair ) ;
         _qgmBsonPair &p = *( _const.rbegin() ) ;
         p.ele = p.obj.firstElement() ;
         out = &(p.ele) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
   
      }
   done:
      PD_TRACE_EXITRC( SDB__QGMPARAMTABLE_ADDCONST2, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMPARATABLE_ADDVAR, "_qgmParamTable::addVar" )
   INT32 _qgmParamTable::addVar( const qgmDbAttr &key,
                                 const BSONElement *&out,
                                 BOOLEAN *pExisted )
   {
      PD_TRACE_ENTRY( SDB__QGMPARATABLE_ADDVAR ) ;
      INT32 rc = SDB_OK ;

      pair<QGM_VAR_TABLE::iterator, BOOLEAN> rInsert =
                        _var.insert( std::make_pair(key, BSONElement()) ) ;
      if ( !rInsert.second )
      {
         PD_LOG( PDDEBUG, "repeat name was found:%s",
                 key.toString().c_str() ) ;
         if ( pExisted )
         {
            *pExisted = TRUE ;
         }
      }

      out = &( rInsert.first->second ) ;
      PD_TRACE_EXITRC( SDB__QGMPARATABLE_ADDVAR, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMPARAMTABLE_SETVAR, "_qgmParamTable::setVar" )
   INT32 _qgmParamTable::setVar( const varItem &item,
                                 const BSONObj &obj )
   {
      PD_TRACE_ENTRY( SDB__QGMPARAMTABLE_SETVAR ) ;
      INT32 rc = SDB_OK ;
      try
      {
         BSONElement ele = obj.getField( item._fieldName.attr().toString() ) ;
         if ( ele.eoo() )
         {
            PD_LOG( PDDEBUG, "key[%s] was not found in bson obj",
                    item._fieldName.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         {
         QGM_VAR_TABLE::iterator itr = _var.find( item._varName ) ;
         if ( _var.end() == itr )
         {
            PD_LOG( PDDEBUG, "key[%s] was not found in var table",
                    item._varName.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         else
         {
            itr->second = ele ;
         }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happpend: %s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__QGMPARAMTABLE_SETVAR, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}
