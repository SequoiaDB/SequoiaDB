/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = rplField.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/04/2019  Linyoubin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rplField.hpp"
#include "../bson/bson.hpp"
#include "../bson/lib/base64.h"
#include "ossUtil.hpp"
#include "rplConfDef.hpp"
#include "rplUtil.hpp"
#include <sstream>
#include <string>
#include <cmath>

using namespace std ;
using namespace bson;
using namespace engine ;

namespace replay
{
   rplConstStringField::rplConstStringField()
   {
   }

   rplConstStringField::~rplConstStringField()
   {
   }

   EN_FieldType rplConstStringField::getFieldType() const
   {
      return CONST_STRING ;
   }

   INT32 rplConstStringField::init( const BSONObj &fieldConf )
   {
      _value = fieldConf.getStringField( RPL_CONF_NAME_FIELD_CONSTVALUE ) ;
      return SDB_OK ;
   }

   INT32 rplConstStringField::getValue( const BSONObj &sRecord, string &value )
   {
      value = _value ;
      return SDB_OK ;
   }

   rplOutputTimeField::rplOutputTimeField()
   {
   }

   rplOutputTimeField::~rplOutputTimeField()
   {
   }

   INT32 rplOutputTimeField::init( const BSONObj &fieldConf )
   {
      return SDB_OK ;
   }

   EN_FieldType rplOutputTimeField::getFieldType() const
   {
      return OUTPUT_TIME ;
   }

   void rplOutputTimeField::getCurrentTimeStr( string &timeStr )
   {
      ossTimestamp Tm ;
      CHAR szFormat[] = "%04d-%02d-%02d %02d.%02d.%02d.%06d" ;
      CHAR szTimestmpStr[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
      struct tm tmpTm ;

      ossGetCurrentTime( Tm ) ;
      ossLocalTime( Tm.time, tmpTm ) ;

      if ( Tm.microtm >= OSS_ONE_MILLION )
      {
         tmpTm.tm_sec ++ ;
         Tm.microtm %= OSS_ONE_MILLION ;
      }

      ossSnprintf ( szTimestmpStr, sizeof( szTimestmpStr ),
                    szFormat,
                    tmpTm.tm_year + 1900,
                    tmpTm.tm_mon + 1,
                    tmpTm.tm_mday,
                    tmpTm.tm_hour,
                    tmpTm.tm_min,
                    tmpTm.tm_sec,
                    Tm.microtm ) ;

      timeStr = szTimestmpStr ;
   }

   INT32 rplOutputTimeField::getValue( const BSONObj &sRecord, string &value )
   {
      getCurrentTimeStr( value ) ;
      return SDB_OK ;
   }

   rplOriginalTimeField::rplOriginalTimeField()
   {
   }

   rplOriginalTimeField::~rplOriginalTimeField()
   {
   }

   INT32 rplOriginalTimeField::init( const BSONObj &fieldConf )
   {
      return SDB_OK ;
   }

   EN_FieldType rplOriginalTimeField::getFieldType() const
   {
      return ORIGINAL_TIME ;
   }

   INT32 rplOriginalTimeField::getValue( const BSONObj &sRecord, string &value )
   {
      // should not call this function
      return SDB_INVALIDARG ;
   }

   rplAutoOPField::rplAutoOPField()
   {
   }

   rplAutoOPField::~rplAutoOPField()
   {
   }

   INT32 rplAutoOPField::init( const BSONObj &fieldConf )
   {
      return SDB_OK ;
   }

   EN_FieldType rplAutoOPField::getFieldType() const
   {
      return AUTO_OP ;
   }

   INT32 rplAutoOPField::getValue( const BSONObj &sRecord, string &value )
   {
      // should not call this function
      return SDB_INVALIDARG ;
   }

   rplMappingField::rplMappingField()
   {
      _hasDefaultValue = FALSE ;
      _defaultValue = "" ;
   }

   rplMappingField::~rplMappingField()
   {
   }

   INT32 rplMappingField::init( const BSONObj &fieldConf )
   {
      INT32 rc = SDB_OK ;
      BSONElement defaultValueEle ;

      //{ source: "fieldName", target: "columnName" }
      const CHAR *sFieldName = fieldConf.getStringField( RPL_CONF_NAME_SOURCE ) ;
      const CHAR *tFieldName = fieldConf.getStringField( RPL_CONF_NAME_TARGET ) ;
      if ( '\0' == sFieldName[0] )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "Field(%s) can't be empty, rc = %d",
                      RPL_CONF_NAME_SOURCE, rc ) ;
      }

      ossSnprintf( _sFieldName, sizeof(_sFieldName) - 1, "%s", sFieldName ) ;
      ossSnprintf( _tFieldName, sizeof(_tFieldName) - 1, "%s", tFieldName ) ;

