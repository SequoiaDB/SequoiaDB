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

   Source File Name = rtnSessionProperty.cpp

   Descriptive Name = session properties

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/01/2018  HGM Initial Draft, split from coordRemoteSession.cpp

   Last Changed =

*******************************************************************************/

#include "rtnSessionProperty.hpp"
#include "dpsTransExecutor.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"
#include "msg.hpp"
#include "msgDef.hpp"
#include "pmdOptionsMgr.hpp"

using namespace bson ;

namespace engine
{

   /*
      _rtnInstanceOption implement
    */
   _rtnInstanceOption::_rtnInstanceOption ()
   : _mode( (UINT8)PREFER_INSTANCE_MODE_UNKNOWN ),
     _strict( 0 ),
     _specInstance( (INT8)PREFER_INSTANCE_TYPE_UNKNOWN ),
     _period( PREFER_INSTANCE_DEF_PERIOD ),
     _instanceList()
   {
   }

   _rtnInstanceOption::_rtnInstanceOption ( const rtnInstanceOption & option )
   : _mode( option._mode ),
     _strict( option._strict ),
     _specInstance( option._specInstance ),
     _period( option._period ),
     _instanceList()
   {
      _instanceList = option._instanceList ;
   }

   _rtnInstanceOption::~_rtnInstanceOption ()
   {
   }

   rtnInstanceOption & _rtnInstanceOption::operator = ( const rtnInstanceOption & option )
   {
      _mode = option._mode ;
      _strict = option._strict ;
      _specInstance = option._specInstance ;
      _period = option._period ;
      _instanceList = option._instanceList ;
      return ( *this ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNINST_RESET "_rtnInstanceOption::reset" )
   void _rtnInstanceOption::reset ()
   {
      PD_TRACE_ENTRY( SDB__RTNINST_RESET ) ;

      _mode = (UINT8)PREFER_INSTANCE_MODE_UNKNOWN ;
      _strict = 0 ;
      _period = PREFER_INSTANCE_DEF_PERIOD ;
      _clearInstance() ;

      PD_TRACE_EXIT( SDB__RTNINST_RESET ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNINST_SETPREFINST_REPL, "_rtnInstanceOption::setPreferredInstance" )
   INT32 _rtnInstanceOption::setPreferredInstance ( PREFER_REPLICA_TYPE replType )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNINST_SETPREFINST_REPL ) ;

      _clearInstance() ;

      switch ( replType )
      {
         case PREFER_REPL_MASTER :
            _specInstance = (INT8)PREFER_INSTANCE_TYPE_MASTER ;
            break ;
         case PREFER_REPL_SLAVE :
            _specInstance = (INT8)PREFER_INSTANCE_TYPE_SLAVE ;
            break ;
         case PREFER_REPL_ANYONE :
            _specInstance = (INT8)PREFER_INSTANCE_TYPE_ANYONE ;
            break ;
         case PREFER_REPL_NODE_1 :
         case PREFER_REPL_NODE_2 :
         case PREFER_REPL_NODE_3 :
         case PREFER_REPL_NODE_4 :
         case PREFER_REPL_NODE_5 :
         case PREFER_REPL_NODE_6 :
         case PREFER_REPL_NODE_7 :
            _instanceList.push_back( (UINT8)replType ) ;
            break ;
         default :
            rc = SDB_INVALIDARG ;
            PD_LOG( PDWARNING, "Unknown preferred replica type: %d",
                    replType ) ;
            break ;
      }

      PD_TRACE_EXITRC( SDB__RTNINST_SETPREFINST_REPL, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNINST_SETPREFINST, "_rtnInstanceOption::setPreferredInstance" )
   INT32 _rtnInstanceOption::setPreferredInstance ( RTN_PREFER_INSTANCE_TYPE instance )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNINST_SETPREFINST ) ;

      if ( instance > PREFER_INSTANCE_TYPE_MIN &&
           instance < PREFER_INSTANCE_TYPE_MAX )
      {
         _instanceList.push_back( (UINT8)instance ) ;
      }
      else if ( PREFER_INSTANCE_TYPE_MASTER == instance ||
                PREFER_INSTANCE_TYPE_SLAVE == instance ||
                PREFER_INSTANCE_TYPE_ANYONE == instance ||
                PREFER_INSTANCE_TYPE_MASTER_SND == instance ||
                PREFER_INSTANCE_TYPE_SLAVE_SND == instance ||
                PREFER_INSTANCE_TYPE_ANYONE_SND == instance )
      {
         _specInstance = (INT8)instance ;
      }

