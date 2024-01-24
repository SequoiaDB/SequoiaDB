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

   Source File Name = sptDBDate.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          19/01/2018  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptDBDate.hpp"
#include "ossUtil.hpp"
#include "utilStr.hpp"
#include <boost/lexical_cast.hpp>
using namespace bson ;
namespace engine
{
   #define SPT_DATE_NAME       "SdbDate"
   #define SPT_DATE_DATE_FIELD "_d"
   #define SPT_DATA_SPECIAL_FIELD "$date"

   JS_CONSTRUCT_FUNC_DEFINE( _sptDBDate, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptDBDate, destruct )
   JS_STATIC_FUNC_DEFINE( _sptDBDate, help )

   JS_BEGIN_MAPPING( _sptDBDate, SPT_DATE_NAME )
      JS_ADD_CONSTRUCT_FUNC( construct )
      JS_ADD_DESTRUCT_FUNC( destruct )
      JS_SET_SPECIAL_FIELD_NAME( SPT_DATA_SPECIAL_FIELD )
      JS_ADD_STATIC_FUNC( "help", help )
      JS_ADD_MEMBER_FUNC( "help", help )
      JS_SET_CVT_TO_BSON_FUNC( _sptDBDate::cvtToBSON )
      JS_SET_JSOBJ_TO_BSON_FUNC( _sptDBDate::fmpToBSON )
      JS_SET_BSON_TO_JSOBJ_FUNC( _sptDBDate::bsonToJSObj )
   JS_MAPPING_END()
   _sptDBDate::_sptDBDate()
   {
   }

   _sptDBDate::~_sptDBDate()
   {
   }

   INT32 _sptDBDate::construct( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string timeStr ;
      INT64 mills = 0 ;
      BOOLEAN isString = FALSE ;
      if( arg.argc() == 0 )
      {
         ossTimestamp currentTime ;
         struct tm localTm ;
         time_t t ;
         CHAR buf[128] ;

         ossGetCurrentTime( currentTime ) ;
         t = currentTime.time ;
         ossLocalTime( t, localTm ) ;
         ossSnprintf( buf, 127,
                      "%04d-%02d-%02d",
                      localTm.tm_year+1900,            // 1) Year (UINT32)
                      localTm.tm_mon+1,                // 2) Month (UINT32)
                      localTm.tm_mday ) ;                 // 3) Day (UINT32)
         timeStr = buf ;
         isString = TRUE ;
      }
      else if( arg.argc() == 1 )
      {

         if( arg.isString( 0 ) )
         {
            UINT64 tmpMills = 0 ;
            rc = arg.getString( 0, timeStr ) ;
            if( SDB_OK != rc )
            {
               goto error ;
            }
            rc = utilStr2Date( timeStr.c_str(), tmpMills ) ;
            if( SDB_OK != rc )
            {
               rc = SDB_OK ;
               // maybe the format is {dateKey:SdbDate("-30610252800000")}, try to parse it
               try
               {
                  mills = boost::lexical_cast< INT64 >( timeStr ) ;
               }
               catch( boost::bad_lexical_cast & )
               {
                  rc = SDB_INVALIDARG ;
                  detail = BSON( SPT_ERR << "Invalid SdbDate value: " + timeStr ) ;
                  goto error ;
               }
            }
            else
            {
               isString = TRUE ;
            }
         }
         else if( arg.isNumber( 0 ) )
         {
            FLOAT64 dp = 0 ;
            rc = arg.getNative( 0, &dp, SPT_NATIVE_FLOAT64 ) ;
            if( SDB_OK != rc )
            {
               goto error ;
            }
            mills = static_cast< INT64 >( dp ) ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << "SdbDate argument must be String or Number") ;
            goto error ;
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "SdbDate() was given too many arguments" ) ;
         goto error ;
      }

