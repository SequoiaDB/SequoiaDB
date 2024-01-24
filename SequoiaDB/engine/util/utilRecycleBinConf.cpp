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

   Source File Name = utilRecycleBinConf.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/03/2021  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "utilRecycleBinConf.hpp"
#include "msgDef.hpp"
#include "pdTrace.hpp"
#include "utilTrace.hpp"
#include "../bson/bson.hpp"

using namespace bson ;
using namespace std ;

namespace engine
{

   /*
      _utilRecycleBinConf implement
    */
   _utilRecycleBinConf::_utilRecycleBinConf()
   : _enable( UTIL_RECYCLEBIN_DFT_ENABLE ),
     _expireTime( UTIL_RECYCLEBIN_DFT_EXPIRETIME ),
     _maxItemNum( UTIL_RECYCLEBIN_DFT_MAXITEMNUM ),
     _maxVersionNum( UTIL_RECYCLEBIN_DFT_MAXVERNUM ),
     _autoDrop( UTIL_RECYCLEBIN_DFT_AUTODROP )
   {
   }

   _utilRecycleBinConf::~_utilRecycleBinConf()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYBINCONF_RESET, "_utilRecycleBinConf::reset")
   void _utilRecycleBinConf::reset()
   {
      PD_TRACE_ENTRY( SDB_UTILRECYBINCONF_RESET ) ;

      _enable = UTIL_RECYCLEBIN_DFT_ENABLE ;
      _expireTime = UTIL_RECYCLEBIN_DFT_EXPIRETIME ;
      _maxItemNum = UTIL_RECYCLEBIN_DFT_MAXITEMNUM ;
      _maxVersionNum = UTIL_RECYCLEBIN_DFT_MAXVERNUM ;
      _autoDrop = UTIL_RECYCLEBIN_DFT_AUTODROP ;

      PD_TRACE_EXIT( SDB_UTILRECYBINCONF_RESET ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYBINCONF_FROMBSON, "_utilRecycleBinConf::fromBSON")
   INT32 _utilRecycleBinConf::fromBSON( const BSONObj &object )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRECYBINCONF_FROMBSON ) ;

      reset() ;

      try
      {
         BSONElement element ;
         BSONObj subObject ;

         // get RecycleBin field
         element = object.getField( FIELD_NAME_RECYCLEBIN ) ;
         if ( EOO == element.type() )
         {
            PD_LOG( PDDEBUG, "No recycle object is found in DC object" ) ;
            goto done ;
         }
         PD_CHECK( Object == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to update from recycle object, "
                   "field [%s] is not an object", FIELD_NAME_RECYCLEBIN ) ;
         subObject = element.embeddedObject() ;

         // get Enable field
         element = subObject.getField( FIELD_NAME_ENABLE ) ;
         PD_CHECK( Bool == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not a bool type",
                   FIELD_NAME_ENABLE ) ;
         _enable = element.Bool() ;

         // get expire time
         element = subObject.getField( FIELD_NAME_EXPIRETIME ) ;
         PD_CHECK( NumberInt == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not a integer type",
                   FIELD_NAME_EXPIRETIME ) ;
         _expireTime = element.numberInt() ;

         // get MaxItemNum field
         element = subObject.getField( FIELD_NAME_MAXITEMNUM ) ;
         PD_CHECK( NumberInt == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not a integer type",
                   FIELD_NAME_MAXITEMNUM ) ;
         _maxItemNum = element.numberInt() ;

         // get MaxVersionNum field
         element = subObject.getField( FIELD_NAME_MAXVERNUM ) ;
         PD_CHECK( NumberInt == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not a integer type",
                   FIELD_NAME_MAXVERNUM ) ;
         _maxVersionNum = element.numberInt() ;

         // get auto drop field
         element = subObject.getField( FIELD_NAME_AUTODROP ) ;
         PD_CHECK( Bool == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not a boolean",
                   FIELD_NAME_AUTODROP ) ;
         _autoDrop = element.Bool() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to update recycle bin conf from BSON, "
                 "occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_UTILRECYBINCONF_FROMBSON, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYBINCONF_UPDATEOPTS, "_utilRecycleBinConf::updateOptions")
   INT32 _utilRecycleBinConf::updateOptions( const BSONObj &object )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRECYBINCONF_UPDATEOPTS ) ;

      try
      {
         BSONObjIterator iter( object ) ;

         while ( iter.more() )
         {
            BSONElement e = iter.next() ;
            const CHAR *fieldName = e.fieldName() ;
            if ( 0 ==  ossStrcmp( fieldName, FIELD_NAME_ENABLE ) )
            {
               PD_LOG_MSG_CHECK( Bool == e.type(),
                                 SDB_INVALIDARG, error, PDERROR,
                                 "Type of field [%s] is not boolean",
                                 fieldName ) ;
               setEnable( e.boolean() ) ;
            }
            else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_EXPIRETIME ) )
            {
               INT64 tmpTime = 0 ;
               PD_LOG_MSG_CHECK( e.isNumber(),
                                 SDB_INVALIDARG, error, PDERROR,
                                 "Type of field [%s] is not number",
                                 fieldName ) ;
               tmpTime = e.numberLong() ;
               PD_LOG_MSG_CHECK( tmpTime >= -1 &&
                                 tmpTime <= OSS_SINT32_MAX_LL,
                                 SDB_INVALIDARG, error, PDERROR,
                                 "Value [%lld] of field [%s] is out of range "
                                 "[%d - %d]", tmpTime, fieldName, -1,
                                 OSS_SINT32_MAX ) ;

               setExpireTime( (INT32)tmpTime ) ;
            }
            else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_MAXITEMNUM ) )
            {
               INT64 tmpNum = 0 ;

               PD_LOG_MSG_CHECK( e.isNumber(),
                                 SDB_INVALIDARG, error, PDERROR,
                                 "Type of field [%s] is not number",
                                 fieldName ) ;

               tmpNum = e.numberLong() ;
               PD_LOG_MSG_CHECK( tmpNum >= -1 &&
                                 tmpNum <= OSS_SINT32_MAX_LL,
                                 SDB_INVALIDARG, error, PDERROR,
                                 "Value [%lld] of field [%s] is out of range "
                                 "[%d - %d]", tmpNum, fieldName, -1,
                                 OSS_SINT32_MAX ) ;

               setMaxItemNum( (INT32)tmpNum ) ;
            }
            else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_MAXVERNUM ) )
            {
               INT64 tmpNum = 0 ;

               PD_LOG_MSG_CHECK( e.isNumber(),
                                 SDB_INVALIDARG, error, PDERROR,
                                 "Type of field [%s] is not number",
                                 fieldName ) ;

               tmpNum = e.numberLong() ;
               PD_LOG_MSG_CHECK( tmpNum >= -1 &&
                                 tmpNum <= OSS_SINT32_MAX_LL,
                                 SDB_INVALIDARG, error, PDERROR,
                                 "Value [%lld] of field [%s] is out of range "
                                 "[%d - %d]", tmpNum, fieldName, -1,
                                 OSS_SINT32_MAX ) ;

               setMaxVersionNum( (INT32)tmpNum ) ;
            }
            else if ( 0 == ossStrcmp( fieldName, FIELD_NAME_AUTODROP ) )
            {
               PD_LOG_MSG_CHECK( Bool == e.type(),
                                 SDB_INVALIDARG, error, PDERROR,
                                 "Type of field [%s] is not boolean",
                                 fieldName, rc ) ;
               setAutoDrop( e.boolean() ) ;
            }
            else
            {
               rc = SDB_OPTION_NOT_SUPPORT ;
               PD_LOG_MSG( PDERROR, "Invalid recycle bin option[%s]", fieldName ) ;
               goto error ;
            }
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to update recycle bin conf from BSON, "
                 "occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_UTILRECYBINCONF_UPDATEOPTS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYBINCONF_TOBSON, "_utilRecycleBinConf::toBSON")
   INT32 _utilRecycleBinConf::toBSON( BSONObj &object,
                                      BOOLEAN needRecycleBinField ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRECYBINCONF_TOBSON ) ;

      try
      {
         BSONObjBuilder builder ;
         if ( needRecycleBinField )
         {
            BSONObjBuilder subBuilder(
                  builder.subobjStart( FIELD_NAME_RECYCLEBIN ) ) ;

            rc = _toBSON( subBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for recycle bin "
                         "conf, rc: %d", rc ) ;

            subBuilder.done() ;
         }
         else
         {
            rc = _toBSON( builder ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for recycle bin "
                         "conf, rc: %d", rc ) ;
         }

         object = builder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build BSON for recycle bin conf, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_UTILRECYBINCONF_TOBSON, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   INT32 _utilRecycleBinConf::_toBSON( BSONObjBuilder &builder ) const
   {
      INT32 rc = SDB_OK ;

      try
      {
         builder.appendBool( FIELD_NAME_ENABLE, _enable ) ;
         builder.append( FIELD_NAME_EXPIRETIME, _expireTime ) ;
         builder.append( FIELD_NAME_MAXITEMNUM, _maxItemNum ) ;
         builder.append( FIELD_NAME_MAXVERNUM, _maxVersionNum ) ;
         builder.appendBool( FIELD_NAME_AUTODROP, _autoDrop ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build BSON for recycle bin conf, "
                 "occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   _utilRecycleBinConf &_utilRecycleBinConf::operator =(
                                             const _utilRecycleBinConf &conf )
   {
      _enable = conf._enable ;
      _expireTime = conf._expireTime ;
      _maxItemNum = conf._maxItemNum ;
      _maxVersionNum = conf._maxVersionNum ;
      _autoDrop = conf._autoDrop ;

      return (*this) ;
   }

}