      PD_TRACE_EXITRC( SDB__RTNINST_SETPREFINST, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNINST_SETPREFINSTMODE, "_rtnInstanceOption::setPreferredInstanceMode" )
   INT32 _rtnInstanceOption::setPreferredInstanceMode ( RTN_PREFER_INSTANCE_MODE mode )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNINST_SETPREFINSTMODE ) ;

      _mode = (UINT8)mode ;

      PD_TRACE_EXITRC( SDB__RTNINST_SETPREFINSTMODE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNINST_PARSEPREFINST_BSON, "_rtnInstanceOption::parsePreferredInstance" )
   INT32 _rtnInstanceOption::parsePreferredInstance ( const BSONElement & option )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNINST_PARSEPREFINST_BSON ) ;

      _clearInstance() ;

      switch ( option.type() )
      {
         case NumberInt :
            rc = _parseIntegerPreferredInstance( option ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse integer option of "
                         "preferred instance, rc: %d", rc ) ;
            break ;
         case String :
            rc = _parseStringPreferredInstance( option ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse string option of "
                         "preferred instance, rc: %d", rc ) ;
            break ;
         case Array :
            rc = _parseArrayPreferredInstance( option ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse string option of "
                         "preferred instance, rc: %d", rc ) ;
            break ;
         default :
            rc = SDB_INVALIDARG ;
            PD_LOG( PDWARNING, "Failed to parse preferred instance: "
                    "should be array, integer or string" ) ;
            goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNINST_PARSEPREFINST_BSON, rc ) ;
      return rc ;

   error :
      _clearInstance() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNINST__PARSEINTPREFINST, "_rtnInstanceOption::_parseIntegerPreferredInstance" )
   INT32 _rtnInstanceOption::_parseIntegerPreferredInstance ( const BSONElement & option )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNINST__PARSEINTPREFINST ) ;

      SDB_ASSERT( NumberInt == option.type(), "integer is required" ) ;

      INT32 prefInst = option.numberInt() ;

      PD_CHECK( prefInst > PREFER_INSTANCE_TYPE_MIN &&
                prefInst < PREFER_INSTANCE_TYPE_MAX,
                SDB_INVALIDARG, error, PDWARNING, "Failed to parse "
                "preferred instance: [%d] out of range ( %d ~ %d )",
                prefInst, PREFER_INSTANCE_TYPE_MIN, PREFER_INSTANCE_TYPE_MAX ) ;

      _instanceList.push_back( (UINT8)prefInst ) ;

   done :
      PD_TRACE_EXITRC( SDB__RTNINST__PARSEINTPREFINST, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNINST__PARSESTRPREFINST, "_rtnInstanceOption::_parseStringPreferredInstance" )
   INT32 _rtnInstanceOption::_parseStringPreferredInstance ( const BSONElement & option )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNINST__PARSESTRPREFINST ) ;

      SDB_ASSERT( String == option.type(), "string is required" ) ;

      const CHAR * instanceStr = option.valuestrsafe() ;

      if ( 0 == ossStrcasecmp( instanceStr,
                               PREFER_INSTANCE_MASTER_STR ) )
      {
         _specInstance = (INT8)PREFER_INSTANCE_TYPE_MASTER ;
      }
      else if ( 0 == ossStrcasecmp( instanceStr,
                                    PREFER_INSTANCE_SLAVE_STR ) )
      {
         _specInstance = (INT8)PREFER_INSTANCE_TYPE_SLAVE ;
      }
      else if ( 0 == ossStrcasecmp( instanceStr,
                                    PREFER_INSTANCE_ANY_STR ) )
      {
         _specInstance = (INT8)PREFER_INSTANCE_TYPE_ANYONE ;
      }
      else if ( 0 == ossStrcasecmp( instanceStr,
                                    PREFER_INSTANCE_MASTER_SND_STR ) )
      {
         _specInstance = (INT8)PREFER_INSTANCE_TYPE_MASTER_SND ;
      }
      else if ( 0 == ossStrcasecmp( instanceStr,
                                    PREFER_INSTANCE_SLAVE_SND_STR ) )
      {
         _specInstance = (INT8)PREFER_INSTANCE_TYPE_SLAVE_SND ;
      }
      else if ( 0 == ossStrcasecmp( instanceStr,
                                    PREFER_INSTANCE_ANY_SND_STR ) )
      {
         _specInstance = (INT8)PREFER_INSTANCE_TYPE_ANYONE_SND ;
      }
      else
      {
         _specInstance = (INT8)PREFER_INSTANCE_TYPE_UNKNOWN ;
      }

      PD_CHECK( PREFER_INSTANCE_TYPE_UNKNOWN != _specInstance, SDB_INVALIDARG,
                error, PDWARNING, "Failed to parse preferred instance: "
                "[%s] is unknown type", instanceStr ) ;

   done :
      PD_TRACE_EXITRC( SDB__RTNINST__PARSESTRPREFINST, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNINST__PARSEARRPREFINST, "_rtnInstanceOption::_parseArrayPreferredInstance" )
   INT32 _rtnInstanceOption::_parseArrayPreferredInstance ( const BSONElement & option )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNINST__PARSEARRPREFINST ) ;

      SDB_ASSERT( Array == option.type(), "array is required" ) ;

      BSONObjIterator iter( option.embeddedObject() ) ;
      while ( iter.more() )
      {
         BSONElement curOption = iter.next() ;
         if ( NumberInt == curOption.type() )
         {
            _parseIntegerPreferredInstance( curOption ) ;
         }
         else if ( String == curOption.type() &&
                   PREFER_INSTANCE_TYPE_UNKNOWN == _specInstance )
         {
            _parseStringPreferredInstance( curOption ) ;
         }
         else
         {
            PD_LOG( PDWARNING, "Unknown type of preferred instance [%s]",
                    curOption.toString( FALSE, TRUE ).c_str() ) ;
         }
      }

      PD_CHECK( isValidated(), SDB_INVALIDARG, error, PDWARNING,
                "Failed to parse preferred instance [%s]",
                option.toString( TRUE, TRUE).c_str() ) ;

   done :
      PD_TRACE_EXITRC( SDB__RTNINST__PARSEARRPREFINST, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNINST_PARSEPREFINST, "_rtnInstanceOption::parsePreferredInstance" )
   INT32 _rtnInstanceOption::parsePreferredInstance ( const CHAR * instanceStr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNINST_PARSEPREFINST ) ;

      CHAR instanceCopyStr [ PMD_MAX_LONG_STR_LEN + 1 ] = { '\0' } ;
      CHAR * curInstanceStr = NULL ;
      CHAR * lastParsed = NULL ;

      PD_CHECK( NULL != instanceStr, SDB_INVALIDARG, error, PDWARNING,
                "Failed to parse preferred instance: empty input string" ) ;

      _clearInstance() ;

      ossStrncpy( instanceCopyStr, instanceStr, PMD_MAX_LONG_STR_LEN ) ;
      instanceCopyStr[ PMD_MAX_LONG_STR_LEN ] = '\0' ;

      // Split the preferred instance string by ','
      curInstanceStr = ossStrtok( instanceCopyStr, ",", &lastParsed ) ;
      while ( NULL != curInstanceStr && '\0' != curInstanceStr )
      {
         if ( 0 == ossStrcasecmp( curInstanceStr,
                                  PREFER_INSTANCE_MASTER_STR ) &&
              PREFER_INSTANCE_TYPE_UNKNOWN == _specInstance )
         {
            _specInstance = PREFER_INSTANCE_TYPE_MASTER ;
         }
         else if ( 0 == ossStrcasecmp( curInstanceStr,
                                       PREFER_INSTANCE_SLAVE_STR ) &&
                   PREFER_INSTANCE_TYPE_UNKNOWN == _specInstance )
         {
            _specInstance = PREFER_INSTANCE_TYPE_SLAVE ;
         }
         else if ( 0 == ossStrcasecmp( curInstanceStr,
                                       PREFER_INSTANCE_ANY_STR ) &&
                   PREFER_INSTANCE_TYPE_UNKNOWN == _specInstance )
         {
            _specInstance = PREFER_INSTANCE_TYPE_ANYONE ;
         }
         else if ( 0 == ossStrcasecmp( curInstanceStr,
                                       PREFER_INSTANCE_MASTER_SND_STR ) &&
                   PREFER_INSTANCE_TYPE_UNKNOWN == _specInstance )
         {
            _specInstance = PREFER_INSTANCE_TYPE_MASTER_SND ;
         }
         else if ( 0 == ossStrcasecmp( curInstanceStr,
                                       PREFER_INSTANCE_SLAVE_SND_STR ) &&
                   PREFER_INSTANCE_TYPE_UNKNOWN == _specInstance )
         {
            _specInstance = PREFER_INSTANCE_TYPE_SLAVE_SND ;
         }
         else if ( 0 == ossStrcasecmp( curInstanceStr,
                                       PREFER_INSTANCE_ANY_SND_STR ) &&
                   PREFER_INSTANCE_TYPE_UNKNOWN == _specInstance )
         {
            _specInstance = PREFER_INSTANCE_TYPE_ANYONE_SND ;
         }
         else
         {
            INT32 instance = ossAtoi( curInstanceStr ) ;
            if ( instance > PREFER_INSTANCE_TYPE_MIN &&
                 instance < PREFER_INSTANCE_TYPE_MAX )
            {
               _instanceList.push_back( (UINT8)instance ) ;
            }
            else
            {
               PD_LOG( PDWARNING, "Unknown preferred instance : [%s]",
                       curInstanceStr ) ;
            }
         }
         curInstanceStr = ossStrtok( lastParsed, ",", &lastParsed ) ;
      }

      PD_CHECK( PREFER_INSTANCE_TYPE_UNKNOWN != _specInstance ||
                !_instanceList.empty(), SDB_INVALIDARG, error, PDWARNING,
                "Failed to parse preferred instance [%s]", instanceStr ) ;

   done :
      PD_TRACE_EXITRC( SDB__RTNINST_PARSEPREFINST, rc ) ;
      return rc ;

   error :
      _clearInstance() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNINST__PARSEPREFINSTMODE, "_rtnInstanceOption::parsePreferredInstanceMode" )
   INT32 _rtnInstanceOption::parsePreferredInstanceMode ( const CHAR * instanceModeStr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNINST__PARSEPREFINSTMODE ) ;

      _mode = (UINT8)PREFER_INSTANCE_MODE_UNKNOWN ;

      PD_CHECK( NULL != instanceModeStr, SDB_INVALIDARG, error, PDWARNING,
                "Failed to parse preferred instance mode: "
                "empty input string" ) ;

      if ( 0 == ossStrcmp( instanceModeStr, PREFER_INSTANCE_RANDOM_STR ) )
      {
         _mode = (UINT8)PREFER_INSTANCE_MODE_RANDOM ;
      }
      else if ( 0 == ossStrcmp( instanceModeStr, PREFER_INSTANCE_ORDERED_STR ) )
      {
         _mode = (UINT8)PREFER_INSTANCE_MODE_ORDERED ;
      }

      PD_CHECK( PREFER_INSTANCE_MODE_UNKNOWN != _mode,
                SDB_INVALIDARG, error, PDWARNING,
                "Failed to parse preferred instance mode: "
                "unknown input string [%s]", instanceModeStr ) ;

   done :
      PD_TRACE_EXITRC( SDB__RTNINST__PARSEPREFINSTMODE, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   void _rtnInstanceOption::setPreferredStrict( BOOLEAN strict )
   {
      _strict = strict ? 1 : 0 ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNINST_TOBSON, "_rtnInstanceOption::toBSON" )
   void _rtnInstanceOption::toBSON ( BSONObjBuilder & builder ) const
   {
      if ( isValidated() )
      {
         const CHAR * modeStr = NULL ;
         if ( _instanceList.empty() )
         {
            builder.append( FIELD_NAME_PREFERED_INSTANCE,
                            _getInstanceStr( _specInstance ) ) ;
         }
         else if ( _instanceList.size() == 1 &&
                   PREFER_INSTANCE_TYPE_UNKNOWN == _specInstance )
         {
            builder.append( FIELD_NAME_PREFERED_INSTANCE,
                            (INT32)_instanceList.front() ) ;
         }
         else
         {
            BSONArrayBuilder instanceBuilder(
                  builder.subarrayStart( FIELD_NAME_PREFERED_INSTANCE ) ) ;
            for ( RTN_INSTANCE_LIST::const_iterator iter = _instanceList.begin() ;
                  iter != _instanceList.end() ;
                  iter ++ )
            {
               instanceBuilder.append( (INT32)( *iter ) ) ;
            }
            if ( PREFER_INSTANCE_TYPE_UNKNOWN != _specInstance )
            {
               instanceBuilder.append( _getInstanceStr( _specInstance ) ) ;
            }
            instanceBuilder.doneFast() ;
         }
         switch ( _mode )
         {
            case PREFER_INSTANCE_MODE_RANDOM :
               modeStr = PREFER_INSTANCE_RANDOM_STR ;
               break ;
            case PREFER_INSTANCE_MODE_ORDERED :
               modeStr = PREFER_INSTANCE_ORDERED_STR ;
               break ;
            default :
               modeStr = PREFER_INSTANCE_RANDOM_STR ;
               break ;
         }
         builder.append( FIELD_NAME_PREFERED_INSTANCE_MODE, modeStr ) ;
         builder.appendBool( FIELD_NAME_PREFERED_STRICT, _strict ) ;
         builder.append( FIELD_NAME_PREFERED_PERIOD, _period ) ;
      }
      else
      {
         // Invalid options, use the default one
         builder.append( FIELD_NAME_PREFERED_INSTANCE,
                         PREFER_INSTANCE_MASTER_STR ) ;
         builder.append( FIELD_NAME_PREFERED_INSTANCE_MODE,
                         PREFER_INSTANCE_RANDOM_STR ) ;
      }
   }

   const CHAR * _rtnInstanceOption::_getInstanceStr ( INT8 instance ) const
   {
      switch ( instance )
      {
         case PREFER_INSTANCE_TYPE_MASTER :
            return PREFER_INSTANCE_MASTER_STR ;
         case PREFER_INSTANCE_TYPE_MASTER_SND :
            return PREFER_INSTANCE_MASTER_SND_STR ;
         case PREFER_INSTANCE_TYPE_SLAVE :
            return PREFER_INSTANCE_SLAVE_STR ;
         case PREFER_INSTANCE_TYPE_SLAVE_SND :
            return PREFER_INSTANCE_SLAVE_SND_STR ;
         case PREFER_INSTANCE_TYPE_ANYONE :
            return PREFER_INSTANCE_ANY_STR ;
         case PREFER_INSTANCE_TYPE_ANYONE_SND :
            return PREFER_INSTANCE_ANY_SND_STR ;
         default :
            break ;
      }
      return "Unknown" ;
   }

   void _rtnInstanceOption::_clearInstance ()
   {
      _specInstance = (INT8)PREFER_INSTANCE_TYPE_UNKNOWN ;
      _instanceList.clear() ;
   }

   /*
      _rtnSessionProperty implement
    */
   _rtnSessionProperty::_rtnSessionProperty ()
   : _instanceOption(),
     _operationTimeout( RTN_SESSION_OPERATION_TIMEOUT_MAX )
   {
   }

   _rtnSessionProperty::_rtnSessionProperty ( const rtnSessionProperty & property )
   : _instanceOption( property._instanceOption ),
     _operationTimeout( property._operationTimeout )
   {
   }

   _rtnSessionProperty::~_rtnSessionProperty ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSESSPROP_SETINSTOPT, "_rtnSessionProperty::setInstanceOption" )
   void _rtnSessionProperty::setInstanceOption ( const CHAR * instanceStr,
                                                 const CHAR * instanceModeStr,
                                                 BOOLEAN preferedStrict,
                                                 INT32 preferedPeriod,
                                                 RTN_PREFER_INSTANCE_TYPE defaultInstance )
   {
      PD_TRACE_ENTRY( SDB__RTNSESSPROP_SETINSTOPT ) ;

      _instanceOption.parsePreferredInstance( instanceStr ) ;
      _instanceOption.parsePreferredInstanceMode( instanceModeStr ) ;
      _instanceOption.setPreferredStrict( preferedStrict ) ;
      _instanceOption.setPreferedPeriod( preferedPeriod ) ;

      if ( !_instanceOption.isValidated() )
      {
         _instanceOption.setPreferredInstance( defaultInstance ) ;

         // if no preferred instance is given, no need to check timeout period
         _instanceOption.setPreferedPeriod( -1 ) ;
      }

      if ( !_instanceOption.hasCommonInstance() )
      {
         _instanceOption.setPreferredStrict( FALSE ) ;
      }

      PD_TRACE_EXIT( SDB__RTNSESSPROP_SETINSTOPT ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSESSPROP_PARSEPROP, "_rtnSessionProperty::parseProperty" )
   INT32 _rtnSessionProperty::parseProperty ( const BSONObj & property )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNSESSPROP_PARSEPROP ) ;

      try
      {
         BOOLEAN newVersion = TRUE ;

         PD_CHECK( !property.isEmpty(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse session property: empty property" ) ;

         if ( property.hasField( FIELD_NAME_PREFERED_INSTANCE ) &&
              !property.hasField( FIELD_NAME_PREFERED_INSTANCE_V1 ) )
         {
            newVersion = FALSE ;
         }

         if ( newVersion )
         {
            rc = _parsePropertyV1( property ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse old version of "
                         "session property, rc: %d", rc ) ;
         }
         else
         {
            rc = _parsePropertyV0( property ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse old version of "
                         "session property, rc: %d", rc ) ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Failed to parse session property, "
                 "received unexpected error: %s", e.what() ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNSESSPROP_PARSEPROP, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   BSONObj _rtnSessionProperty::toBSON () const
   {
      BSONObjBuilder builder ;

      _instanceOption.toBSON( builder ) ;
      builder.append( FIELD_NAME_TIMEOUT, _operationTimeout ) ;

      _toBson( builder ) ;

      return builder.obj() ;
   }

   //PD_TRACE_DECLARE_FUNCTION( SDB__RTNSESSPROP__PARSEPROPV0, "_rtnSessionProperty::_parsePropertyV0" )
   INT32 _rtnSessionProperty::_parsePropertyV0 ( const BSONObj & property )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNSESSPROP__PARSEPROPV0 ) ;

      INT64 operationTimeout = -1 ;
      rtnInstanceOption instanceOption = getInstanceOption() ;

      BOOLEAN gotInstance = FALSE ;
      BOOLEAN gotOperationTimeout = FALSE ;

      BSONObjIterator iter( property ) ;
      while ( iter.more() )
      {
         BSONElement field = iter.next() ;

         if ( 0 == ossStrcmp( field.fieldName(), FIELD_NAME_PREFERED_INSTANCE ) )
         {
            /// PreferedInstance
            INT32 replType = PREFER_REPL_ANYONE ;

            PD_CHECK( field.type() == NumberInt, SDB_INVALIDARG, error,
                      PDERROR, "Field[%s] is not integer",
                      FIELD_NAME_PREFERED_INSTANCE ) ;

            replType = field.numberInt();
            PD_CHECK( replType > PREFER_REPL_TYPE_MIN &&
                      replType < PREFER_REPL_TYPE_MAX,
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to set preferedInstanace, invalid value[%d], "
                      "Value range:(%d~%d)", replType,
                      PREFER_REPL_TYPE_MIN, PREFER_REPL_TYPE_MAX ) ;

            rc = instanceOption.setPreferredInstance( (PREFER_REPLICA_TYPE)replType ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to set old version preferred "
                         "instance, rc: %d", rc ) ;

            gotInstance = TRUE ;
         }
         else if ( 0 == ossStrcmp( field.fieldName(), FIELD_NAME_TIMEOUT ) )
         {
            /// Timeout
            PD_CHECK( field.isNumber(), SDB_INVALIDARG, error,
                      PDERROR, "Field[%s] is not number",
                      FIELD_NAME_TIMEOUT ) ;

            operationTimeout = (INT64)field.numberLong() ;
            gotOperationTimeout = TRUE ;
         }
         else
         {
            PD_LOG( PDERROR, "Option [%s] is not supported in session property",
                    field.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }

      if ( gotInstance )
      {
         setInstanceOption( instanceOption ) ;
         _onSetInstance() ;
      }

      if ( gotOperationTimeout )
      {
         setOperationTimeout( operationTimeout ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNSESSPROP__PARSEPROPV0, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION( SDB__RTNSESSPROP__PARSEPROPV1, "_rtnSessionProperty::_parsePropertyV1" )
   INT32 _rtnSessionProperty::_parsePropertyV1 ( const BSONObj & property )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNSESSPROP__PARSEPROPV1 ) ;

      INT64 operationTimeout = -1 ;
      rtnInstanceOption instanceOption = getInstanceOption() ;

      BOOLEAN gotInstance = FALSE ;
      BOOLEAN gotOperationTimeout = FALSE ;
      dpsTransConfItem transConf ;
      const CHAR *pSource = NULL ;

      BSONObjIterator iter( property ) ;
      while ( iter.more() )
      {
         BSONElement field = iter.next() ;

         if ( 0 == ossStrcasecmp( field.fieldName(),
                                  FIELD_NAME_PREFERED_INSTANCE_V1 ) )
         {
            /// PreferedInstance
            rc = instanceOption.parsePreferredInstance( field ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse preferred instance, "
                         " rc: %d", rc ) ;
            PD_CHECK( instanceOption.isValidated(), SDB_INVALIDARG, error,
                      PDERROR, "Failed to parse preferred instance" ) ;

            gotInstance = TRUE ;
         }
         else if ( 0 == ossStrcasecmp( field.fieldName(),
                                       FIELD_NAME_PREFERED_INSTANCE_MODE ) )
         {
            /// PreferedInstanceMode
            PD_CHECK( String == field.type(), SDB_INVALIDARG, error,
                      PDERROR, "Field[%s] is not string",
                      FIELD_NAME_PREFERED_INSTANCE_MODE ) ;

            rc = instanceOption.parsePreferredInstanceMode(
               field.valuestrsafe() ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse preferred instance "
                         "mode, rc: %d", rc ) ;

            gotInstance = TRUE ;
         }
         else if ( 0 == ossStrcasecmp( field.fieldName(),
                                       FIELD_NAME_PREFERED_STRICT ) )
         {
            /// PreferedStrict
            PD_CHECK( Bool == field.type(), SDB_INVALIDARG, error,
                      PDERROR, "Field[%s] is not boolean",
                      FIELD_NAME_PREFERED_STRICT ) ;

            instanceOption.setPreferredStrict( field.boolean() ) ;
            gotInstance = TRUE ;
         }
         else if ( 0 == ossStrcasecmp( field.fieldName(),
                                       FIELD_NAME_PREFERED_PERIOD ) )
         {
            /// PreferedPeriod
            PD_CHECK( field.isNumber(), SDB_INVALIDARG, error, PDERROR,
                      "Field [%s] is not number", FIELD_NAME_PREFERED_PERIOD ) ;
            instanceOption.setPreferedPeriod( field.numberLong() ) ;
            gotInstance = TRUE ;
         }
         else if ( 0 == ossStrcasecmp( field.fieldName(), FIELD_NAME_TIMEOUT ) )
         {
            /// Timeout
            PD_CHECK( field.isNumber(), SDB_INVALIDARG, error,
                      PDERROR, "Field[%s] is not number",
                      FIELD_NAME_TIMEOUT ) ;

            operationTimeout = (INT64)field.numberLong() ;
            gotOperationTimeout = TRUE ;
         }
         else if ( 0 == ossStrcasecmp( field.fieldName(),
                                       FIELD_NAME_PREFERED_INSTANCE ) )
         {
            /// do nothing
         }
         else if ( 0 == ossStrcasecmp( field.fieldName(),
                                       FIELD_NAME_TRANSISOLATION ) )
         {
            PD_CHECK( field.isNumber(), SDB_INVALIDARG, error,
                      PDERROR, "Field[%s] is not number",
                      FIELD_NAME_TRANSISOLATION ) ;
            INT32 transIsolation = field.numberInt() ;
            if ( transIsolation < TRANS_ISOLATION_RU ||
                 transIsolation >= TRANS_ISOLATION_MAX )
            {
               PD_LOG( PDERROR, "Field[%s]'s value is invalid",
                       field.toString().c_str() ) ;
               goto error ;
            }
            transConf.setTransIsolation( transIsolation, TRUE ) ;
         }
         else if ( 0 == ossStrcasecmp( field.fieldName(),
                                       FIELD_NAME_TRANS_TIMEOUT ) )
         {
            PD_CHECK( field.isNumber(), SDB_INVALIDARG, error,
                      PDERROR, "Field[%s] is not number",
                      FIELD_NAME_TRANS_TIMEOUT ) ;
            INT32 transTimeout = field.numberInt() ;
            if ( transTimeout < 0 )
            {
               transTimeout = 0 ;
            }
            transConf.setTransTimeout( transTimeout * OSS_ONE_SEC, TRUE ) ;
         }
         else if ( 0 == ossStrcasecmp( field.fieldName(),
                                       FIELD_NAME_TRANS_WAITLOCK ) )
         {
            PD_CHECK( field.isBoolean(), SDB_INVALIDARG, error,
                      PDERROR, "Field[%s] is not boolean",
                      FIELD_NAME_TRANS_WAITLOCK ) ;
            transConf.setTransWaitLock( field.boolean() ? TRUE : FALSE,
                                        TRUE ) ;
         }
         else if ( 0 == ossStrcasecmp( field.fieldName(),
                                       FIELD_NAME_TRANS_USE_RBS ) )
         {
            PD_CHECK( field.isBoolean(), SDB_INVALIDARG, error,
                      PDERROR, "Field[%s] is not boolean",
                      FIELD_NAME_TRANS_USE_RBS ) ;
            transConf.setUseRollbackSemgent( field.boolean() ? TRUE : FALSE,
                                             TRUE ) ;
         }
         else if ( 0 == ossStrcasecmp( field.fieldName(),
                                       FIELD_NAME_TRANS_AUTOCOMMIT ) )
         {
            PD_CHECK( field.isBoolean(), SDB_INVALIDARG, error,
                      PDERROR, "Field[%s] is not boolean",
                      FIELD_NAME_TRANS_AUTOCOMMIT ) ;
            transConf.setTransAutoCommit( field.boolean() ? TRUE : FALSE,
                                          TRUE ) ;
         }
         else if ( 0 == ossStrcasecmp( field.fieldName(),
                                       FIELD_NAME_TRANS_AUTOROLLBACK ) )
         {
            PD_CHECK( field.isBoolean(), SDB_INVALIDARG, error,
                      PDERROR, "Field[%s] is not boolean",
                      FIELD_NAME_TRANS_AUTOROLLBACK ) ;
            transConf.setTransAutoRollback( field.boolean() ? TRUE : FALSE,
                                            TRUE ) ;
         }
         else if ( 0 == ossStrcasecmp( field.fieldName(),
                                       FIELD_NAME_TRANS_RCCOUNT ) )
         {
            PD_CHECK( field.isBoolean(), SDB_INVALIDARG, error,
                      PDERROR, "Field[%s] is not boolean",
                      FIELD_NAME_TRANS_RCCOUNT ) ;
            transConf.setTransRCCount( field.boolean() ? TRUE : FALSE,
                                       TRUE ) ;

         }
         else if ( 0 == ossStrcasecmp( field.fieldName(), FIELD_NAME_SOURCE ) )
         {
            PD_CHECK( String == field.type(), SDB_INVALIDARG, error,
                      PDERROR, "Field[%s] is not string",
                      FIELD_NAME_SOURCE ) ;
            pSource = field.valuestr() ;
         }
         else
         {
            PD_LOG( PDERROR, "Option [%s] is not supported in session property",
                    field.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }

      if ( pSource )
      {
         rc = _checkSource( pSource ) ;
         if ( rc )
         {
            goto error ;
         }
      }

      if ( transConf.getTransConfMask() )
      {
         rc = _checkTransConf( &transConf ) ;
         if ( rc )
         {
            goto error ;
         }
         /// update trans conf
         _updateTransConf( &transConf ) ;
      }

      if ( gotInstance )
      {
         if ( !instanceOption.hasCommonInstance() )
         {
            instanceOption.setPreferredStrict( FALSE ) ;
         }
         if ( !instanceOption.isValidated() )
         {
            // if no preferred instance is given, no need to check
            // timeout period
            _instanceOption.setPreferedPeriod( -1 ) ;
         }
         setInstanceOption( instanceOption ) ;
         _onSetInstance() ;
      }

      if ( gotOperationTimeout )
      {
         setOperationTimeout( operationTimeout ) ;
      }

      if ( pSource )
      {
         _updateSource( pSource ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNSESSPROP__PARSEPROPV1, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   INT32 _rtnSessionProperty::_checkTransConf( const _dpsTransConfItem *pTransConf )
   {
      return SDB_OK ;
   }

   INT32 _rtnSessionProperty::_checkSource( const CHAR *pSource )
   {
      return SDB_OK ;
   }

   void _rtnSessionProperty::_updateTransConf( const _dpsTransConfItem *pTransConf )
   {
   }

   void _rtnSessionProperty::_updateSource( const CHAR *pSource )
   {
   }

}