      defaultValueEle = fieldConf.getField( RPL_CONF_NAME_FIELD_DEFAULTVALUE ) ;
      if ( defaultValueEle.type() == EOO )
      {
         _hasDefaultValue = FALSE ;
      }
      else
      {
         _hasDefaultValue = TRUE ;
         _defaultValue = defaultValueEle.str() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rplMappingField::getValue( const BSONObj &sRecord, string &value )
   {
      INT32 rc = SDB_OK ;
      BSONElement ele = sRecord.getField( _sFieldName ) ;
      if ( ele.type() == EOO )
      {
         if ( !_hasDefaultValue )
         {
            rc = SDB_INVALIDARG ;
            PD_RC_CHECK( rc, PDERROR, "Invalid field type(%s:%d), rc = %d",
                         _sFieldName, ele.type(), rc ) ;
         }

         value = _defaultValue ;
      }
      else
      {
         rc = _getValue( ele, value ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get value, rc = %d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   rplMappingStrField::rplMappingStrField()
   {
   }

   rplMappingStrField::~rplMappingStrField()
   {
   }

   INT32 rplMappingStrField::init( const BSONObj & fieldConf )
   {
      INT32 rc = SDB_OK ;
      rc = rplMappingField::init( fieldConf ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init rplMappingStrField, rc = %d",
                   rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   EN_FieldType rplMappingStrField::getFieldType() const
   {
      return MAPPING_STRING ;
   }

   INT32 rplMappingStrField::_getValue( const BSONElement &ele, string &value )
   {
      INT32 rc = SDB_OK ;
      if ( ele.type() == String )
      {
         value = ele.str() ;
      }
      else if ( ele.type() == jstOID )
      {
         OID oid = ele.OID() ;
         value = oid.toString() ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "Invalid field type(%s:%d), rc = %d",
                      _sFieldName, ele.type(), rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   rplIntField::rplIntField()
   {
   }

   rplIntField::~rplIntField()
   {
   }

   INT32 rplIntField::init( const BSONObj &fieldConf )
   {
      INT32 rc = SDB_OK ;
      rc = rplMappingField::init( fieldConf ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init rplIntField, rc = %d",
                   rc ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   EN_FieldType rplIntField::getFieldType() const
   {
      return MAPPING_INT ;
   }

   INT32 rplIntField::_getValue( const BSONElement &ele, string &value )
   {
      INT32 rc = SDB_OK ;

      rc = rplCastNumberToString( ele, NumberInt, value ) ;
      PD_RC_CHECK( rc, PDERROR, "Invalid field type(%s:%d), rc = %d",
                   _sFieldName, ele.type(), rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   rplLongField::rplLongField()
   {
   }

   rplLongField::~rplLongField()
   {
   }

   INT32 rplLongField::init( const BSONObj &fieldConf )
   {
      INT32 rc = SDB_OK ;
      rc = rplMappingField::init( fieldConf ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init rplLongField, rc = %d",
                   rc ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   EN_FieldType rplLongField::getFieldType() const
   {
      return MAPPING_LONG ;
   }

   INT32 rplLongField::_getValue( const BSONElement &ele, string &value )
   {
      INT32 rc = SDB_OK ;

      rc = rplCastNumberToString( ele, NumberLong, value ) ;
      PD_RC_CHECK( rc, PDERROR, "Invalid field type(%s:%d), rc = %d",
                   _sFieldName, ele.type(), rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   rplDecimalField::rplDecimalField()
   {
   }

   rplDecimalField::~rplDecimalField()
   {
   }

   INT32 rplDecimalField::init( const BSONObj &fieldConf )
   {
      INT32 rc = SDB_OK ;
      rc = rplMappingField::init( fieldConf ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init rplDecimalField, rc = %d",
                   rc ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   EN_FieldType rplDecimalField::getFieldType() const
   {
      return MAPPING_DECIMAL ;
   }

   INT32 rplDecimalField::_getValue( const BSONElement &ele, string &value )
   {
      INT32 rc = SDB_OK ;

      rc = rplCastNumberToString( ele, NumberDecimal, value ) ;
      PD_RC_CHECK( rc, PDERROR, "Invalid field type(%s:%d), rc = %d",
                   _sFieldName, ele.type(), rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   rplTimestampField::rplTimestampField()
   {
   }

   rplTimestampField::~rplTimestampField()
   {
   }

   INT32 rplTimestampField::init( const BSONObj &fieldConf )
   {
      INT32 rc = SDB_OK ;
      rc = rplMappingField::init( fieldConf ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init rplTimestampField, rc = %d",
                   rc ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   EN_FieldType rplTimestampField::getFieldType() const
   {
      return MAPPING_TIMESTAMP ;
   }

   INT32 rplTimestampField::_getValue( const BSONElement &ele, string &value )
   {
      INT32 rc = SDB_OK ;
      UINT32 inc = 0 ;
      string timeStr ;
      Date_t date ;
      time_t timer ;
      ossTimestamp timestamp ;
      if ( ele.type() != Timestamp )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "Invalid field type(%s:%d), rc = %d",
                      _sFieldName, ele.type(), rc ) ;
      }

      date = ele.timestampTime() ;
      inc = ele.timestampInc() ;
      timer = (time_t)(((long long)date.millis)/1000) ;

      rplTimeIncToString( timer, inc, timeStr ) ;
      value = timeStr ;

   done:
      return rc ;
   error:
      goto done ;
   }

   rplDoubleField::rplDoubleField()
   : _fullPrecision( TRUE ),
     _floatFmt( RPL_FLOATFMT_DEFAULT )
   {
   }

   rplDoubleField::~rplDoubleField()
   {
   }

   INT32 rplDoubleField::init( const BSONObj &fieldConf )
   {
      INT32 rc = SDB_OK ;
      const CHAR *floatFmt = NULL ;

      rc = rplMappingField::init( fieldConf ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init rplDoubleField, rc = %d",
                   rc ) ;

      floatFmt = fieldConf.getStringField( RPL_CONF_NAME_FIELD_FLOATFMT ) ;
      if ( NULL == floatFmt || '\0' == floatFmt[0] )
      {
         /* Default: Ryu shortest round-trip (same as floatFmt:"full") */
         _fullPrecision = TRUE ;
         _floatFmt = RPL_FLOATFMT_DEFAULT ;
      }
      else if ( 0 == ossStrcmp( floatFmt, RPL_FLOATFMT_FULL ) )
      {
         _fullPrecision = TRUE ;
         _floatFmt = RPL_FLOATFMT_DEFAULT ;
      }
      else if ( 0 == ossStrcmp( floatFmt, RPL_FLOATFMT_DB2 ) )
      {
         _fullPrecision = FALSE ;
         _floatFmt = RPL_FLOATFMT_DB2_VALUE ;
      }
      else
      {
         rc = rplCheckFloatFmt( floatFmt ) ;
         PD_RC_CHECK( rc, PDERROR, "%s(%s) is invalid, rc = %d",
                      RPL_CONF_NAME_FIELD_FLOATFMT, floatFmt, rc ) ;
         _fullPrecision = FALSE ;
         _floatFmt = floatFmt ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   EN_FieldType rplDoubleField::getFieldType() const
   {
      return MAPPING_DOUBLE ;
   }

   INT32 rplDoubleField::_getValue( const BSONElement &ele, string &value )
   {
      INT32 rc = SDB_OK ;
      FLOAT64 dVal = 0.0 ;

      if ( ele.type() == NumberDouble )
      {
         dVal = ele.Double() ;
      }
      else if ( ele.type() == NumberInt )
      {
         dVal = (FLOAT64)ele.Int() ;
      }
      else if ( ele.type() == NumberLong )
      {
         dVal = (FLOAT64)ele.Long() ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "Invalid field type(%s:%d), rc = %d",
                      _sFieldName, ele.type(), rc ) ;
      }

      if ( std::isinf( dVal ) )
      {
         value.clear() ;
         goto done ;
      }

      if ( std::isnan( dVal ) )
      {
         if ( _hasDefaultValue )
         {
            value = _defaultValue ;
         }
         else
         {
            value.clear() ;
         }
         goto done ;
      }

      rplDoubleToString( dVal, _fullPrecision, _floatFmt.c_str(), value ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   rplDateField::rplDateField()
   {
   }

   rplDateField::~rplDateField()
   {
   }

   INT32 rplDateField::init( const BSONObj &fieldConf )
   {
      INT32 rc = SDB_OK ;
      rc = rplMappingField::init( fieldConf ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init rplDateField, rc = %d",
                   rc ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   EN_FieldType rplDateField::getFieldType() const
   {
      return MAPPING_DATE ;
   }

   INT32 rplDateField::_getValue( const BSONElement &ele, string &value )
   {
      INT32 rc = SDB_OK ;
      if ( ele.type() != Date )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "Invalid field type(%s:%d), rc = %d",
                      _sFieldName, ele.type(), rc ) ;
      }

      rplDateToString( (INT64)ele.Date().millis, value ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   rplBinaryField::rplBinaryField()
   {
   }

   rplBinaryField::~rplBinaryField()
   {
   }

   INT32 rplBinaryField::init( const BSONObj &fieldConf )
   {
      INT32 rc = SDB_OK ;
      rc = rplMappingField::init( fieldConf ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init rplBinaryField, rc = %d",
                   rc ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   EN_FieldType rplBinaryField::getFieldType() const
   {
      return MAPPING_BINARY ;
   }

   INT32 rplBinaryField::_getValue( const BSONElement &ele, string &value )
   {
      INT32 rc = SDB_OK ;
      INT32 len = 0 ;
      const CHAR *data = NULL ;

      if ( ele.type() != BinData )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "Invalid field type(%s:%d), rc = %d",
                      _sFieldName, ele.type(), rc ) ;
      }

      // Export BinData payload only as Base64; ignore BSON subtype ($type).
      data = ele.binDataClean( len ) ;
      if ( NULL == data || len <= 0 )
      {
         value = "" ;
         goto done ;
      }

      value = base64::encode( data, len ) ;

   done:
      return rc ;
   error:
      goto done ;
   }
}

