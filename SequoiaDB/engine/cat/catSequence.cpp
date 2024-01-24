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

   Source File Name = catSequence.cpp

   Descriptive Name = Sequence definition

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/19/2018  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "catSequence.hpp"
#include "catGTSDef.hpp"
#include "utilArguments.hpp"
#include "utilStr.hpp"
#include "utilMath.hpp"
#include "pdTrace.hpp"
#include "catTrace.hpp"
#include "pd.hpp"
#include "../bson/bson.hpp"
#include <string>
#include <set>

using namespace bson ;

namespace engine
{
   _catSequence::_catSequence( const std::string& name )
   {
      _name = name ;
      _internal = FALSE ;
      _version = 0 ;
      _currentValue = 1 ;
      _cachedValue = _currentValue ;
      _startValue = 1 ;
      _minValue = 1 ;
      _maxValue = OSS_SINT64_MAX ;
      _increment = 1 ;
      _cacheSize = 1000 ;
      _acquireSize = 1000 ;
      _cycled = FALSE ;
      _cycledCount = 0 ;
      _initial = TRUE ;
      _exceeded = FALSE ;
      _ID = UTIL_GLOBAL_NULL ;
      _clUniqueID = UTIL_UNIQUEID_NULL ;
   }

   _catSequence::~_catSequence()
   {
   }

   void _catSequence::setOID( const bson::OID& oid )
   {
      _oid = oid ;
   }

   void _catSequence::setInternal( BOOLEAN internal )
   {
      _internal = internal ;
   }

   void _catSequence::setCachedValue( INT64 cachedValue )
   {
      _cachedValue = cachedValue ;
   }

   void _catSequence::setVersion( INT64 version )
   {
      _version = version ;
   }

   void _catSequence::increaseVersion()
   {
      _version++ ;
   }

   void _catSequence::setCurrentValue( INT64 currentValue )
   {
      _currentValue = currentValue ;
   }

   void _catSequence::setStartValue( INT64 startValue )
   {
      _startValue = startValue ;
   }

   void _catSequence::setMinValue( INT64 minValue )
   {
      _minValue = minValue ;
   }

   void _catSequence::setMaxValue( INT64 maxValue )
   {
      _maxValue = maxValue ;
   }

   void _catSequence::setIncrement( INT32 increment )
   {
      _increment = increment ;
   }

   void _catSequence::setCacheSize( INT32 cacheSize )
   {
      _cacheSize = cacheSize ;
   }

   void _catSequence::setAcquireSize( INT32 acquireSize )
   {
      _acquireSize = acquireSize ;
   }

   void _catSequence::setCycled( BOOLEAN cycled )
   {
      _cycled = cycled ;
   }

   void _catSequence::setCycledCount( INT32 cycledCount )
   {
      _cycledCount = cycledCount ;
   }

   void _catSequence::increaseCycledCount()
   {
      _cycledCount++ ;
   }

   void _catSequence::setInitial( BOOLEAN initial )
   {
      _initial = initial ;
   }

   void _catSequence::setExceeded( BOOLEAN exceeded )
   {
      _exceeded = exceeded ;
   }

   void _catSequence::setID( utilSequenceID id )
   {
      _ID = id ;
   }

   void _catSequence::setCLUniqueID( utilCLUniqueID clUniqueID )
   {
      _clUniqueID = clUniqueID ;
   }

   INT32 _catSequence::toBSONObj( bson::BSONObj& obj, BOOLEAN forUpdate ) const
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObjBuilder builder ;
         if ( !forUpdate )
         {
            builder.append( CAT_SEQUENCE_OID, _oid ) ;
            builder.append( CAT_SEQUENCE_INTERNAL, (bool)_internal ) ;
            builder.append( CAT_SEQUENCE_ID, (INT64)_ID ) ;
            builder.append( CAT_SEQUENCE_CLUID, (INT64)_clUniqueID ) ;
         }
         builder.append( CAT_SEQUENCE_NAME, _name ) ;
         builder.append( CAT_SEQUENCE_VERSION, _version ) ;
         builder.append( CAT_SEQUENCE_CURRENT_VALUE, _currentValue ) ;
         builder.append( CAT_SEQUENCE_START_VALUE, _startValue ) ;
         builder.append( CAT_SEQUENCE_MIN_VALUE, _minValue ) ;
         builder.append( CAT_SEQUENCE_MAX_VALUE, _maxValue ) ;
         builder.append( CAT_SEQUENCE_INCREMENT, _increment ) ;
         builder.append( CAT_SEQUENCE_CACHE_SIZE, _cacheSize ) ;
         builder.append( CAT_SEQUENCE_ACQUIRE_SIZE, _acquireSize ) ;
         builder.append( CAT_SEQUENCE_CYCLED, (bool)_cycled ) ;
         builder.append( CAT_SEQUENCE_CYCLED_COUNT, _cycledCount ) ;
         builder.append( CAT_SEQUENCE_INITIAL, (bool)_initial ) ;