      if( isString )
      {
         rval.addSelfProperty( SPT_DATE_DATE_FIELD )->setValue( timeStr ) ;
      }
      else
      {
         rval.addSelfProperty( SPT_DATE_DATE_FIELD )->setValue( (FLOAT64)mills ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBDate::destruct()
   {
      return SDB_OK ;
   }
   INT32 _sptDBDate::cvtToBSON( const CHAR* key, const sptObject &value,
                                BOOLEAN isSpecialObj, BSONObjBuilder& builder,
                                string &errMsg )
   {
      INT32 rc = SDB_OK ;
      string dateStr ;
      string fieldName ;
      UINT64 tm = 0 ;
      if( isSpecialObj )
      {
         fieldName = SPT_DATA_SPECIAL_FIELD ;
      }
      else
      {
         fieldName = SPT_DATE_DATE_FIELD ;
      }
      rc = value.getStringField( fieldName, dateStr,
                                 SPT_CVT_FLAGS_FROM_STRING |
                                 SPT_CVT_FLAGS_FROM_OBJECT ) ;
      if( SDB_OK != rc )
      {
         FLOAT64 tmpDateVal ;
         rc = value.getDoubleField( fieldName, tmpDateVal,
                                    SPT_CVT_FLAGS_FROM_DOUBLE |
                                    SPT_CVT_FLAGS_FROM_INT ) ;
         if( SDB_OK != rc )
         {
            errMsg = "SdbDate date value mast be String or Number" ;
            goto error ;
         }
         tm = tmpDateVal ;
      }
      else
      {
         rc = engine::utilStr2Date( dateStr.c_str(), tm ) ;
         if( SDB_OK != rc )
         {
            rc = SDB_OK ;
            try
            {
               tm = boost::lexical_cast<UINT64>( dateStr.c_str() ) ;
            }
            catch( boost::bad_lexical_cast & )
            {
               errMsg = "Invalid SdbDate value: " + dateStr ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }
      }
      builder.appendDate( key, static_cast< Date_t >( tm ) ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBDate::fmpToBSON( const sptObject &value, BSONObj &retObj,
                                string &errMsg )
   {
      INT32 rc = SDB_OK ;
      string fieldName ;
      string dateStr ;
      rc = value.getStringField( SPT_DATE_DATE_FIELD, dateStr ) ;
      if( SDB_OK != rc )
      {
         FLOAT64 tmpDateVal ;
         rc = value.getDoubleField( SPT_DATE_DATE_FIELD, tmpDateVal,
                                    SPT_CVT_FLAGS_FROM_DOUBLE |
                                    SPT_CVT_FLAGS_FROM_INT ) ;
         if( SDB_OK != rc )
         {
            errMsg = "Failed to convert Date" ;
            goto error ;
         }
         retObj = BSON( SPT_DATE_DATE_FIELD << tmpDateVal ) ;
      }
      else
      {
         retObj = BSON( SPT_DATE_DATE_FIELD << dateStr ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBDate::bsonToJSObj( sdbclient::sdb &db, const BSONObj &data,
                                  _sptReturnVal &rval, bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      sptDBDate *pDate = SDB_OSS_NEW sptDBDate() ;
      BSONType type ;
      BSONElement ele ;
      if( NULL == pDate )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Failed to new sptDBDate obj" ) ;
         goto error ;
      }
      rc = rval.setUsrObjectVal< sptDBDate >( pDate ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to set return obj" ) ;
         goto error ;
      }
      ele = data.getField( SPT_DATE_DATE_FIELD ) ;
      type = ele.type() ;
      if( String == type )
      {
         string tmpDateStr = ele.String() ;
         rval.addReturnValProperty( SPT_DATE_DATE_FIELD )->setValue( tmpDateStr ) ;
      }
      else if( NumberDouble == type )
      {
         INT64 date = ele.Double() ;
         rval.addReturnValProperty( SPT_DATE_DATE_FIELD )->setValue( date ) ;
      }
   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pDate ) ;
      goto done ;
   }

   INT32 _sptDBDate::help( const _sptArguments &arg,
                           _sptReturnVal &rval,
                           BSONObj &detail )
   {
      stringstream ss ;
      ss << endl ;
      ss << "   --Constructor methods for class \"SdbDate\" : " << endl ;
      ss << "   { \"$date\": <date> }" << endl ;
      ss << "   SdbDate( [date] )          "
         << "- Data type: date" << endl ;
      ss << endl ;
      ss << "   --Static methods for class \"SdbDate\" : " << endl ;
      ss << endl ;
      ss << "   --Instance methods for class \"SdbDate\" : " << endl ;
      rval.getReturnVal().setValue( ss.str() ) ;
      return SDB_OK ;
   }
}
