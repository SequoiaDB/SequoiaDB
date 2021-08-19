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

   Source File Name = sptDBCL.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          24/10/2017  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptDBCL.hpp"
#include "sptDBCS.hpp"
#include "sptDBCursor.hpp"
#include "sptDBOptionBase.hpp"
#include "sptDBQueryOption.hpp"
#include "msg.h"
#include "msgDef.h"
#include "ossFile.hpp"
#include "utilStr.hpp"
#include "sptDBTimestamp.hpp"

using namespace bson ;
using namespace sdbclient ;
namespace engine
{
   #define LOB_BUFFER_LEN  (2*1024*1024)
   #define SPT_CL_NAME  "SdbCollection"
   JS_CONSTRUCT_FUNC_DEFINE( _sptDBCL, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptDBCL, destruct )
   JS_MEMBER_FUNC_DEFINE( _sptDBCL, rawFind )
   JS_MEMBER_FUNC_DEFINE( _sptDBCL, insert )
   JS_MEMBER_FUNC_DEFINE( _sptDBCL, update )
   JS_MEMBER_FUNC_DEFINE( _sptDBCL, upsert )
   JS_MEMBER_FUNC_DEFINE( _sptDBCL, remove )
   JS_MEMBER_FUNC_DEFINE( _sptDBCL, pop )
   JS_MEMBER_FUNC_DEFINE( _sptDBCL, count )
   JS_MEMBER_FUNC_DEFINE( _sptDBCL, createIndex )
   JS_MEMBER_FUNC_DEFINE( _sptDBCL, getIndexes )
   JS_MEMBER_FUNC_DEFINE( _sptDBCL, dropIndex )
   JS_MEMBER_FUNC_DEFINE( _sptDBCL, bulkInsert )
   JS_MEMBER_FUNC_DEFINE( _sptDBCL, split )
   JS_MEMBER_FUNC_DEFINE( _sptDBCL, splitAsync )
   JS_MEMBER_FUNC_DEFINE( _sptDBCL, aggregate )
   JS_MEMBER_FUNC_DEFINE( _sptDBCL, alter )
   JS_MEMBER_FUNC_DEFINE( _sptDBCL, attachCL )
   JS_MEMBER_FUNC_DEFINE( _sptDBCL, detachCL )
   JS_MEMBER_FUNC_DEFINE( _sptDBCL, explain )
   JS_MEMBER_FUNC_DEFINE( _sptDBCL, putLob )
   JS_MEMBER_FUNC_DEFINE( _sptDBCL, getLob )
   JS_MEMBER_FUNC_DEFINE( _sptDBCL, getLobRTimeDetail )
   JS_MEMBER_FUNC_DEFINE( _sptDBCL, deleteLob )
   JS_MEMBER_FUNC_DEFINE( _sptDBCL, listLobs )
   JS_MEMBER_FUNC_DEFINE( _sptDBCL, createLobID )
   JS_MEMBER_FUNC_DEFINE( _sptDBCL, listLobPieces )
   JS_MEMBER_FUNC_DEFINE( _sptDBCL, truncateLob )
   JS_MEMBER_FUNC_DEFINE( _sptDBCL, truncate )
   JS_MEMBER_FUNC_DEFINE( _sptDBCL, createIdIndex )
   JS_MEMBER_FUNC_DEFINE( _sptDBCL, dropIdIndex )
   JS_MEMBER_FUNC_DEFINE( _sptDBCL, createAutoIncrement )
   JS_MEMBER_FUNC_DEFINE( _sptDBCL, dropAutoIncrement )
   JS_MEMBER_FUNC_DEFINE( _sptDBCL, getQueryMeta )
   JS_MEMBER_FUNC_DEFINE( _sptDBCL, enableSharding )
   JS_MEMBER_FUNC_DEFINE( _sptDBCL, disableSharding )
   JS_MEMBER_FUNC_DEFINE( _sptDBCL, enableCompression )
   JS_MEMBER_FUNC_DEFINE( _sptDBCL, disableCompression )
   JS_MEMBER_FUNC_DEFINE( _sptDBCL, setAttributes )
   JS_MEMBER_FUNC_DEFINE( _sptDBCL, getDetail )

   JS_BEGIN_MAPPING( _sptDBCL, SPT_CL_NAME )
      JS_ADD_CONSTRUCT_FUNC( construct )
      JS_ADD_DESTRUCT_FUNC( destruct )
      JS_ADD_MEMBER_FUNC( "rawFind", rawFind )
      JS_ADD_MEMBER_FUNC( "_insert", insert )
      JS_ADD_MEMBER_FUNC( "update", update )
      JS_ADD_MEMBER_FUNC( "upsert", upsert )
      JS_ADD_MEMBER_FUNC( "remove", remove )
      JS_ADD_MEMBER_FUNC( "pop", pop )
      JS_ADD_MEMBER_FUNC( "_count", count )
      JS_ADD_MEMBER_FUNC( "createIndex", createIndex )
      JS_ADD_MEMBER_FUNC( "_getIndexes", getIndexes )
      JS_ADD_MEMBER_FUNC( "dropIndex", dropIndex )
      JS_ADD_MEMBER_FUNC( "_bulkInsert", bulkInsert )
      JS_ADD_MEMBER_FUNC( "split", split )
      JS_ADD_MEMBER_FUNC( "splitAsync", splitAsync )
      JS_ADD_MEMBER_FUNC( "aggregate", aggregate )
      JS_ADD_MEMBER_FUNC( "alter", alter )
      JS_ADD_MEMBER_FUNC( "attachCL", attachCL )
      JS_ADD_MEMBER_FUNC( "detachCL", detachCL )
      JS_ADD_MEMBER_FUNC( "explain", explain )
      JS_ADD_MEMBER_FUNC( "putLob", putLob )
      JS_ADD_MEMBER_FUNC( "getLob", getLob )
      JS_ADD_MEMBER_FUNC( "getLobDetail", getLobRTimeDetail )
      JS_ADD_MEMBER_FUNC( "deleteLob", deleteLob )
      JS_ADD_MEMBER_FUNC( "listLobs", listLobs )
      JS_ADD_MEMBER_FUNC( "createLobID", createLobID )
      JS_ADD_MEMBER_FUNC( "listLobPieces", listLobPieces )
      JS_ADD_MEMBER_FUNC( "truncate", truncate )
      JS_ADD_MEMBER_FUNC( "createIdIndex", createIdIndex )
      JS_ADD_MEMBER_FUNC( "dropIdIndex", dropIdIndex )
      JS_ADD_MEMBER_FUNC( "createAutoIncrement", createAutoIncrement )
      JS_ADD_MEMBER_FUNC( "dropAutoIncrement", dropAutoIncrement )
      JS_ADD_MEMBER_FUNC( "getQueryMeta", getQueryMeta )
      JS_ADD_MEMBER_FUNC( "enableSharding", enableSharding )
      JS_ADD_MEMBER_FUNC( "disableSharding", disableSharding )
      JS_ADD_MEMBER_FUNC( "enableCompression", enableCompression)
      JS_ADD_MEMBER_FUNC( "disableCompression", disableCompression )
      JS_ADD_MEMBER_FUNC( "setAttributes", setAttributes )
      JS_ADD_MEMBER_FUNC( "truncateLob", truncateLob )
      JS_ADD_MEMBER_FUNC( "getDetail", getDetail )
      JS_SET_CVT_TO_BSON_FUNC( _sptDBCL::cvtToBSON )
      JS_SET_JSOBJ_TO_BSON_FUNC( _sptDBCL::fmpToBSON )
      JS_SET_BSON_TO_JSOBJ_FUNC( _sptDBCL::bsonToJSObj )
   JS_MAPPING_END()

   _sptDBCL::_sptDBCL( _sdbCollection *pCL )
   {
      _cl.pCollection = pCL ;
   }

   _sptDBCL::~_sptDBCL()
   {
   }

   INT32 _sptDBCL::construct( const _sptArguments &arg,
                              _sptReturnVal &rval,
                              bson::BSONObj &detail )
   {
      detail = BSON( SPT_ERR << "use of new SdbCollection() is forbidden, you should use "
                       "other functions to produce a SdbCollection object" ) ;
      return SDB_SYS ;
   }