         obj = builder.obj() ;
      }
      catch( std::exception& e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to build BSONObj for sequence %s, exception: %s, rc=%d",
                 _name.c_str(), e.what(), rc ) ;
      }

      return rc ;
   }

   void _catSequence::copyFrom( const _catSequence& other, BOOLEAN withInternalField )
   {
      _currentValue = other.getCurrentValue() ;
      _cachedValue = other.getCachedValue() ;
      _increment = other.getIncrement() ;
      _startValue = other.getStartValue() ;
      _minValue = other.getMinValue() ;
      _maxValue = other.getMaxValue() ;
      _cacheSize = other.getCacheSize() ;
      _acquireSize = other.getAcquireSize() ;
      _cycled = other.isCycled() ;
      _cycledCount = other.getCycledCount() ;
      _initial = other.isInitial() ;
      _exceeded = other.isExceeded() ;
      if ( withInternalField )
      {
         _oid = other.getOID() ;
         _version = other.getVersion() ;
         _internal = other.isInternal() ;
         _ID = other.getID() ;
         _clUniqueID = other.getCLUniqueID() ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_SET_OPTIONS, "_catSequence::setOptions" )
   INT32 _catSequence::setOptions ( const BSONObj& options,
                                    BOOLEAN isFirstInitial,
                                    BOOLEAN withInternalFields,
                                    UINT32 * alterMask )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_GTS_SEQ_SET_OPTIONS ) ;

      UINT32 fieldMask = UTIL_ARG_FIELD_EMPTY ;
      const CHAR *readOnlyField = NULL ;

      rc = _loadOptions( options, isFirstInitial, withInternalFields,
                         fieldMask ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to load sequence options, "
                   "rc: %d", rc ) ;

      if ( OSS_BIT_TEST( fieldMask, UTIL_CL_AUTOINC_INITIAL_FIELD ) )
      {
         readOnlyField = CAT_SEQUENCE_INITIAL ;
      }
      else if ( OSS_BIT_TEST( fieldMask, UTIL_CL_AUTOINC_CYCLEDCOUNT_FIELD ) )
      {
         readOnlyField = CAT_SEQUENCE_CYCLED_COUNT ;
      }

      PD_CHECK( NULL == readOnlyField, SDB_INVALIDARG, error, PDERROR,
               "Field[%s] is read-only and not allowed to be modified",
               readOnlyField ) ;

      if ( isFirstInitial )
      {
         if ( getIncrement() < 0 )
         {
            // set default value for reversed sequence
            if ( !OSS_BIT_TEST( fieldMask, UTIL_CL_AUTOINC_STARTVALUE_FIELD ) )
            {
               setStartValue( -1 ) ;
               OSS_BIT_SET( fieldMask, UTIL_CL_AUTOINC_STARTVALUE_FIELD ) ;
            }
            if ( !OSS_BIT_TEST( fieldMask, UTIL_CL_AUTOINC_MINVALUE_FIELD ) )
            {
               setMinValue( OSS_SINT64_MIN ) ;
               OSS_BIT_SET( fieldMask, UTIL_CL_AUTOINC_MINVALUE_FIELD ) ;
            }
            if ( !OSS_BIT_TEST( fieldMask, UTIL_CL_AUTOINC_MAXVALUE_FIELD ) )
            {
               setMaxValue( -1 ) ;
               OSS_BIT_SET( fieldMask, UTIL_CL_AUTOINC_MAXVALUE_FIELD ) ;
            }
         }
         else
         {
            // set default value for sequence
            if ( !OSS_BIT_TEST( fieldMask, UTIL_CL_AUTOINC_STARTVALUE_FIELD ) )
            {
               setStartValue( 1 ) ;
               OSS_BIT_SET( fieldMask, UTIL_CL_AUTOINC_STARTVALUE_FIELD ) ;
            }
         }
      }

      if ( OSS_BIT_TEST( fieldMask, UTIL_CL_AUTOINC_CURVALUE_FIELD ) )
      {
         PD_CHECK( getCurrentValue() >= getMinValue() &&
                   getCurrentValue() <= getMaxValue(),
                   SDB_INVALIDARG, error, PDERROR,
                   "Invalid currentValue[%lld], out of bounds for "
                   "minValue[%lld] and maxValue[%lld]",
                   getCurrentValue(), getMinValue(), getMaxValue() ) ;
         // make sure getNextValue from CurrentValue not StarValue
         // when alter CurrentValue on a non-used sequence
         if ( isInitial() &&
              !OSS_BIT_TEST( fieldMask, UTIL_CL_AUTOINC_INITIAL_FIELD ) )
         {
            setInitial( FALSE ) ;
            OSS_BIT_SET( fieldMask, UTIL_CL_AUTOINC_INITIAL_FIELD ) ;
         }
      }
      else if ( isInitial() &&
                OSS_BIT_TEST( fieldMask, UTIL_CL_AUTOINC_STARTVALUE_FIELD ) )
      {
         // if is the sequence is initial, and start value is changed, reset
         // current value with start value
         setCurrentValue( getStartValue() ) ;
         setCachedValue( getStartValue() ) ;
         OSS_BIT_SET( fieldMask, UTIL_CL_AUTOINC_CURVALUE_FIELD ) ;
      }

      if ( OSS_BIT_TEST( fieldMask, UTIL_CL_AUTOINC_CYCLED_FIELD ) && !isCycled() )
      {
         setCycledCount( 0 ) ;
         OSS_BIT_SET( fieldMask, UTIL_CL_AUTOINC_CYCLEDCOUNT_FIELD ) ;
      }

      _checkExceeded() ;

      if ( NULL != alterMask )
      {
         *alterMask = fieldMask ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_GTS_SEQ_SET_OPTIONS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_LOADOPTS, "_catSequence::loadOptions" )
   INT32 _catSequence::loadOptions ( const bson::BSONObj & options )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_GTS_SEQ_LOADOPTS ) ;

      UINT32 fieldMask = UTIL_ARG_FIELD_EMPTY ;
      rc = _loadOptions( options, FALSE, TRUE, fieldMask ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to load sequence options, "
                   "rc: %d", rc ) ;

      _checkExceeded() ;

   done :
      PD_TRACE_EXITRC( SDB_GTS_SEQ_LOADOPTS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_VALIDATE, "_catSequence::validate" )
   INT32 _catSequence::validate() const
   {
      INT32 rc = SDB_OK ;
      UINT64 sequenceNum = 0 ;
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_VALIDATE ) ;

      if ( !_catSequence::isValidName( getName(), isInternal() ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid sequence name: %s",
                 getName().c_str() ) ;
         goto error ;
      }

      if ( getVersion() < 0 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid sequence version: %lld",
                 getVersion() ) ;
         goto error ;
      }

      if ( getMinValue() >= getMaxValue() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "MinValue[%lld] is greater than or equals to MaxValue[%lld]",
                 getMinValue(), getMaxValue() ) ;
         goto error ;
      }

      if ( getStartValue() < getMinValue() ||
           getStartValue() > getMaxValue() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid startValue[%lld]",
                 getStartValue() ) ;
         goto error ;
      }

      if ( 0 == getIncrement() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Increment cannot be zero" ) ;
         goto error ;
      }

      if ( getCacheSize() <= 0 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid CacheSize[%d]",
                 getCacheSize() ) ;
         goto error ;
      }

      if ( getAcquireSize() <= 0 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid FetchSize[%d]",
                 getAcquireSize() ) ;
         goto error ;
      }

      if ( getAcquireSize() > getCacheSize() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "AcquireSize[%d] is greater than CacheSize[%d]",
                 getAcquireSize(), getCacheSize() ) ;
         goto error ;
      }

      if ( getMaxValue() <= 0 || getMinValue() >= 0 )
      {
         UINT64 diff = (UINT64) ( getMaxValue() - getMinValue() ) ;
         sequenceNum = diff / utilAbs( getIncrement() ) + 1 ;
      }
      else
      {
         UINT64 diff = (UINT64) getMaxValue() + (UINT64)( -getMinValue() ) ;

         sequenceNum = diff / utilAbs( getIncrement() ) ;

         // if sequenceNum equals OSS_UINT64_MAX, plus 1 will overflow
         if ( sequenceNum != OSS_UINT64_MAX )
         {
            sequenceNum += 1 ;
         }
      }

      if ( (UINT64)getCacheSize() > sequenceNum )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "CacheSize[%d] is greater than amount of sequence value[%llu]",
                 getCacheSize(), sequenceNum ) ;
         goto error ;
      }

      if ( (UINT64)getAcquireSize() > sequenceNum )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "FetchSize[%d] is greater than amount of sequence value[%llu]",
                 getAcquireSize(), sequenceNum ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_GTS_SEQ_VALIDATE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _catSequence::isValidName( const std::string& name, BOOLEAN internal )
   {
      static CHAR* invalidUserDef[] = { "$", "SYS" } ;
      static CHAR* invalidCommon[] = { " ", "\t" } ;
      const UINT32 invalidUserDefSize = sizeof( invalidUserDef ) / sizeof( CHAR* ) ;
      const UINT32 invalidCommonSize = sizeof( invalidCommon ) / sizeof( CHAR* ) ;

      if ( !internal )
      {
         for ( UINT32 i = 0; i < invalidUserDefSize; i++ )
         {
            if ( utilStrStartsWithIgnoreCase( name, invalidUserDef[i] ) )
            {
               return FALSE ;
            }
         }
      }

      for ( UINT32 i = 0; i < invalidCommonSize; i++ )
      {
         if ( utilStrStartsWith( name, invalidCommon[i] ) )
         {
            return FALSE ;
         }
      }

      return TRUE ;
   }

   INT32 _catSequence::validateFieldNames( const bson::BSONObj& options )
   {
      static std::string fieldNameArray[] = {
         CAT_SEQUENCE_NAME,
         CAT_SEQUENCE_OID,
         CAT_SEQUENCE_ID,
         CAT_SEQUENCE_VERSION,
         CAT_SEQUENCE_CURRENT_VALUE,
         CAT_SEQUENCE_INCREMENT,
         CAT_SEQUENCE_START_VALUE,
         CAT_SEQUENCE_MIN_VALUE,
         CAT_SEQUENCE_MAX_VALUE,
         CAT_SEQUENCE_CACHE_SIZE,
         CAT_SEQUENCE_ACQUIRE_SIZE,
         CAT_SEQUENCE_CYCLED,
         CAT_SEQUENCE_CYCLED_COUNT,
         CAT_SEQUENCE_INTERNAL,
         CAT_SEQUENCE_INITIAL,
         CAT_SEQUENCE_NEXT_VALUE
      } ;
      static std::set<std::string> fieldNames( fieldNameArray,
         fieldNameArray + sizeof( fieldNameArray ) / sizeof( *fieldNameArray ) ) ;

      INT32 rc = SDB_OK ;
      BSONObjIterator iter( options ) ;

      while ( iter.more() )
      {
         BSONElement ele = iter.next() ;
         std::string fieldName = ele.fieldName() ;
         if ( fieldNames.find( fieldName ) == fieldNames.end() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Unknown sequence field: %s", fieldName.c_str() ) ;
            break ;
         }
      }

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ__LOAD_OPTIONS, "_catSequence::_loadOptions" )
   INT32 _catSequence::_loadOptions ( const bson::BSONObj & options,
                                      BOOLEAN isFirstInitial,
                                      BOOLEAN withInternalFields,
                                      UINT32 & fieldMask )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_GTS_SEQ__LOAD_OPTIONS ) ;

      BSONElement ele ;

      // CAT_SEQUENCE_INCREMENT
      ele = options.getField( CAT_SEQUENCE_INCREMENT ) ;
      if ( NumberInt == ele.type() || NumberLong == ele.type() )
      {
         INT64 increment = ele.numberLong() ;
         PD_CHECK( increment >= OSS_SINT32_MIN_LL &&
                   increment <= OSS_SINT32_MAX_LL,
                   SDB_INVALIDARG, error, PDERROR,
                   "Option [%s] is overflow: %lld",
                   CAT_SEQUENCE_INCREMENT, increment ) ;
         if ( isFirstInitial || getIncrement() != (INT32)increment )
         {
            setIncrement( (INT32)increment ) ;
            OSS_BIT_SET( fieldMask, UTIL_CL_AUTOINC_INCREMENT_FIELD ) ;
         }
      }
      else
      {
         PD_CHECK( EOO == ele.type(), SDB_INVALIDARG, error, PDERROR,
                   "Invalid type (%d) for option [%s]",
                   ele.type(), CAT_SEQUENCE_INCREMENT ) ;
      }

      // CAT_SEQUENCE_START_VALUE
      ele = options.getField( CAT_SEQUENCE_START_VALUE ) ;
      if ( NumberInt == ele.type() || NumberLong == ele.type() )
      {
         INT64 startValue = ele.numberLong() ;
         if ( isFirstInitial || getStartValue() != startValue )
         {
            setStartValue( startValue ) ;
            OSS_BIT_SET( fieldMask, UTIL_CL_AUTOINC_STARTVALUE_FIELD ) ;
         }
      }
      else
      {
         PD_CHECK( EOO == ele.type(), SDB_INVALIDARG, error, PDERROR,
                   "Invalid type (%d) for option [%s]",
                   ele.type(), CAT_SEQUENCE_START_VALUE ) ;
      }

      // CAT_SEQUENCE_MIN_VALUE
      ele = options.getField( CAT_SEQUENCE_MIN_VALUE ) ;
      if ( NumberInt == ele.type() || NumberLong == ele.type() )
      {
         INT64 minValue = ele.numberLong() ;
         if ( isFirstInitial || getMinValue() != minValue )
         {
            setMinValue( minValue ) ;
            OSS_BIT_SET( fieldMask, UTIL_CL_AUTOINC_MINVALUE_FIELD ) ;
         }
      }
      else
      {
         PD_CHECK( EOO == ele.type(), SDB_INVALIDARG, error, PDERROR,
                   "Invalid type (%d) for option [%s]",
                   ele.type(), CAT_SEQUENCE_MIN_VALUE ) ;
      }

      // CAT_SEQUENCE_MAX_VALUE
      ele = options.getField( CAT_SEQUENCE_MAX_VALUE ) ;
      if ( NumberInt == ele.type() || NumberLong == ele.type() )
      {
         INT64 maxValue = ele.numberLong() ;
         if ( isFirstInitial || getMaxValue() != maxValue )
         {
            setMaxValue( maxValue ) ;
            OSS_BIT_SET( fieldMask, UTIL_CL_AUTOINC_MAXVALUE_FIELD ) ;
         }
      }
      else
      {
         PD_CHECK( EOO == ele.type(), SDB_INVALIDARG, error, PDERROR,
                   "Invalid type (%d) for option [%s]",
                   ele.type(), CAT_SEQUENCE_MAX_VALUE ) ;
      }

      // CAT_SEQUENCE_CURRENT_VALUE
      ele = options.getField( CAT_SEQUENCE_CURRENT_VALUE ) ;
      if ( NumberInt == ele.type() || NumberLong == ele.type() )
      {
         INT64 currentValue = ele.numberLong() ;
         if ( isFirstInitial || getCurrentValue() != currentValue )
         {
            setCurrentValue( currentValue ) ;
            setCachedValue( currentValue ) ;
            OSS_BIT_SET( fieldMask, UTIL_CL_AUTOINC_CURVALUE_FIELD ) ;
         }
         else if ( isInitial() )
         {
            OSS_BIT_SET( fieldMask, UTIL_CL_AUTOINC_CURVALUE_FIELD ) ;
         }
      }
      else
      {
         PD_CHECK( EOO == ele.type(), SDB_INVALIDARG, error, PDERROR,
                   "Invalid type (%d) for option [%s]",
                   ele.type(), CAT_SEQUENCE_CURRENT_VALUE ) ;
      }

      // CAT_SEQUENCE_CACHE_SIZE
      ele = options.getField( CAT_SEQUENCE_CACHE_SIZE ) ;
      if ( NumberInt == ele.type() || NumberLong == ele.type() )
      {
         INT64 cacheSize = ele.numberLong() ;
         PD_CHECK( cacheSize >= OSS_SINT32_MIN_LL &&
                   cacheSize <= OSS_SINT32_MAX_LL,
                   SDB_INVALIDARG, error, PDERROR,
                   "Option [%s] is overflow: %lld",
                   CAT_SEQUENCE_CACHE_SIZE, cacheSize ) ;
         if ( isFirstInitial || getCacheSize() != (INT32)cacheSize )
         {
            setCacheSize( (INT32)cacheSize ) ;
            OSS_BIT_SET( fieldMask, UTIL_CL_AUTOINC_CACHESIZE_FIELD ) ;
         }
      }
      else
      {
         PD_CHECK( EOO == ele.type(), SDB_INVALIDARG, error, PDERROR,
                   "Invalid type (%d) for option [%s]",
                   ele.type(), CAT_SEQUENCE_CACHE_SIZE ) ;
      }

      // CAT_SEQUENCE_ACQUIRE_SIZE
      ele = options.getField( CAT_SEQUENCE_ACQUIRE_SIZE ) ;
      if ( NumberInt == ele.type() || NumberLong == ele.type() )
      {
         INT64 acquireSize = ele.numberLong() ;
         PD_CHECK( acquireSize >= OSS_SINT32_MIN_LL &&
                   acquireSize <= OSS_SINT32_MAX_LL,
                   SDB_INVALIDARG, error, PDERROR,
                   "Option [%s] is overflow: %lld",
                   CAT_SEQUENCE_ACQUIRE_SIZE, acquireSize ) ;
         if ( isFirstInitial || getAcquireSize() != (INT32)acquireSize )
         {
            setAcquireSize( (INT32)acquireSize ) ;
            OSS_BIT_SET( fieldMask, UTIL_CL_AUTOINC_ACQUIRESIZE_FIELD ) ;
         }
      }
      else
      {
         PD_CHECK( EOO == ele.type(), SDB_INVALIDARG, error, PDERROR,
                   "Invalid type (%d) for option [%s]",
                   ele.type(), CAT_SEQUENCE_ACQUIRE_SIZE ) ;
      }

      // CAT_SEQUENCE_CYCLED
      ele = options.getField( CAT_SEQUENCE_CYCLED ) ;
      if ( Bool == ele.type() )
      {
         bool cycled = ele.Bool();
         if ( isFirstInitial || isCycled() != (BOOLEAN)cycled )
         {
            setCycled( (BOOLEAN)cycled ) ;
            OSS_BIT_SET( fieldMask, UTIL_CL_AUTOINC_CYCLED_FIELD ) ;
         }
      }
      else
      {
         PD_CHECK( EOO == ele.type(), SDB_INVALIDARG, error, PDERROR,
                   "Invalid type (%d) for option [%s]",
                   ele.type(), CAT_SEQUENCE_CYCLED ) ;
      }

      // CAT_SEQUENCE_CYCLED_COUNT
      ele = options.getField( CAT_SEQUENCE_CYCLED_COUNT ) ;
      if ( NumberInt == ele.type() )
      {
         INT64 cycledCount = ele.numberLong() ;
         PD_CHECK( cycledCount >= OSS_SINT32_MIN_LL &&
                   cycledCount <= OSS_SINT32_MAX_LL,
                   SDB_INVALIDARG, error, PDERROR,
                   "Option [%s] is overflow: %lld",
                   CAT_SEQUENCE_CYCLED_COUNT, cycledCount ) ;
         if ( isFirstInitial || getCycledCount() != (INT32)cycledCount )
         {
            setCycledCount( (INT32)cycledCount ) ;
            OSS_BIT_SET( fieldMask, UTIL_CL_AUTOINC_CYCLEDCOUNT_FIELD ) ;
         }
      }
      else
      {
         PD_CHECK( EOO == ele.type(), SDB_INVALIDARG, error, PDERROR,
                   "Invalid type (%d) for option [%s]",
                   ele.type(), CAT_SEQUENCE_CYCLED_COUNT ) ;
      }

      // CAT_SEQUENCE_INITIAL
      ele = options.getField( CAT_SEQUENCE_INITIAL ) ;
      if ( Bool == ele.type() )
      {
         bool initial = ele.Bool();
         if ( isFirstInitial || isInitial() != (BOOLEAN)initial )
         {
            setInitial( (BOOLEAN)initial ) ;
            OSS_BIT_SET( fieldMask, UTIL_CL_AUTOINC_INITIAL_FIELD ) ;
         }
      }
      else
      {
         PD_CHECK( EOO == ele.type(), SDB_INVALIDARG, error, PDERROR,
                   "Invalid type (%d) for option [%s]",
                   ele.type(), CAT_SEQUENCE_INITIAL ) ;
      }

      if ( withInternalFields )
      {
         // CAT_SEQUENCE_OID
         ele = options.getField( CAT_SEQUENCE_OID ) ;
         if ( jstOID == ele.type() )
         {
            setOID( ele.OID() ) ;
         }
         else
         {
            PD_CHECK( EOO == ele.type(), SDB_INVALIDARG, error, PDERROR,
                      "Invalid type (%d) for option [%s]",
                      ele.type(), CAT_SEQUENCE_OID ) ;
         }

         // CAT_SEQUENCE_ID
         ele = options.getField( CAT_SEQUENCE_ID ) ;
         if ( ele.isNumber() )
         {
            setID( (utilSequenceID)ele.numberLong() ) ;
         }
         else
         {
            PD_CHECK( EOO == ele.type(), SDB_INVALIDARG, error, PDERROR,
                      "Invalid type (%d) for option [%s]",
                      ele.type(), CAT_SEQUENCE_ID ) ;
         }

         // CAT_SEQUENCE_VERSION
         ele = options.getField( CAT_SEQUENCE_VERSION ) ;
         if ( NumberInt == ele.type() || NumberLong == ele.type() )
         {
            setVersion( ele.numberLong() ) ;
         }
         else
         {
            PD_CHECK( EOO == ele.type(), SDB_INVALIDARG, error, PDERROR,
                      "Invalid type (%d) for option [%s]",
                      ele.type(), CAT_SEQUENCE_VERSION ) ;
         }

         // CAT_SEQUENCE_CLUID
         ele = options.getField( CAT_SEQUENCE_CLUID ) ;
         if ( NumberInt == ele.type() || NumberLong == ele.type() )
         {
            setCLUniqueID( ele.numberLong() ) ;
         }
         else
         {
            PD_CHECK( EOO == ele.type(), SDB_INVALIDARG, error, PDERROR,
                      "Invalid type (%d) for option [%s]",
                      ele.type(), CAT_SEQUENCE_CLUID ) ;
         }
      }

      if ( isFirstInitial || withInternalFields )
      {
         // CAT_SEQUENCE_INTERNAL
         ele = options.getField( CAT_SEQUENCE_INTERNAL ) ;
         if ( Bool == ele.type() )
         {
            setInternal( (BOOLEAN)ele.boolean() ) ;
         }
         else
         {
            PD_CHECK( EOO == ele.type(), SDB_INVALIDARG, error, PDERROR,
                      "Invalid type (%d) for option [%s]",
                      ele.type(), CAT_SEQUENCE_INTERNAL ) ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB_GTS_SEQ__LOAD_OPTIONS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ__CHKEXCEEDED, "_catSequence::_checkExceeded" )
   void _catSequence::_checkExceeded ()
   {
      PD_TRACE_ENTRY( SDB_GTS_SEQ__CHKEXCEEDED ) ;

      if ( !isInitial() )
      {
         // clear flag before exceeded check
         setExceeded( FALSE ) ;

         if ( getIncrement() > 0 )
         {
            if ( getCachedValue() == getMaxValue() ||
                 getCachedValue() > ( getMaxValue() - getIncrement() ) ||
                 getCachedValue() < getMinValue() )
            {
               setExceeded( TRUE ) ;
            }
         }
         else if ( getIncrement() < 0 )
         {
            if ( getCachedValue() == getMinValue() ||
                 getCachedValue() < ( getMinValue() - getIncrement() ) ||
                 getCachedValue() > getMaxValue() )
            {
               setExceeded( TRUE ) ;
            }
         }
      }

      PD_TRACE_EXIT( SDB_GTS_SEQ__CHKEXCEEDED ) ;
   }

}

