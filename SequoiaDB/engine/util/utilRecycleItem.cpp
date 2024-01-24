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

   Source File Name = utilRecycleItem.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for recycle item.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/03/2021  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "utilRecycleItem.hpp"
#include "utilUniqueID.hpp"
#include "pdTrace.hpp"
#include "utilTrace.hpp"
#include "../bson/bson.hpp"

using namespace bson ;
using namespace std ;

namespace engine
{

   const CHAR *utilGetRecycleTypeName( UTIL_RECYCLE_TYPE type )
   {
      switch ( type )
      {
         case UTIL_RECYCLE_CS :
         {
            return UTIL_RECYCLE_CS_NAME ;
         }
         case UTIL_RECYCLE_CL :
         {
            return UTIL_RECYCLE_CL_NAME ;
         }
         case UTIL_RECYCLE_SEQ :
         {
            return UTIL_RECYCLE_SEQ_NAME ;
         }
         case UTIL_RECYCLE_IDX :
         {
            return UTIL_RECYCLE_IDX_NAME ;
         }
         default :
         {
            break ;
         }
      }
      return "Unknown" ;
   }

   UTIL_RECYCLE_TYPE utilGetRecycleType( const CHAR *typeName )
   {
      if ( 0 == ossStrcmp( typeName, UTIL_RECYCLE_CS_NAME ) )
      {
         return UTIL_RECYCLE_CS ;
      }
      else if ( 0 == ossStrcmp( typeName, UTIL_RECYCLE_CL_NAME ) )
      {
         return UTIL_RECYCLE_CL ;
      }
      else if ( 0 == ossStrcmp( typeName, UTIL_RECYCLE_SEQ_NAME ) )
      {
         return UTIL_RECYCLE_SEQ ;
      }
      else if ( 0 == ossStrcmp( typeName, UTIL_RECYCLE_IDX_NAME ) )
      {
         return UTIL_RECYCLE_IDX ;
      }
      return UTIL_RECYCLE_UNKNOWN ;
   }

   const CHAR *utilGetRecycleOpTypeName( UTIL_RECYCLE_OPTYPE type )
   {
      switch ( type )
      {
         case UTIL_RECYCLE_OP_DROP :
         {
            return UTIL_RECYCLE_OP_DROP_NAME ;
         }
         case UTIL_RECYCLE_OP_TRUNCATE :
         {
            return UTIL_RECYCLE_OP_TRUNCATE_NAME ;
         }
         default :
         {
            break ;
         }
      }
      return "Unknown" ;
   }