   INT32 _sptDBCL::destruct()
   {
      return SDB_OK ;
   }

   INT32 _sptDBCL::rawFind( const _sptArguments &arg,
                            _sptReturnVal &rval,
                            bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;
      BSONObj cond ;
      BSONObj sel ;
      BSONObj order ;
      BSONObj hint ;
      BSONObj options ;
      INT32 numToSkip = 0 ;
      INT32 numToRet = -1 ;
      INT32 flags = 0 ;
      _sdbCursor *pCursor = NULL ;
      string objectName ;

      if( !arg.isNull( 0 ) )
      {
         objectName = arg.getUserObjClassName( 0 ) ;
      }

      if ( SPT_OPTIONBASE_NAME == objectName ||
           SPT_QUERYOPTION_NAME == objectName )
      {
         rc = arg.getBsonobj( 0, obj ) ;
         if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
         {
            detail = BSON( SPT_ERR << "Cond must be obj" ) ;
            goto error ;
         }

         if ( obj.hasField( SPT_OPTIONBASE_COND_FIELD ) )
         {
            cond = obj.getObjectField( SPT_OPTIONBASE_COND_FIELD ) ;
         }
         if ( obj.hasField( SPT_OPTIONBASE_SEL_FIELD ) )
         {
            sel = obj.getObjectField( SPT_OPTIONBASE_SEL_FIELD ) ;
         }
         if ( obj.hasField( SPT_OPTIONBASE_SORT_FIELD ) )
         {
            order = obj.getObjectField( SPT_OPTIONBASE_SORT_FIELD ) ;
         }
         if ( obj.hasField( SPT_OPTIONBASE_HINT_FIELD ) )
         {
            hint = obj.getObjectField( SPT_OPTIONBASE_HINT_FIELD ) ;
         }
         if ( obj.hasField( SPT_OPTIONBASE_SKIP_FIELD ) )
         {
            numToSkip = obj.getIntField( SPT_OPTIONBASE_SKIP_FIELD ) ;
         }
         if ( obj.hasField( SPT_OPTIONBASE_LIMIT_FIELD ) )
         {
            numToRet = obj.getIntField( SPT_OPTIONBASE_LIMIT_FIELD ) ;
         }
         if ( obj.hasField( SPT_QUERYOPTION_OPTIONS_FIELD ) )
         {
            options = obj.getObjectField( SPT_QUERYOPTION_OPTIONS_FIELD ) ;
         }
      }
      else
      {
         if( !arg.isNull( 0 ) )
         {
            rc = arg.getBsonobj( 0, cond, FALSE ) ;
            if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
            {
               detail = BSON( SPT_ERR << "Cond must be obj" ) ;
               goto error ;
            }
         }
         if( !arg.isNull( 1 ) )
         {
            rc = arg.getBsonobj( 1, sel ) ;
            if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
            {
               detail = BSON( SPT_ERR << "Sel must be obj" ) ;
               goto error ;
            }
         }
         if( !arg.isNull( 2 ) )
         {
            rc = arg.getBsonobj( 2, order ) ;
            if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
            {
               detail = BSON( SPT_ERR << "Sort must be obj" ) ;
               goto error ;
            }
         }
         if( !arg.isNull( 3 ) )
         {
            rc = arg.getBsonobj( 3, hint ) ;
            if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
            {
               detail = BSON( SPT_ERR << "Hint must be obj" ) ;
               goto error ;
            }
         }
         if( !arg.isNull( 4 ) )
         {
            rc = arg.getNative( 4, &numToSkip, SPT_NATIVE_INT32 ) ;
            if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
            {
               detail = BSON( SPT_ERR << "SkipNum must be number" ) ;
               goto error ;
            }
         }
         if( !arg.isNull( 5 ) )
         {
            rc = arg.getNative( 5, &numToRet, SPT_NATIVE_INT32 ) ;
            if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
            {
               detail = BSON( SPT_ERR << "RetNum must be number" ) ;
               goto error ;
            }
         }
         if( !arg.isNull( 6 ) )
         {
            rc = arg.getNative( 6, &flags, SPT_NATIVE_INT32 ) ;
            if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
            {
               detail = BSON( SPT_ERR << "Flags must be number" ) ;
               goto error ;
            }
         }
         if( !arg.isNull( 7 ) )
         {
            rc = arg.getBsonobj( 7, options ) ;
            if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
            {
               detail = BSON( SPT_ERR << "Options must be obj" ) ;
               goto error ;
            }
         }

      }

      // in case of "query and modify"
      if ( Object == hint.getField( FIELD_NAME_MODIFY ).type() )
      {
         BSONObj filter = BSON( FIELD_NAME_MODIFY << "" ) ;
         BSONObj hintObj = hint.filterFieldsUndotted( filter, FALSE ) ;
         BSONObj modifyObj = hint.filterFieldsUndotted( filter, TRUE ) ;
         BSONObj modifyObjVal ;
         string opType ;

         // get modify obj
         BSONElement ele = modifyObj.getField( FIELD_NAME_MODIFY ) ;
         if ( Object != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR <<
                           "$Modify in 'query and modify' should be an object" ) ;
            goto error ;
         }
         modifyObjVal = ele.Obj() ;
         // get op type
         ele = modifyObjVal.getField( FIELD_NAME_OP ) ;
         if ( String != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR <<
                           "OP type in 'query and modify' should be a string" ) ;
            goto error ;
         }
         opType = ele.String() ;
         if ( FIELD_OP_VALUE_UPDATE == opType )
         {
            BSONObj rule ;
            BOOLEAN returnNew = FALSE ;
            // get update rule
            ele = modifyObjVal.getField( FIELD_NAME_OP_UPDATE ) ;
            if ( Object != ele.type() )
            {
               rc = SDB_INVALIDARG ;
               detail = BSON( SPT_ERR <<
                              "'query and update' has no update rule" ) ;
               goto error ;
            }
            rule = ele.Obj() ;
            // get returnNew
            ele = modifyObjVal.getField( FIELD_NAME_RETURNNEW ) ;
            if ( Bool != ele.type() )
            {
               rc = SDB_INVALIDARG ;
               detail = BSON( SPT_ERR <<
                              "'query and update' does not specify returnNew" ) ;
               goto error ;
            }
            returnNew = ele.Bool() ;
            // get options
            if ( options.hasField( FIELD_NAME_KEEP_SHARDING_KEY ) )
            {
               ele = options.getField( FIELD_NAME_KEEP_SHARDING_KEY ) ;
               if( Bool != ele.type() )
               {
                  rc = SDB_INVALIDARG ;
                  detail = BSON( SPT_ERR << FIELD_NAME_KEEP_SHARDING_KEY
                                            " must be bool in options" ) ;
                  goto error ;
               }
               if( TRUE == ele.Bool() )
               {
                  flags |= QUERY_KEEP_SHARDINGKEY_IN_UPDATE ;
               }
            }
            // execute
            rc = _cl.pCollection->queryAndUpdate( &pCursor, rule, cond, sel,
                                                  order, hintObj, numToSkip,
                                                  numToRet, flags, returnNew ) ;
            if( SDB_OK != rc && SDB_DMS_EOC != rc )
            {
               detail = BSON( SPT_ERR <<
                              "Failed to query and update in collection" ) ;
               goto error ;
            }
         }
         else if ( FIELD_OP_VALUE_REMOVE == opType )
         {
            // execute
            rc = _cl.pCollection->queryAndRemove( &pCursor, cond, sel,
                                                  order, hintObj,
                                                  numToSkip, numToRet, flags ) ;
            if( SDB_OK != rc && SDB_DMS_EOC != rc )
            {
               detail = BSON( SPT_ERR <<
                              "Failed to query and remove in collection" ) ;
               goto error ;
            }
         }
         else
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR <<
                           "invalid OP type for 'query and modify'" ) ;
            goto error ;
         }
      }
      else // in case of "query" only
      {
         rc = _cl.pCollection->query( &pCursor, cond, sel, order, hint,
                                      numToSkip, numToRet, flags ) ;
         if( SDB_OK != rc && SDB_DMS_EOC != rc )
         {
            detail = BSON( SPT_ERR << "Failed to query collection" ) ;
            goto error ;
         }
      }
      SPT_SET_CURSOR_TO_RETURNVAL( pCursor ) ;
   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pCursor ) ;
      goto done ;
   }

   INT32 _sptDBCL::_parseInsertOptions( const _sptArguments &arg, SINT32 &flags,
                                        bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;

      flags = 0 ;
      if ( arg.isNumber( 1 ) )
      {
         rc = arg.getNative( 1, &flags, SPT_NATIVE_INT32 ) ;
         if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
         {
            detail = BSON( SPT_ERR << "Flags must be number" ) ;
            goto error ;
         }
      }
      else if ( arg.isObject( 1 ) )
      {
         BSONObj options ;
         BSONElement elem ;
         BOOLEAN contOnDup = FALSE ;
         BOOLEAN returnOid = FALSE ;
         rc = arg.getBsonobj( 1, options ) ;
         if( SDB_OUT_OF_BOUND == rc )
         {
            detail = BSON( SPT_ERR << "Failed to get insert options" ) ;
            goto error ;
         }

         /// ContOnDup
         elem = options.getField( FIELD_NAME_CONTONDUP ) ;
         contOnDup =
            ( elem.eoo() || Bool != elem.type() ) ? FALSE : elem.Bool() ;
         if ( contOnDup )
         {
            flags |= FLG_INSERT_CONTONDUP ;
         }

         /// ReturnOID
         elem = options.getField( FIELD_NAME_RETURN_OID ) ;
         returnOid =
            ( elem.eoo() || Bool != elem.type() ) ? FALSE : elem.Bool() ;
         if ( returnOid )
         {
            flags |= FLG_INSERT_RETURN_OID ;
         }

         /// replace on duplicate
         elem = options.getField( FIELD_NAME_REPLACEONDUP ) ;
         returnOid =
            ( elem.eoo() || Bool != elem.type() ) ? FALSE : elem.Bool() ;
         if ( returnOid )
         {
            flags |= FLG_INSERT_REPLACEONDUP ;
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "The second argument should be insert flag or insert options" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCL::insert( const _sptArguments &arg,
                           _sptReturnVal &rval,
                           bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      SINT32 flags = 0 ;
      BSONObj record ;
      BSONObj result ;

      rc = arg.getBsonobj( 0, record ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Doc must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         if ( arg.hasErrMsg() )
         {
            detail = BSON( SPT_ERR << arg.getErrMsg().c_str() ) ;
         }
         else
         {
            detail = BSON( SPT_ERR << "Doc must be obj" ) ;
         }
         goto error ;
      }

      rc = _parseInsertOptions( arg, flags, detail ) ;
      if ( SDB_OK != rc )
      {
         // detail have set in _parseInsertOptions() when rc is not ok.
         goto error ;
      }
      flags |= FLG_INSERT_RETURNNUM ;

      rc = _cl.insert( record, flags, &result ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << "Failed to insert record" ) ;
         goto error ;
      }

      if ( !result.isEmpty() )
      {
         rval.getReturnVal().setValue( result ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCL::update( const _sptArguments &arg,
                           _sptReturnVal &rval,
                           bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj rule ;
      BSONObj cond ;
      BSONObj hint ;
      BSONObj options ;
      BSONObj result ;
      INT32 flags = 0 ;
      // Get rule
      if( !arg.isVoid( 0 ) && !arg.isNull( 0 ) )
      {
         rc = arg.getBsonobj( 0, rule ) ;
         if( SDB_OUT_OF_BOUND == rc )
         {
            detail = BSON( SPT_ERR << "Rule must be config" ) ;
            goto error ;
         }
         else if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Rule must be obj" ) ;
            goto error ;
         }
      }
      // Get condition
      if( !arg.isVoid( 1 ) && !arg.isNull( 1 ) )
      {
         rc = arg.getBsonobj( 1, cond ) ;
         if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
         {
            detail = BSON( SPT_ERR << "Cond must be obj" ) ;
            goto error ;
         }
      }
      // Get hint
      if( !arg.isVoid( 2 ) && !arg.isNull( 2 ) )
      {
         rc = arg.getBsonobj( 2, hint ) ;
         if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
         {
            detail = BSON( SPT_ERR << "Hint must be obj" ) ;
            goto error ;
         }
      }
      // Get options
      if( !arg.isVoid( 3 ) && !arg.isNull( 3 ) )
      {
         rc = arg.getBsonobj( 3, options ) ;
         if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
         {
            detail = BSON( SPT_ERR << "Options must be obj" ) ;
            goto error ;
         }
      }
      // Set flags
      if( options.hasField( FIELD_NAME_KEEP_SHARDING_KEY ) )
      {
         BSONElement ele = options.getField( FIELD_NAME_KEEP_SHARDING_KEY ) ;
         if( Bool != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << FIELD_NAME_KEEP_SHARDING_KEY
                                      " must be bool in options" ) ;
            goto error ;
         }
         if( TRUE == ele.Bool() )
         {
            flags |= UPDATE_KEEP_SHARDINGKEY ;
         }
      }
      if( options.hasField( FIELD_NAME_JUSTONE ) )
      {
         BSONElement ele = options.getField( FIELD_NAME_JUSTONE ) ;
         if( Bool != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << FIELD_NAME_JUSTONE
                                      " must be bool in options" ) ;
            goto error ;
         }
         if( TRUE == ele.Bool() )
         {
            flags |= UPDATE_ONE ;
         }
      }
      flags |= UPDATE_RETURNNUM ;
      // Call cpp driver interface
      rc = _cl.update( rule, cond, hint, flags, &result ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to update record" ) ;
         goto error ;
      }

      if ( !result.isEmpty() )
      {
         rval.getReturnVal().setValue( result ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCL::upsert( const _sptArguments &arg,
                           _sptReturnVal &rval,
                           bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj rule ;
      BSONObj cond ;
      BSONObj hint ;
      BSONObj setOnInsert ;
      BSONObj options ;
      BSONObj result ;
      INT32 flags = 0 ;

      rc = arg.getBsonobj( 0, rule ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Update rule must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Update rule must be obj" ) ;
         goto error ;
      }

      if( !arg.isNull( 1 ) )
      {
         rc = arg.getBsonobj( 1, cond ) ;
         if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
         {
            detail = BSON( SPT_ERR << "Condition must be obj" ) ;
            goto error ;
         }
      }
      if( !arg.isNull( 2 ) )
      {
         rc = arg.getBsonobj( 2, hint ) ;
         if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
         {
            detail = BSON( SPT_ERR << "Hint must be obj" ) ;
            goto error ;
         }
      }
      if( !arg.isNull( 3 ) )
      {
         rc = arg.getBsonobj( 3, setOnInsert ) ;
         if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
         {
            detail = BSON( SPT_ERR << "SetOnInsert must be obj" ) ;
            goto error ;
         }
      }
      if( !arg.isNull( 4 ) )
      {
         rc = arg.getBsonobj( 4, options ) ;
         if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
         {
            detail = BSON( SPT_ERR << "Options must be obj" ) ;
            goto error ;
         }
      }
      if( options.hasField( FIELD_NAME_KEEP_SHARDING_KEY ) )
      {
         BSONElement ele = options.getField( FIELD_NAME_KEEP_SHARDING_KEY ) ;
         if( Bool != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << FIELD_NAME_KEEP_SHARDING_KEY
                                      " must be bool in options" ) ;
            goto error ;
         }
         if( TRUE == ele.Bool() )
         {
            flags |= FLG_UPDATE_KEEP_SHARDINGKEY ;
         }
      }
      if( options.hasField( FIELD_NAME_JUSTONE ) )
      {
         BSONElement ele = options.getField( FIELD_NAME_JUSTONE ) ;
         if( Bool != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << FIELD_NAME_JUSTONE
                                      " must be bool in options" ) ;
            goto error ;
         }
         if( TRUE == ele.Bool() )
         {
            flags |= FLG_UPDATE_ONE ;
         }
      }
      flags |= UPDATE_RETURNNUM ;

      rc = _cl.upsert( rule, cond, hint, setOnInsert, flags, &result ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to upsert collection" ) ;
         goto error ;
      }

      if ( !result.isEmpty() )
      {
         rval.getReturnVal().setValue( result ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCL::remove( const _sptArguments &arg,
                           _sptReturnVal &rval,
                           bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj cond ;
      BSONObj hint ;
      BSONObj options ;
      BSONObj result ;
      INT32 flags = 0 ;

      // get condition
      if( !arg.isNull( 0 ) )
      {
         rc = arg.getBsonobj( 0, cond ) ;
         if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
         {
            detail = BSON( SPT_ERR << "Cond must be obj" ) ;
            goto error ;
         }
      }
      // get hint
      if( !arg.isNull( 1 ) )
      {
         rc = arg.getBsonobj( 1, hint ) ;
         if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
         {
            detail = BSON( SPT_ERR << "Hint must be obj" ) ;
            goto error ;
         }
      }
      // get options
      if( !arg.isNull( 2 ) )
      {
         rc = arg.getBsonobj( 2, options ) ;
         if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
         {
            detail = BSON( SPT_ERR << "Options must be obj" ) ;
            goto error ;
         }
      }
      if( options.hasField( FIELD_NAME_JUSTONE ) )
      {
         BSONElement ele = options.getField( FIELD_NAME_JUSTONE ) ;
         if( Bool != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << FIELD_NAME_JUSTONE
                                      " must be bool in options" ) ;
            goto error ;
         }
         if( TRUE == ele.Bool() )
         {
            flags |= FLG_DELETE_ONE ;
         }
      }
      flags |= FLG_DELETE_RETURNNUM ;

      rc = _cl.del( cond, hint, flags, &result ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to remove record" ) ;
         goto error ;
      }

      if ( !result.isEmpty() )
      {
         rval.getReturnVal().setValue( result ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCL::pop( const _sptArguments &arg,
                        _sptReturnVal &rval,
                        bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj options ;

      rc = arg.getBsonobj( 0, options ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Options must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Options must be obj" ) ;
         goto error ;
      }
      rc = _cl.pop( options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to pop collection" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCL::count( const _sptArguments &arg,
                          _sptReturnVal &rval,
                          bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj cond ;
      BSONObj hint ;
      SINT64 count = 0 ;

      rc = arg.getBsonobj( 0, cond, FALSE ) ;
      if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
      {
         detail = BSON( SPT_ERR << "Cond must be obj" ) ;
         goto error ;
      }
      rc = arg.getBsonobj( 1, hint ) ;
      if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
      {
         detail = BSON( SPT_ERR << "Hint must be obj" ) ;
         goto error ;
      }
      rc = _cl.getCount( count, cond, hint ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get count" ) ;
         goto error ;
      }
      rval.getReturnVal().setValue( count ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCL::createIndex( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj indexDef ;
      string indexName ;
      INT32 isUnique = FALSE ;
      INT32 isEnforced = FALSE ;
      INT32 sortBufferSize = SDB_INDEX_SORT_BUFFER_DEFAULT_SIZE ;

      rc = arg.getString( 0, indexName ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Name must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Name must be string" ) ;
         goto error ;
      }

      rc = arg.getBsonobj( 1, indexDef ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "IndexDef must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "IndexDef must be obj" ) ;
         goto error ;
      }

      // arg2 may be options object or native type
      if ( arg.isObject( 2 ) )
      {
         BSONObj options ;
         rc = arg.getBsonobj( 2, options ) ;
         if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
         {
            detail = BSON( SPT_ERR << "Options must be obj" ) ;
            goto error ;
         }

         rc = _cl.createIndex( indexDef, indexName.c_str(), options ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Failed to create index" ) ;
            goto error ;
         }
         goto done ;
      }

      rc = arg.getNative( 2, &isUnique, SPT_NATIVE_INT32 ) ;
      if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
      {
         detail = BSON( SPT_ERR << "IsUnique must be bool" ) ;
         goto error ;
      }

      rc = arg.getNative( 3, &isEnforced, SPT_NATIVE_INT32 ) ;
      if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
      {
         detail = BSON( SPT_ERR << "Enforced must be bool" ) ;
         goto error ;
      }

      rc = arg.getNative( 4, &sortBufferSize, SPT_NATIVE_INT32 ) ;
      if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
      {
         detail = BSON( SPT_ERR << "SortBufferSize must be number" ) ;
         goto error ;
      }

      rc = _cl.createIndex( indexDef, indexName.c_str(), isUnique,
                             isEnforced, sortBufferSize ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to create index" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCL::getIndexes( const _sptArguments &arg,
                              _sptReturnVal &rval,
                              bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string name ;
      _sdbCursor *pCursor = NULL ;
      const CHAR* pName = NULL ;
      rc = arg.getString( 0, name, FALSE ) ;
      if( SDB_OK == rc )
      {
         pName = name.c_str() ;
      }
      else if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
      {
         detail = BSON( SPT_ERR << "Name must be string" ) ;
         goto error ;
      }

      rc = _cl.getIndexes( &pCursor, pName ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get indexs" ) ;
         goto error ;
      }
      SPT_SET_CURSOR_TO_RETURNVAL( pCursor ) ;
   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pCursor ) ;
      goto done ;
   }

   INT32 _sptDBCL::dropIndex( const _sptArguments &arg,
                              _sptReturnVal &rval,
                              bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string name ;
      // Get index name
      rc = arg.getString( 0, name ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Name must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Name must be string" ) ;
         goto error ;
      }
      rc = _cl.dropIndex( name.c_str() ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to drop index" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCL::bulkInsert( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      SINT32 flags = 0 ;
      vector< BSONObj > objVec ;
      BSONObj result ;

      rc = arg.getArray( 0, objVec ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Docs array must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Docs array must be object Array" ) ;
         goto error ;
      }

      rc = _parseInsertOptions( arg, flags, detail ) ;
      if ( SDB_OK != rc )
      {
         // detail have set in _parseInsertOptions() when rc is not ok.
         goto error ;
      }

      flags |= FLG_INSERT_RETURNNUM ;
      rc = _cl.insert( objVec, flags, &result ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << "Failed to insert record" ) ;
         goto error ;
      }

      if ( !result.isEmpty() )
      {
         rval.getReturnVal().setValue( result ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCL::split( const _sptArguments &arg,
                          _sptReturnVal &rval,
                          bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string sourceName ;
      string targetName ;
      BSONObj cond ;
      BSONObj endCond ;

      rc = arg.getString( 0, sourceName ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Source group must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Source group must be string" ) ;
         goto error ;
      }
      rc = arg.getString( 1, targetName ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Target group must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Target group must be string" ) ;
         goto error ;
      }
      if( arg.isNumber( 2 ) )
      {
         FLOAT64 percent = 0 ;
         rc = arg.getNative( 2, &percent, SPT_NATIVE_FLOAT64 ) ;
         if( SDB_OK != SDB_OK )
         {
            detail = BSON( SPT_ERR << "Percent must be double" ) ;
            goto error ;
         }
         rc = _cl.split( sourceName.c_str(), targetName.c_str(), percent ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Failed to split collection" ) ;
            goto error ;
         }
      }
      else if( arg.isObject( 2 ) )
      {
         BSONObj cond ;
         BSONObj endCond ;
         rc = arg.getBsonobj( 2, cond ) ;
         if( SDB_OUT_OF_BOUND == rc )
         {
            detail = BSON( SPT_ERR << "Cond must be config" ) ;
            goto error ;
         }
         else if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Cond must be obj" ) ;
            goto error ;
         }

         if( !arg.isNull( 3 ) )
         {
            rc = arg.getBsonobj( 3, endCond ) ;
            if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
            {
               detail = BSON( SPT_ERR << "EndCondition must be obj" ) ;
               goto error ;
            }
         }
         rc = _cl.split( sourceName.c_str(), targetName.c_str(), cond, endCond ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Failed to split collection" ) ;
            goto error ;
         }
      }
      else
      {
         rc = SDB_OUT_OF_BOUND ;
         detail = BSON( SPT_ERR << "3st param must be double or obj" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCL::splitAsync( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string sourceName ;
      string targetName ;
      SINT64 taskID = 0 ;

      rc = arg.getString( 0, sourceName ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Source group must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Source group must be string" ) ;
         goto error ;
      }

      rc = arg.getString( 1, targetName ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Target group must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Target group must be string" ) ;
         goto error ;
      }

      if( arg.isNumber( 2 ) )
      {
         FLOAT64 percent = 0 ;
         rc = arg.getNative( 2, &percent, SPT_NATIVE_FLOAT64 ) ;
         if( SDB_OK != SDB_OK )
         {
            detail = BSON( SPT_ERR << "Percent must be double" ) ;
            goto error ;
         }

         rc = _cl.splitAsync( sourceName.c_str(), targetName.c_str(),
                              percent, taskID ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Failed to split collection asynchronously" ) ;
            goto error ;
         }
      }
      else if( arg.isObject( 2 ) )
      {
         BSONObj cond ;
         BSONObj endCond ;
         rc = arg.getBsonobj( 2, cond ) ;
         if( SDB_OUT_OF_BOUND == rc )
         {
            detail = BSON( SPT_ERR << "Cond must be config" ) ;
            goto error ;
         }
         else if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Cond must be obj" ) ;
            goto error ;
         }
         rc = arg.getBsonobj( 3, endCond ) ;
         if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
         {
            detail = BSON( SPT_ERR << "EndCondition must be obj" ) ;
            goto error ;
         }
         rc = _cl.splitAsync( taskID, sourceName.c_str(), targetName.c_str(),
                              cond, endCond ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Failed to split collection asynchronously" ) ;
            goto error ;
         }
      }
      else
      {
         rc = SDB_OUT_OF_BOUND ;
         detail = BSON( SPT_ERR << "3st param must be double or obj" ) ;
         goto error ;
      }

      /// return taskid
      rval.getReturnVal().setValue( taskID ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCL::aggregate( const _sptArguments &arg,
                              _sptReturnVal &rval,
                              bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      _sdbCursor *pCursor = NULL ;
      vector< BSONObj > objArr ;
      UINT32 argc = arg.argc() ;
      if( argc < 1 )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "At least one param must be config" ) ;
         goto error ;
      }
      for( UINT32 index = 0; index < argc; index++ )
      {
         BSONObj obj ;

         rc = arg.getBsonobj( index, obj, FALSE ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "SubOp must be obj" ) ;
            goto error ;
         }
         objArr.push_back( obj ) ;
      }
      rc = _cl.aggregate( &pCursor, objArr ) ;
      if( SDB_OK != rc && SDB_DMS_EOC != rc )
      {
         detail = BSON( SPT_ERR << "Failed to arrgregate data" ) ;
         goto error ;
      }
      SPT_SET_CURSOR_TO_RETURNVAL( pCursor ) ;
   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pCursor ) ;
      goto done ;
   }

   INT32 _sptDBCL::alter( const _sptArguments &arg,
                          _sptReturnVal &rval,
                          bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj options ;
      rc = arg.getBsonobj( 0, options ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Options must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Options must be obj" ) ;
         goto error ;
      }
      rc = _cl.alterCollection( options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to alter collection" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCL::attachCL( const _sptArguments &arg,
                             _sptReturnVal &rval,
                             bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string subCLName ;
      BSONObj options ;
      rc = arg.getString( 0, subCLName ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "SubCLFullName must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "SubCLFullName must be string" ) ;
         goto error ;
      }
      rc = arg.getBsonobj( 1, options ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Options must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Options must be obj" ) ;
         goto error ;
      }
      rc = _cl.attachCollection( subCLName.c_str(), options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to attach collection" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCL::detachCL( const _sptArguments &arg,
                             _sptReturnVal &rval,
                             bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string subCLName ;
      BSONObj options ;

      rc = arg.getString( 0, subCLName ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "SubCLFullName must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "SubCLFullName must be string" ) ;
         goto error ;
      }
      rc = _cl.detachCollection( subCLName.c_str() ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to detach collection" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCL::explain( const _sptArguments &arg,
                            _sptReturnVal &rval,
                            bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      _sdbCursor *pCursor = NULL ;
      BSONObj cond ;
      BSONObj sel ;
      BSONObj sort ;
      BSONObj hint ;
      BSONObj options ;
      INT32 skip = 0 ;
      INT32 limit = -1 ;
      INT32 flags = 0 ;

      if( !arg.isNull( 0 ) )
      {
         rc = arg.getBsonobj( 0, cond, FALSE ) ;
         if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
         {
            detail = BSON( SPT_ERR << "Query must be obj" ) ;
            goto error ;
         }
      }
      if( !arg.isNull( 1 ) )
      {
         rc = arg.getBsonobj( 1, sel ) ;
         if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
         {
            detail = BSON( SPT_ERR << "Select must be obj" ) ;
            goto error ;
         }
      }
      if( !arg.isNull( 2 ) )
      {
         rc = arg.getBsonobj( 2, sort ) ;
         if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
         {
            detail = BSON( SPT_ERR << "Sort must be obj" ) ;
            goto error ;
         }
      }
      if( !arg.isNull( 3 ) )
      {
         rc = arg.getBsonobj( 3, hint ) ;
         if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
         {
            detail = BSON( SPT_ERR << "Hint must be obj" ) ;
            goto error ;
         }
      }
      if( !arg.isNull( 4 ) )
      {
         rc = arg.getNative( 4, &skip, SPT_NATIVE_INT32 ) ;
         if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
         {
            detail = BSON( SPT_ERR << "Skip must be int" ) ;
            goto error ;
         }
      }
      if( !arg.isNull( 5 ) )
      {
         rc = arg.getNative( 5, &limit, SPT_NATIVE_INT32 ) ;
         if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
         {
            detail = BSON( SPT_ERR << "Limit must be int" ) ;
            goto error ;
         }
      }
      if( !arg.isNull( 6 ) )
      {
         rc = arg.getNative( 6, &flags, SPT_NATIVE_INT32 ) ;
         if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
         {
            detail = BSON( SPT_ERR << "Flags must be int" ) ;
            goto error ;
         }
      }
      if( hint.hasField( FIELD_NAME_MODIFY ) &&
          Object == hint.getField( FIELD_NAME_MODIFY ).type())
      {
         flags |= FLG_QUERY_MODIFY ;
      }
      if( !arg.isNull( 7 ) )
      {
         rc = arg.getBsonobj( 7, options ) ;
         if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
         {
            detail = BSON( SPT_ERR << "Options must be obj" ) ;
            goto error ;
         }
      }
      rc = _cl.explain( &pCursor, cond, sel, sort, hint, skip,
                        limit, flags, options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get explain" ) ;
         goto error ;
      }
      SPT_SET_CURSOR_TO_RETURNVAL( pCursor ) ;
   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pCursor ) ;
      goto done ;
   }

   INT32 _sptDBCL::getLobRTimeDetail( const _sptArguments &arg,
                                      _sptReturnVal &rval,
                                      bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string oidStr ;
      sdbLob lob ;
      BSONObjBuilder builder ;
      bson::BSONObj lobRunTimeDetail ;

      rc = arg.getString( 0, oidStr ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Oid must be cofig" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Oid must be string" ) ;
         goto error ;
      }
      if ( !utilIsValidOID( oidStr.c_str() ) )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Oid string invalid" ) ;
         goto error ;
      }

      rc = _cl.openLob( lob, OID( oidStr ), SDB_LOB_SHAREREAD ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to open lob" ) ;
         goto error ;
      }

      rc = lob.getRunTimeDetail( lobRunTimeDetail ) ;
      if ( SDB_OK != rc )
      {
         lob.close() ;
         detail = BSON( SPT_ERR << "Failed to get lob runtime detail" ) ;
         goto error ;
      }

      lob.close() ;
      rval.getReturnVal().setValue( lobRunTimeDetail ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCL::putLob( const _sptArguments &arg,
                           _sptReturnVal &rval,
                           bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string oidStr ;
      string filePath ;
      OID oid ;
      ossFile file ;
      INT64 bufLen = LOB_BUFFER_LEN ;
      INT64 readSize = 0 ;
      CHAR *buf = NULL ;
      sdbLob lob ;
      rc = arg.getString( 0, filePath ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Filepath must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Filepath must be string" ) ;
         goto error ;
      }

      rc = arg.getString( 1, oidStr ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         //oid is generated from server side
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "OidStr must be string" ) ;
         goto error ;
      }
      else
      {
         if ( !utilIsValidOID( oidStr.c_str() ) )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << "Oid string invalid" ) ;
            goto error ;
         }
         oid = OID( oidStr ) ;
      }

      rc = file.open( filePath.c_str(), OSS_READONLY|OSS_SHAREREAD,
                      OSS_DEFAULTFILE ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to open file" ) ;
         goto error ;
      }

      if ( oid.isSet() )
      {
         rc = _cl.createLob( lob, &oid ) ;
      }
      else
      {
         rc = _cl.createLob( lob, NULL ) ;
         if ( SDB_OK == rc )
         {
            rc = lob.getOid( oid ) ;
         }
      }

      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to open lob" ) ;
         goto error ;
      }

      buf = static_cast< CHAR* >( SDB_OSS_MALLOC( bufLen ) ) ;
      if( NULL == buf )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Failed to mallc buffer" ) ;
         goto error ;
      }
      while( SDB_OK == ( rc = file.readN( buf, bufLen, readSize ) ) )
      {
         rc = lob.write( buf, static_cast< UINT32 >(readSize) ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Failed to write lob" ) ;
            goto error ;
         }
      }
      if( SDB_EOF == rc )
      {
         rc = SDB_OK ;
         rc = lob.close() ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Failed to close lob" ) ;
            goto error ;
         }
      }
      else
      {
         detail = BSON( SPT_ERR << "Failed to read local file" ) ;
         goto error ;
      }
      rval.getReturnVal().setValue( oid.toString() ) ;
   done:
      SAFE_OSS_FREE( buf ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCL::getLob( const _sptArguments &arg,
                           _sptReturnVal &rval,
                           bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string filePath ;
      string oidStr ;
      INT32 replace = FALSE ;
      INT32 mode = OSS_READWRITE|OSS_EXCLUSIVE ;
      ossFile file ;
      sdbLob lob ;
      INT64 lobSize = -1 ;
      INT64 readSize = 0 ;
      UINT64 createTime = 0 ;
      CHAR *buf = NULL ;
      BSONObjBuilder builder ;

      rc = arg.getString( 0, oidStr ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Oid must be cofig" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Oid must be string" ) ;
         goto error ;
      }
      if ( !utilIsValidOID( oidStr.c_str() ) )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Oid string invalid" ) ;
         goto error ;
      }

      rc = arg.getString( 1, filePath ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "FilePath must be cofig" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "FilePath must be string" ) ;
         goto error ;
      }
      rc = arg.getNative( 2, &replace, SPT_NATIVE_INT32 ) ;
      if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
      {
         detail = BSON( SPT_ERR << "Forced must be bool" ) ;
         goto error ;
      }
      if( replace )
      {
         mode |= OSS_REPLACE ;
      }
      else
      {
         mode |= OSS_CREATEONLY ;
      }
      rc = _cl.openLob( lob, OID( oidStr ), SDB_LOB_READ ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to open lob" ) ;
         goto error ;
      }
      rc = file.open( filePath, mode, OSS_DEFAULTFILE ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to open file" ) ;
         goto error ;
      }

      lobSize = lob.getSize() ;
      createTime = lob.getCreateTime() ;

      buf = static_cast< CHAR* >( SDB_OSS_MALLOC( LOB_BUFFER_LEN ) ) ;
      if( NULL == buf )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Failed to malloc buffer" ) ;
         goto error ;
      }
      while( readSize < lobSize )
      {
         UINT32 read = 0 ;
         rc = lob.read( LOB_BUFFER_LEN, buf, &read ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Failed to read lob" ) ;
            goto error ;
         }
         readSize += static_cast< INT64 >( read ) ;
         rc = file.writeN( buf, static_cast< INT64 >( read ) ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Failed to write local file" ) ;
            goto error ;
         }
      }
      rc = lob.close() ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to close lob" ) ;
         goto error ;
      }
      rc = file.close() ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to close local file" ) ;
         goto error ;
      }
      try
      {
         builder.append( "LobSize", lob.getSize() ) ;
         builder.appendTimestamp( "CreateTime", createTime,
                          (createTime - ( createTime / 1000 * 1000 ) ) * 1000) ;
      }
      catch( std::exception )
      {
         rc = SDB_SYS ;
         detail = BSON( SPT_ERR << "Failed to build return bson obj" ) ;
         goto error ;
      }
      rval.getReturnVal().setValue( builder.obj() ) ;
   done:
      SAFE_OSS_FREE( buf ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCL::deleteLob( const _sptArguments &arg,
                              _sptReturnVal &rval,
                              bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string oidStr ;
      OID oid ;
      // Get arg
      rc = arg.getString( 0,  oidStr ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Oid must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Oid must be string" ) ;
         goto error ;
      }
      if ( !utilIsValidOID( oidStr.c_str() ) )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Oid string invalid" ) ;
         goto error ;
      }

      // Call driver
      oid.init( oidStr ) ;
      rc = _cl.removeLob( oid ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to delete lob" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCL::truncateLob( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      INT64 length = 0 ;
      string oidStr ;
      OID oid ;
      rc = arg.getString( 0, oidStr ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Oid must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Oid must be string" ) ;
         goto error ;
      }
      rc = arg.getNative( 1, &length, SPT_NATIVE_INT64 ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Length must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Length must be number" ) ;
         goto error ;
      }
      if ( !utilIsValidOID( oidStr.c_str() ) )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Oid string invalid" ) ;
         goto error ;
      }

      oid.init( oidStr ) ;
      rc = _cl.truncateLob( oid, length ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCL::createLobID( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      OID oid ;
      string timestamp ;

      if ( arg.argc() > 1 )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Not more than 1 parameters" ) ;
         goto error ;
      }

      if( 0 == arg.argc() )
      {
         rc = _cl.createLobID( oid ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Failed to list createLobID" ) ;
            goto error ;
         }

         rval.getReturnVal().setValue( oid.toString() ) ;
         goto done ;
      }

      rc = arg.getString( 0,  timestamp ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Must be String" ) ;
         goto error ;
      }

      rc = _cl.createLobID( oid, timestamp.c_str() ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to create lobID" ) ;
         goto error ;
      }

      rval.getReturnVal().setValue( oid.toString() ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCL::listLobs( const _sptArguments &arg,
                             _sptReturnVal &rval,
                             bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      _sdbCursor *pCursor = NULL ;
      string objectName ;
      BSONObj obj ;
      BSONObj cond ;
      BSONObj sel ;
      BSONObj order ;
      BSONObj hint ;
      INT64 numToSkip = 0 ;
      INT64 numToRet = -1 ;

      if( arg.argc() == 0 )
      {
         rc = _cl.listLobs( &pCursor ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Failed to list lobs" ) ;
            goto error ;
         }
         SPT_SET_CURSOR_TO_RETURNVAL( pCursor ) ;
         goto done ;
      }

      if ( arg.argc() > 1 )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Not more than 1 parameters" ) ;
         goto error ;
      }

      if( arg.isNull(0) )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Parameter can't be NULL" ) ;
         goto error ;
      }

      objectName = arg.getUserObjClassName(0) ;
      if ( SPT_QUERYOPTION_NAME != objectName )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Parameter should be SdbQueryOption" ) ;
         goto error ;
      }

      rc = arg.getBsonobj( 0, obj ) ;
      if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
      {
         detail = BSON( SPT_ERR << ( arg.hasErrMsg() ? arg.getErrMsg() :
                                     "Parameter must be obj" ) ) ;
         goto error ;
      }

      if ( obj.hasField( SPT_OPTIONBASE_COND_FIELD ) )
      {
         cond = obj.getObjectField( SPT_OPTIONBASE_COND_FIELD ) ;
      }
      if ( obj.hasField( SPT_OPTIONBASE_SEL_FIELD ) )
      {
         sel = obj.getObjectField( SPT_OPTIONBASE_SEL_FIELD ) ;
      }
      if ( obj.hasField( SPT_OPTIONBASE_SORT_FIELD ) )
      {
         order = obj.getObjectField( SPT_OPTIONBASE_SORT_FIELD ) ;
      }
      if ( obj.hasField( SPT_OPTIONBASE_HINT_FIELD ) )
      {
         hint = obj.getObjectField( SPT_OPTIONBASE_HINT_FIELD ) ;
      }
      if ( obj.hasField( SPT_OPTIONBASE_SKIP_FIELD ) )
      {
         numToSkip = obj.getIntField( SPT_OPTIONBASE_SKIP_FIELD ) ;
      }
      if ( obj.hasField( SPT_OPTIONBASE_LIMIT_FIELD ) )
      {
         numToRet = obj.getIntField( SPT_OPTIONBASE_LIMIT_FIELD ) ;
      }

      rc = _cl.listLobs( &pCursor, cond, sel, order, hint, numToSkip,
                         numToRet ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to list lobs" ) ;
         goto error ;
      }

      SPT_SET_CURSOR_TO_RETURNVAL( pCursor ) ;

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pCursor ) ;
      goto done ;
   }

   INT32 _sptDBCL::listLobPieces( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      _sdbCursor *pCursor = NULL ;
      string objectName ;
      BSONObj obj ;
      BSONObj cond ;
      BSONObj sel ;
      BSONObj order ;
      BSONObj hint ;
      INT64 numToSkip = 0 ;
      INT64 numToRet = -1 ;

      if( arg.argc() == 0 )
      {
         rc = _cl.listLobPieces( &pCursor ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Failed to list lob pieces" ) ;
            goto error ;
         }
         SPT_SET_CURSOR_TO_RETURNVAL( pCursor ) ;
         goto done ;
      }

      if ( arg.argc() > 1 )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Not more than 1 parameters" ) ;
         goto error ;
      }

      if( arg.isNull(0) )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Parameter can't be NULL" ) ;
         goto error ;
      }

      objectName = arg.getUserObjClassName(0) ;
      if ( SPT_QUERYOPTION_NAME != objectName )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Parameter should be SdbQueryOption" ) ;
         goto error ;
      }

      rc = arg.getBsonobj( 0, obj ) ;
      if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
      {
         detail = BSON( SPT_ERR << ( arg.hasErrMsg() ? arg.getErrMsg() :
                                     "Parameter must be obj" ) ) ;
         goto error ;
      }

      if ( obj.hasField( SPT_OPTIONBASE_COND_FIELD ) )
      {
         cond = obj.getObjectField( SPT_OPTIONBASE_COND_FIELD ) ;
      }
      if ( obj.hasField( SPT_OPTIONBASE_SEL_FIELD ) )
      {
         sel = obj.getObjectField( SPT_OPTIONBASE_SEL_FIELD ) ;
      }
      if ( obj.hasField( SPT_OPTIONBASE_SORT_FIELD ) )
      {
         order = obj.getObjectField( SPT_OPTIONBASE_SORT_FIELD ) ;
      }
      if ( obj.hasField( SPT_OPTIONBASE_HINT_FIELD ) )
      {
         hint = obj.getObjectField( SPT_OPTIONBASE_HINT_FIELD ) ;
      }
      if ( obj.hasField( SPT_OPTIONBASE_SKIP_FIELD ) )
      {
         numToSkip = obj.getIntField( SPT_OPTIONBASE_SKIP_FIELD ) ;
      }
      if ( obj.hasField( SPT_OPTIONBASE_LIMIT_FIELD ) )
      {
         numToRet = obj.getIntField( SPT_OPTIONBASE_LIMIT_FIELD ) ;
      }

      rc = _cl.listLobPieces( &pCursor, cond, sel, order, hint, numToSkip,
                              numToRet ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to list lob pieces" ) ;
         goto error ;
      }

      SPT_SET_CURSOR_TO_RETURNVAL( pCursor ) ;

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pCursor ) ;
      goto done ;
   }

   INT32 _sptDBCL::truncate( const _sptArguments &arg,
                             _sptReturnVal &rval,
                             bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      rc = _cl.truncate() ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to truncate collection" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCL::createIdIndex( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj options ;

      if( !arg.isNull( 0 ) )
      {
         rc = arg.getBsonobj( 0, options ) ;
         if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
         {
            detail = BSON( SPT_ERR << "Options must be obj" ) ;
            goto error ;
         }
      }
      rc = _cl.createIdIndex( options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to create ID idnex" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCL::dropIdIndex( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;

      rc = _cl.dropIdIndex() ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to drop ID index" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCL::createAutoIncrement( const _sptArguments &arg,
                                        _sptReturnVal &rval,
                                        bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;

      if( arg.argc() > 1 )
      {
         detail = BSON( SPT_ERR << "Contain unknow parameters" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if( !arg.isNull( 0 ) )
      {
         if( arg.isArray( 0 ) )
         {
            vector<bson::BSONObj> objArr ;
            rc = arg.getArray( 0, objArr ) ;
            if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
            {
               detail = BSON( SPT_ERR << "Options must be obj" ) ;
               goto error ;
            }
            rc = _cl.createAutoIncrement( objArr ) ;
         }
         else if( arg.isObject( 0 ) )
         {
            BSONObj obj ;
            rc = arg.getBsonobj( 0, obj ) ;
            if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
            {
               detail = BSON( SPT_ERR << "Options must be obj" ) ;
               goto error ;
            }
            rc = _cl.createAutoIncrement( obj ) ;
         }
         else
         {
            detail = BSON( SPT_ERR << "Fields must be obj or array of obj" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      else
      {
         detail = BSON( SPT_ERR << "Invalid parameters" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to create autoincrement field" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCL::dropAutoIncrement( const _sptArguments &arg,
                                      _sptReturnVal &rval,
                                      bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string fieldName ;
      BSONObj options ;

      if( arg.argc() > 1 )
      {
         detail = BSON( SPT_ERR << "Contain unknow parameters" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if( !arg.isNull( 0 ) )
      {
         if( arg.isString( 0 ) )
         {
            rc = arg.getString( 0, fieldName ) ;
            if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
            {
               detail = BSON( SPT_ERR << "Component must be string" ) ;
               goto error ;
            }
            rc = _cl.dropAutoIncrement( fieldName.c_str() ) ;
            if( SDB_OK != rc )
            {
               detail = BSON( SPT_ERR << "Failed to drop autoincrement field" ) ;
               goto error ;
            }
         }
         else if( arg.isArray( 0 ) )
         {
            vector< const CHAR * > fieldNames ;
            vector< string > vecStr ;
            rc = arg.getArray( 0, vecStr ) ;
            if( SDB_OUT_OF_BOUND == rc )
            {
               detail = BSON( SPT_ERR << "Docs array must be config" ) ;
               goto error ;
            }
            for ( UINT32 i = 0 ; i < vecStr.size() ; ++i )
            {
               fieldNames.push_back( vecStr[i].c_str() ) ;
            }
            rc = _cl.dropAutoIncrement( fieldNames ) ;
            if( SDB_OK != rc )
            {
               detail = BSON( SPT_ERR << "Failed to drop autoincrement field" ) ;
               goto error ;
            }
         }
         else
         {
            detail = BSON( SPT_ERR << "Fields must be string or array of string" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      else
      {
         detail = BSON( SPT_ERR << "Invalid parameters" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }


   INT32 _sptDBCL::getQueryMeta( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj cond ;
      BSONObj sort ;
      BSONObj hint ;
      INT32 skip = 0 ;
      INT32 limit = -1 ;
      _sdbCursor *pCursor = NULL ;

      rc = arg.getBsonobj( 0, cond ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Condition must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Condition must be obj" ) ;
         goto error ;
      }
      rc = arg.getBsonobj( 1, sort ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Sort must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Sort must be obj" ) ;
         goto error ;
      }
      rc = arg.getBsonobj( 2, hint ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Hint must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Hint must be obj" ) ;
         goto error ;
      }
      rc = arg.getNative( 3, &skip, SPT_NATIVE_INT32 ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Skip must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Skip must be int" ) ;
         goto error ;
      }
      rc = arg.getNative( 4, &limit, SPT_NATIVE_INT32 ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Limit must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Limit must be int" ) ;
         goto error ;
      }
      rc = _cl.getQueryMeta( &pCursor, cond, sort, hint, skip, limit ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get query mete" ) ;
         goto error ;
      }
      SPT_SET_CURSOR_TO_RETURNVAL( pCursor ) ;
   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pCursor ) ;
      goto done ;
   }

   INT32 _sptDBCL::enableSharding( const _sptArguments &arg,
                                   _sptReturnVal &rval,
                                   bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj options ;
      if( arg.argc() > 0 )
      {
         rc = arg.getBsonobj( 0, options ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Options must be config" ) ;
            goto error ;
         }
      }
      rc = _cl.enableSharding( options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to enable sharding" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCL::disableSharding( const _sptArguments &arg,
                                    _sptReturnVal &rval,
                                    bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      if( arg.argc() != 0 )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Wrong arguments" ) ;
         goto error ;
      }
      rc = _cl.disableSharding() ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to disable sharding" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCL::enableCompression( const _sptArguments &arg,
                                      _sptReturnVal &rval,
                                      bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj options ;
      if( arg.argc() > 0 )
      {
         rc = arg.getBsonobj( 0, options ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Options must be config" ) ;
            goto error ;
         }
      }
      rc = _cl.enableCompression( options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to enable compression" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCL::disableCompression( const _sptArguments &arg,
                                       _sptReturnVal &rval,
                                       bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      if( arg.argc() != 0 )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Wrong arguments" ) ;
         goto error ;
      }
      rc = _cl.disableCompression() ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to disable compression" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCL::setAttributes( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj options ;
      if( arg.argc() > 0 )
      {
         rc = arg.getBsonobj( 0, options ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Options must be config" ) ;
            goto error ;
         }
      }
      rc = _cl.setAttributes( options ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to set attributes" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCL::getDetail( const _sptArguments &arg,
                              _sptReturnVal &rval,
                              bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      _sdbCursor *pCursor = NULL ;
      rc = _cl.getDetail( &pCursor ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get detail" ) ;
         goto error ;
      }
      SPT_SET_CURSOR_TO_RETURNVAL( pCursor ) ;
   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pCursor ) ;
      goto done ;
   }

   INT32 _sptDBCL::query( const BSONObj &cond, const BSONObj &sel,
                          const BSONObj &order, const BSONObj &hint,
                          const BSONObj &options, INT32 numToSkip,
                          INT32 numToRet, INT32 flags, _sdbCursor **curosr )
   {
      INT32 rc = SDB_OK ;
      if( Object == hint.getField( FIELD_NAME_MODIFY ).type() )
      {
         flags |= FLG_QUERY_MODIFY ;
      }

      if( options.hasField( FIELD_NAME_KEEP_SHARDING_KEY ) )
      {
         BSONElement ele = options.getField( FIELD_NAME_KEEP_SHARDING_KEY ) ;
         if( Bool != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if( TRUE == ele.Bool() )
         {
            flags |= QUERY_KEEP_SHARDINGKEY_IN_UPDATE ;
         }
      }
      rc = _cl.query( curosr, cond, sel, order, hint, numToSkip, numToRet,
                      flags ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCL::getCount( SINT64 &count, const BSONObj &condition,
                             const BSONObj &hint )
   {
      INT32 rc = SDB_OK ;
      rc = _cl.getCount( count, condition, hint ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCL::cvtToBSON( const CHAR* key, const sptObject &value,
                              BOOLEAN isSpecialObj, BSONObjBuilder& builder,
                              string &errMsg )
   {
      errMsg = "SdbCL can not be converted to bson" ;
      return SDB_INVALIDARG ;
   }

   INT32 _sptDBCL::fmpToBSON( const sptObject &value, BSONObj &retObj,
                              string &errMsg )
   {
      INT32 rc = SDB_OK ;
      string clName ;
      string csName ;
      sptObjectPtr csPtr ;

      rc = value.getStringField( SPT_CL_NAME_FIELD, clName ) ;
      if( SDB_OK != rc )
      {
         errMsg = "Failed to get cl name field" ;
         goto error ;
      }
      rc = value.getObjectField( SPT_CL_CS_FIELD, csPtr ) ;
      if( SDB_OK != rc )
      {
         errMsg = "Failed to get cs" ;
         goto error ;
      }
      rc = csPtr->getStringField( SPT_CS_NAME_FIELD, csName ) ;
      if( SDB_OK != rc )
      {
         errMsg = "Failed to get cs name field" ;
         goto error ;
      }
      retObj = BSON( SPT_CL_NAME_FIELD << csName + "." + clName ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBCL::bsonToJSObj( sdbclient::sdb &db, const BSONObj &data,
                                _sptReturnVal &rval, bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string clFullName ;
      string clName ;
      string csName ;
      size_t pos = 0 ;
      _sdbCollectionSpace *pCS = NULL ;
      sptDBCS* sptCS = NULL ;
      _sdbCollection *pCL = NULL ;
      sptDBCL *sptCL = NULL ;

      sptProperty *pTmpProp = NULL ;

      clFullName = data.getStringField( SPT_CL_NAME_FIELD ) ;
      pos = clFullName.find( "." ) ;
      if( pos == std::string::npos ||
          pos >= clFullName.size() - 1 )
      {
         rc = SDB_SYS ;
         detail = BSON( SPT_ERR << "Invalid fullname" ) ;
         goto error ;
      }
      csName = clFullName.substr( 0, pos ) ;
      clName = clFullName.substr( pos + 1 ) ;

      rc = db.getCollectionSpace( csName.c_str(), &pCS ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get cs" ) ;
         goto error ;
      }

      rc = pCS->getCollection( clName.c_str(), &pCL ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to get cl" ) ;
         goto error ;
      }

      sptCS = SDB_OSS_NEW sptDBCS( pCS ) ;
      if( NULL == sptCS )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Failed to new sptDBCS obj" ) ;
         goto error ;
      }
      pCS = NULL ;

      sptCL = SDB_OSS_NEW sptDBCL( pCL ) ;
      if( NULL == sptCL )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Failed to new sptDBCL obj" ) ;
         goto error ;
      }
      pCL = NULL ;

      rc = rval.setUsrObjectVal< sptDBCL >( sptCL ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to set return obj" ) ;
         goto error ;
      }
      sptCL = NULL ;

      //rval.getReturnVal().setName( clName ) ;
      //rval.getReturnVal().setAttr( SPT_PROP_READONLY ) ;
      rval.addReturnValProperty( SPT_CL_NAME_FIELD )->setValue( clName ) ;

      pTmpProp = rval.addReturnValProperty( SPT_CL_CS_FIELD ) ;
      if ( !pTmpProp )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Failed to alloc memory" ) ;
         goto error ;
      }
      rc = pTmpProp->assignUsrObject< sptDBCS >( sptCS ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << "Failed to set return obj property" ) ;
         goto error ;
      }
      sptCS = NULL ;

      pTmpProp->addBackwardProp( SPT_CS_CONN_FIELD ) ;
      pTmpProp = pTmpProp->addSubProp( SPT_CS_NAME_FIELD ) ;
      if ( !pTmpProp )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Failed to alloc memory" ) ;
         goto error ;
      }
      pTmpProp->setValue( csName ) ;

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pCL ) ;
      SAFE_OSS_DELETE( pCS ) ;
      SAFE_OSS_DELETE( sptCL ) ;
      SAFE_OSS_DELETE( sptCS ) ;
      goto done ;
   }
}