   UTIL_RECYCLE_OPTYPE utilGetRecycleOpType( const CHAR *typeName )
   {
      if ( 0 == ossStrcmp( typeName, UTIL_RECYCLE_OP_DROP_NAME ) )
      {
         return UTIL_RECYCLE_OP_DROP ;
      }
      else if ( 0 == ossStrcmp( typeName, UTIL_RECYCLE_OP_TRUNCATE_NAME ) )
      {
         return UTIL_RECYCLE_OP_TRUNCATE ;
      }
      return UTIL_RECYCLE_OP_UNKNOWN ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILGETRECYCLSINCSBOUNDS, "utilGetRecyCLsInCSBounds")
   INT32 utilGetRecyCLsInCSBounds( const CHAR *fieldName,
                                   utilCSUniqueID csUniqueID,
                                   BSONObj &matcher )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILGETRECYCLSINCSBOUNDS ) ;

      try
      {
         BSONObjBuilder builder ;

         rc = utilGetCSBounds( fieldName, csUniqueID, builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get bounds with collection "
                      "space unique ID [%u], rc: %d", csUniqueID, rc ) ;

         builder.append( FIELD_NAME_TYPE,
                         utilGetRecycleTypeName( UTIL_RECYCLE_CL ) ) ;

         matcher = builder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_UTILGETRECYCLSINCSBOUNDS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _utilRecycleItem implement
    */
   _utilRecycleItem::_utilRecycleItem()
   : _recycleID( UTIL_RECYCLEID_NULL ),
     _originID( UTIL_UNIQUEID_NULL ),
     _type( UTIL_RECYCLE_UNKNOWN ),
     _opType( UTIL_RECYCLE_OP_UNKNOWN ),
     _recycleTime( 0 ),
     _isMainCL( FALSE ),
     _isCSRecycled( FALSE )
   {
      _recycleName[ 0 ] = '\0' ;
      _originName[ 0 ] = '\0' ;
   }

   _utilRecycleItem::_utilRecycleItem( const _utilRecycleItem &item )
   : _recycleID( item._recycleID ),
     _originID( item._originID ),
     _type( item._type ),
     _opType( item._opType ),
     _recycleTime( item._recycleTime ),
     _isMainCL( item._isMainCL ),
     _isCSRecycled( item._isCSRecycled )
   {
      setRecycleName( item._recycleName ) ;
      setOriginName( item._originName ) ;
   }

   _utilRecycleItem::_utilRecycleItem( UTIL_RECYCLE_TYPE type,
                                       UTIL_RECYCLE_OPTYPE opType )
   : _recycleID( UTIL_RECYCLEID_NULL ),
     _originID( UTIL_UNIQUEID_NULL ),
     _type( type ),
     _opType( opType ),
     _recycleTime( 0 ),
     _isMainCL( FALSE ),
     _isCSRecycled( FALSE )
   {
      _recycleName[ 0 ] = '\0' ;
      _originName[ 0 ] = '\0' ;
   }

   _utilRecycleItem::~_utilRecycleItem()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYCLEITEM_INHERIT, "_utilRecycleItem::inherit")
   void _utilRecycleItem::inherit( const _utilRecycleItem &item,
                                   const CHAR *originName,
                                   utilCLUniqueID originID )
   {
      PD_TRACE_ENTRY( SDB_UTILRECYCLEITEM_INHERIT ) ;

      // inherit type
      _type = item._type ;
      _opType = item._opType ;

      // set with different origin ID and name
      setOriginID( originID ) ;
      setOriginName( originName ) ;

      // inherit recycle ID
      init( item._recycleID ) ;

      PD_TRACE_EXIT( SDB_UTILRECYCLEITEM_INHERIT ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYCLEITEM_INIT_RECYID, "_utilRecycleItem::init")
   void _utilRecycleItem::init( utilRecycleID recycleID )
   {
      PD_TRACE_ENTRY( SDB_UTILRECYCLEITEM_INIT_RECYID ) ;

      SDB_ASSERT( NULL != _originName, "origin name is invalid" ) ;
      SDB_ASSERT( UTIL_RECYCLEID_NULL != recycleID, "recycle ID is invalid" ) ;
      SDB_ASSERT( UTIL_UNIQUEID_NULL != _originID, "origin ID is invalid" ) ;

      setRecycleID( recycleID ) ;
      _setRecycleName( recycleID, _originID ) ;
      setRecycleTime( ossGetCurrentMilliseconds() ) ;

      PD_TRACE_EXIT( SDB_UTILRECYCLEITEM_INIT_RECYID ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYCLEITEM_RESET, "_utilRecycleItem::reset")
   void _utilRecycleItem::reset()
   {
      PD_TRACE_ENTRY( SDB_UTILRECYCLEITEM_RESET ) ;

      _recycleID = UTIL_RECYCLEID_NULL ;
      _recycleName[ 0 ] = '\0' ;
      _originID = UTIL_UNIQUEID_NULL ;
      _originName[ 0 ] = '\0' ;
      _type = UTIL_RECYCLE_UNKNOWN ;
      _opType = UTIL_RECYCLE_OP_UNKNOWN ;
      _recycleTime = 0 ;
      _isMainCL = FALSE ;
      _isCSRecycled = FALSE ;

      PD_TRACE_EXIT( SDB_UTILRECYCLEITEM_RESET ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYCLEITEM_TOBSON, "_utilRecycleItem::toBSON")
   INT32 _utilRecycleItem::toBSON( BSONObj &object ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRECYCLEITEM_TOBSON ) ;

      try
      {
         BSONObjBuilder builder ;

         rc = toBSON( builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON fro recycle item, "
                      "rc: %d", rc ) ;

         object = builder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build BSON for recycle item, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_UTILRECYCLEITEM_TOBSON, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYCLEITEM_TOBSON_BUILDER, "_utilRecycleItem::toBSON")
   INT32 _utilRecycleItem::toBSON( BSONObjBuilder &builder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRECYCLEITEM_TOBSON_BUILDER ) ;

      try
      {
         CHAR timeStamp[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
         ossMillisecondsToString( _recycleTime, timeStamp ) ;
         builder.append( FIELD_NAME_RECYCLE_NAME, _recycleName ) ;
         builder.append( FIELD_NAME_RECYCLE_ID, (INT64)_recycleID ) ;
         builder.append( FIELD_NAME_ORIGIN_NAME, _originName ) ;
         builder.append( FIELD_NAME_ORIGIN_ID, (INT64)_originID ) ;
         builder.append( FIELD_NAME_TYPE, utilGetRecycleTypeName( _type ) ) ;
         builder.append( FIELD_NAME_OPTYPE,
                         utilGetRecycleOpTypeName( _opType ) ) ;
         builder.append( FIELD_NAME_RECYCLE_TIME, timeStamp ) ;
         if ( _isMainCL )
         {
            builder.appendBool( FIELD_NAME_ISMAINCL, TRUE ) ;
         }
         if ( _isCSRecycled )
         {
            builder.appendBool( FIELD_NAME_RECYCLE_ISCSRECY, _isCSRecycled ) ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build BSON for recycle item, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_UTILRECYCLEITEM_TOBSON_BUILDER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYCLEITEM_TOBSON_FIELD, "_utilRecycleItem::toBSON")
   INT32 _utilRecycleItem::toBSON( BSONObjBuilder &builder,
                                   const CHAR *fieldName ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRECYCLEITEM_TOBSON_FIELD ) ;

      try
      {
         BSONObjBuilder subBuilder( builder.subobjStart( fieldName ) ) ;

         rc = utilRecycleItem::toBSON( subBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for recycle item, "
                      "rc: %d", rc ) ;

         subBuilder.done() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build BSON for recycle item, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_UTILRECYCLEITEM_TOBSON_FIELD, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYCLEITEM_FROMBSON, "_utilRecycleItem::fromBSON")
   INT32 _utilRecycleItem::fromBSON( const BSONObj &object )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRECYCLEITEM_FROMBSON ) ;

      try
      {
         BSONElement element ;

         // recycle name
         element = object.getField( FIELD_NAME_RECYCLE_NAME ) ;
         PD_CHECK( String == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not string",
                   FIELD_NAME_RECYCLE_NAME ) ;
         setRecycleName( element.valuestr() ) ;

         // recycle ID
         element = object.getField( FIELD_NAME_RECYCLE_ID ) ;
         PD_CHECK( NumberLong == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not long number",
                   FIELD_NAME_RECYCLE_ID ) ;
         _recycleID = (UINT64)( element.numberLong() ) ;

         // origin name
         element = object.getField( FIELD_NAME_ORIGIN_NAME ) ;
         PD_CHECK( String == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not string",
                   FIELD_NAME_ORIGIN_NAME ) ;
         setOriginName( element.valuestr() ) ;

         // origin ID
         element = object.getField( FIELD_NAME_ORIGIN_ID ) ;
         PD_CHECK( NumberLong == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not long number",
                   FIELD_NAME_ORIGIN_ID ) ;
         _originID = (UINT64)( element.numberLong() ) ;

         // type
         element = object.getField( FIELD_NAME_TYPE ) ;
         PD_CHECK( String == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not string",
                   FIELD_NAME_TYPE ) ;
         _type = utilGetRecycleType( element.valuestr() ) ;

         // operation type
         element = object.getField( FIELD_NAME_OPTYPE ) ;
         PD_CHECK( String == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not string",
                   FIELD_NAME_OPTYPE ) ;
         _opType = utilGetRecycleOpType( element.valuestr() ) ;

         // recycle time
         element = object.getField( FIELD_NAME_RECYCLE_TIME ) ;
         if ( String == element.type() )
         {
            _recycleTime = ossStringToMilliseconds( element.valuestr() ) ;
         }
         else if ( Timestamp == element.type() )
         {
            _recycleTime = (UINT64)( element.timestampTime() ) ;
         }
         else
         {
            PD_LOG( PDERROR, "Failed to get field [%s], it is not string or "
                    "long number", FIELD_NAME_RECYCLE_TIME ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         // is main collection
         element = object.getField( FIELD_NAME_ISMAINCL ) ;
         if ( EOO != element.type() )
         {
            PD_CHECK( Bool == element.type(), SDB_SYS, error, PDERROR,
                      "Failed to get field [%s], it is not boolean",
                      FIELD_NAME_ISMAINCL ) ;
            _isMainCL = element.boolean() ;
         }
         else
         {
            _isMainCL = FALSE ;
         }

         // is valid
         element = object.getField( FIELD_NAME_RECYCLE_ISCSRECY ) ;
         if ( EOO != element.type() )
         {
            PD_CHECK( Bool == element.type(), SDB_SYS, error, PDERROR,
                      "Failed to get field [%s], it is not boolean",
                      FIELD_NAME_RECYCLE_ISCSRECY ) ;
            _isCSRecycled = element.boolean() ;
         }
         else
         {
            _isCSRecycled = FALSE ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to get recycle item from BSON, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_UTILRECYCLEITEM_FROMBSON, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYCLEITEM_FROMBSON_FIELD, "_utilRecycleItem::fromBSON")
   INT32 _utilRecycleItem::fromBSON( const BSONObj &object,
                                     const CHAR *fieldName )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRECYCLEITEM_FROMBSON_FIELD ) ;

      try
      {
         BSONElement element = object.getField( fieldName ) ;
         BSONObj recycleObject ;

         PD_CHECK( Object == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not an object",
                   fieldName ) ;
         recycleObject = element.embeddedObject() ;

         rc = fromBSON( recycleObject ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get recycle item from "
                      "BSON, rc: %d", rc ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to get recycle item from BSON, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_UTILRECYCLEITEM_FROMBSON_FIELD, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYCLEITEM_FROMRECYCLENAME, "_utilRecycleItem::fromRecycleName")
   INT32 _utilRecycleItem::fromRecycleName( const CHAR *recycleName )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRECYCLEITEM_FROMRECYCLENAME ) ;

      SDB_ASSERT( NULL != recycleName, "recycle name is invalid" ) ;

      utilCLUniqueID originID = UTIL_UNIQUEID_NULL ;
      utilRecycleID recycleID = UTIL_RECYCLEID_NULL ;
      INT32 parsedNum = 0 ;

      PD_CHECK( NULL != recycleName, SDB_SYS, error, PDERROR,
                "Failed to parse from recycle name, it is invalid" ) ;

      parsedNum = ossSscanf( recycleName,
                             UTIL_RECYCLE_FORMAT,
                             &recycleID,
                             &originID ) ;
      PD_CHECK( parsedNum == 2, SDB_INVALIDARG, error, PDERROR,
                "Failed to parse from recycle name [%s], it is invalid",
                recycleName ) ;

      setRecycleName( recycleName ) ;
      setRecycleID( recycleID ) ;
      setOriginID( originID ) ;

   done:
      PD_TRACE_EXITRC( SDB_UTILRECYCLEITEM_FROMRECYCLENAME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   _utilRecycleItem &_utilRecycleItem::operator =( const _utilRecycleItem &item )
   {
      _recycleID = item._recycleID ;
      _originID = item._originID ;
      _type = item._type ;
      _opType = item._opType ;
      _recycleTime = item._recycleTime ;
      ossStrncpy( _recycleName, item._recycleName, UTIL_RECYCLE_NAME_SZ ) ;
      _recycleName[ UTIL_RECYCLE_NAME_SZ ] = '\0' ;
      ossStrncpy( _originName, item._originName, UTIL_ORIGIN_NAME_SZ ) ;
      _originName[ UTIL_ORIGIN_NAME_SZ ] = '\0' ;
      _isMainCL = item._isMainCL ;
      _isCSRecycled = item._isCSRecycled ;

      return ( *this ) ;
   }

   BOOLEAN _utilRecycleItem::operator ==( const _utilRecycleItem &item ) const
   {
      return ( _recycleID == item._recycleID &&
               _originID == item._originID &&
               _type == item._type &&
               _opType == item._opType &&
               0 == ossStrncmp( _recycleName,
                                item._recycleName,
                                UTIL_RECYCLE_NAME_SZ ) &&
               0 == ossStrncmp( _originName,
                                item._originName,
                                UTIL_ORIGIN_NAME_SZ ) &&
               _isMainCL == item._isMainCL &&
               _isCSRecycled == item._isCSRecycled ) ;
   }

   void _utilRecycleItem::_setRecycleName( utilRecycleID recycleID,
                                           utilCLUniqueID originID )
   {
      ossSnprintf( _recycleName, UTIL_RECYCLE_NAME_SZ, UTIL_RECYCLE_FORMAT,
                   recycleID, originID ) ;
   }

}
