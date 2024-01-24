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
   : _mode( ( UINT8 )PMD_PREFER_INSTANCE_MODE_UNKNOWN ),
     _constraint( ( UINT8 )PMD_PREFER_CONSTRAINT_UNKNOWN ),
     _strict( 0 ),
     _specInstance( ( INT8 )PMD_PREFER_INSTANCE_TYPE_UNKNOWN ),
     _period( PREFER_INSTANCE_DEF_PERIOD ),
     _instanceList()
   {
   }

   _rtnInstanceOption::_rtnInstanceOption ( const rtnInstanceOption & option )
   : _mode( option._mode ),
     _constraint( ( UINT8 )option._constraint ),
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
      _constraint = option._constraint ;
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

      _mode = ( UINT8 )PMD_PREFER_INSTANCE_MODE_UNKNOWN ;
      _constraint = ( UINT8 )PMD_PREFER_CONSTRAINT_UNKNOWN ;
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
            _specInstance = ( INT8 )PMD_PREFER_INSTANCE_TYPE_MASTER ;
            break ;
         case PREFER_REPL_SLAVE :
            _specInstance = ( INT8 )PMD_PREFER_INSTANCE_TYPE_SLAVE ;
            break ;
         case PREFER_REPL_ANYONE :
            _specInstance = ( INT8 )PMD_PREFER_INSTANCE_TYPE_ANYONE ;
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
   INT32 _rtnInstanceOption::setPreferredInstance ( PMD_PREFER_INSTANCE_TYPE instance )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNINST_SETPREFINST ) ;

      if ( instance > PMD_PREFER_INSTANCE_TYPE_MIN &&
           instance < PMD_PREFER_INSTANCE_TYPE_MAX )
      {
         _instanceList.push_back( (UINT8)instance ) ;
      }
      else if ( PMD_PREFER_INSTANCE_TYPE_MASTER == instance ||
                PMD_PREFER_INSTANCE_TYPE_SLAVE == instance ||
                PMD_PREFER_INSTANCE_TYPE_ANYONE == instance ||
                PMD_PREFER_INSTANCE_TYPE_MASTER_SND == instance ||
                PMD_PREFER_INSTANCE_TYPE_SLAVE_SND == instance ||
                PMD_PREFER_INSTANCE_TYPE_ANYONE_SND == instance )
      {
         _specInstance = (INT8)instance ;
      }

      PD_TRACE_EXITRC( SDB__RTNINST_SETPREFINST, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNINST_SETPREFINSTMODE, "_rtnInstanceOption::setPreferredInstanceMode" )
   INT32 _rtnInstanceOption::setPreferredInstanceMode ( PMD_PREFER_INSTANCE_MODE mode )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNINST_SETPREFINSTMODE ) ;

      _mode = (UINT8)mode ;

      PD_TRACE_EXITRC( SDB__RTNINST_SETPREFINSTMODE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNINST_SETPREFCONSTRNT, "_rtnInstanceOption::setPreferredConstraint" )
   INT32 _rtnInstanceOption::setPreferredConstraint( PMD_PREFER_CONSTRAINT constraint )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNINST_SETPREFCONSTRNT ) ;

      _constraint = (UINT8)constraint ;

      PD_TRACE_EXITRC( SDB__RTNINST_SETPREFCONSTRNT, rc ) ;

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
            PD_LOG_MSG( PDERROR, "Failed to parse preferred instance: "
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

      PD_LOG_MSG_CHECK( prefInst > PMD_PREFER_INSTANCE_TYPE_MIN &&
                        prefInst < PMD_PREFER_INSTANCE_TYPE_MAX,
                        SDB_INVALIDARG, error, PDERROR, "Failed to parse "
                        "preferred instance: id[%d] is out of range [%d, %d]",
                        prefInst, PMD_PREFER_INSTANCE_TYPE_MIN + 1,
                        PMD_PREFER_INSTANCE_TYPE_MAX - 1 ) ;

      // Remove duplicate instance id.
      _instanceList.remove( (UINT8)prefInst );
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
         _specInstance = ( INT8 )PMD_PREFER_INSTANCE_TYPE_MASTER ;
      }
      else if ( 0 == ossStrcasecmp( instanceStr,
                                    PREFER_INSTANCE_SLAVE_STR ) )
      {
         _specInstance = ( INT8 )PMD_PREFER_INSTANCE_TYPE_SLAVE ;
      }
      else if ( 0 == ossStrcasecmp( instanceStr,
                                    PREFER_INSTANCE_ANY_STR ) )
      {
         _specInstance = ( INT8 )PMD_PREFER_INSTANCE_TYPE_ANYONE ;
      }
      else if ( 0 == ossStrcasecmp( instanceStr,
                                    PREFER_INSTANCE_MASTER_SND_STR ) )
      {
         _specInstance = ( INT8 )PMD_PREFER_INSTANCE_TYPE_MASTER_SND ;
      }
      else if ( 0 == ossStrcasecmp( instanceStr,
                                    PREFER_INSTANCE_SLAVE_SND_STR ) )
      {
         _specInstance = ( INT8 )PMD_PREFER_INSTANCE_TYPE_SLAVE_SND ;
      }
      else if ( 0 == ossStrcasecmp( instanceStr,
                                    PREFER_INSTANCE_ANY_SND_STR ) )
      {
         _specInstance = ( INT8 )PMD_PREFER_INSTANCE_TYPE_ANYONE_SND ;
      }
      else
      {
         _specInstance = ( INT8 )PMD_PREFER_INSTANCE_TYPE_UNKNOWN ;
      }

      PD_LOG_MSG_CHECK( PMD_PREFER_INSTANCE_TYPE_UNKNOWN != _specInstance,
                        SDB_INVALIDARG, error, PDERROR,
                        "Failed to parse preferred instance: "
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
            rc = _parseIntegerPreferredInstance( curOption ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse integer option of "
                         "preferred instance, rc: %d", rc ) ;
         }
         else if ( String == curOption.type() )
         {
            PD_LOG_MSG_CHECK( PMD_PREFER_INSTANCE_TYPE_UNKNOWN == _specInstance,
                              SDB_INVALIDARG, error, PDERROR,
                              "More than one preferred instance type exists") ;
            rc = _parseStringPreferredInstance( curOption ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse string option of "
                         "preferred instance, rc: %d", rc ) ;
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
      BOOLEAN hasInvalidChar = FALSE ;
      PMD_PREFER_INSTANCE_TYPE specInstanceTmp = PMD_PREFER_INSTANCE_TYPE_UNKNOWN ;

      PD_TRACE_ENTRY( SDB__RTNINST_PARSEPREFINST ) ;

      PD_CHECK( NULL != instanceStr, SDB_INVALIDARG, error, PDWARNING,
                "Failed to parse preferred instance: empty input string" ) ;

      _clearInstance() ;
      rc = pmdParsePreferInstStr( instanceStr, _instanceList, specInstanceTmp,
                                  hasInvalidChar ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to parse preferd instance str, rc: %d", rc ) ;
      _specInstance = ( INT8 )specInstanceTmp ;

      PD_CHECK( PMD_PREFER_INSTANCE_TYPE_UNKNOWN != _specInstance ||
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
   INT32 _rtnInstanceOption::parsePreferredInstanceMode ( const CHAR *instanceModeStr )
   {
      INT32 rc = SDB_OK ;
      PMD_PREFER_INSTANCE_MODE modeTmp = PMD_PREFER_INSTANCE_MODE_UNKNOWN ;

      PD_TRACE_ENTRY( SDB__RTNINST__PARSEPREFINSTMODE ) ;

      rc = pmdParsePreferInstModeStr( instanceModeStr, modeTmp ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to parse preferd instance mode str, rc: %d", rc ) ;
      _mode = ( UINT8 )modeTmp ;

      PD_CHECK( PMD_PREFER_INSTANCE_MODE_UNKNOWN != _mode,
                SDB_INVALIDARG, error, PDWARNING,
                "Failed to parse preferred instance mode: "
                "unknown input string [%s]", instanceModeStr ) ;

   done :
      PD_TRACE_EXITRC( SDB__RTNINST__PARSEPREFINSTMODE, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNINST__PARSEPREFCONSTRAINT, "_rtnInstanceOption::parsePreferredConstraint" )
   INT32 _rtnInstanceOption::parsePreferredConstraint ( const CHAR *constraintStr )
   {
      INT32 rc = SDB_OK ;
      PMD_PREFER_CONSTRAINT constraintTmp = PMD_PREFER_CONSTRAINT_UNKNOWN ;

      PD_TRACE_ENTRY( SDB__RTNINST__PARSEPREFCONSTRAINT ) ;

      rc = pmdParsePreferConstraintStr( constraintStr, constraintTmp ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to parse preferred constraint str, rc: %d", rc ) ;
      _constraint = ( UINT8 )constraintTmp ;

      PD_CHECK( PMD_PREFER_CONSTRAINT_UNKNOWN != _constraint,
                SDB_INVALIDARG, error, PDWARNING,
                "Failed to parse preferred constraint: "
                "unknown input string [%s]", constraintStr ) ;

   done :
      PD_TRACE_EXITRC( SDB__RTNINST__PARSEPREFCONSTRAINT, rc ) ;
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
      // TODO: "Prefered*" is fixed as "Preferred*". But for compatiblility,
      // show both of them. "Prefered" to be removed in future.

      if ( isValidated() )
      {
         const CHAR * modeStr = NULL ;
         const CHAR * constraintStr = NULL ;
         if ( _instanceList.empty() )
         {
            const CHAR * value = pmdPreferInstInt2String(
                  ( PMD_PREFER_INSTANCE_TYPE )_specInstance ) ;
            builder.append( FIELD_NAME_PREFERRED_INSTANCE_LEGACY, value ) ;
            builder.append( FIELD_NAME_PREFERRED_INSTANCE, value ) ;
         }
         else if ( _instanceList.size() == 1 &&
                   PMD_PREFER_INSTANCE_TYPE_UNKNOWN == _specInstance )
         {
            INT32 value = ( INT32 )_instanceList.front() ;
            builder.append( FIELD_NAME_PREFERRED_INSTANCE_LEGACY, value ) ;
            builder.append( FIELD_NAME_PREFERRED_INSTANCE, value ) ;
         }
         else
         {
            BSONArrayBuilder instanceBuilder(
                  builder.subarrayStart( FIELD_NAME_PREFERRED_INSTANCE_LEGACY ) ) ;
            for ( RTN_INSTANCE_LIST::const_iterator iter = _instanceList.begin() ;
                  iter != _instanceList.end() ;
                  iter ++ )
            {
               instanceBuilder.append( ( INT32 )( *iter ) ) ;
            }
            if ( PMD_PREFER_INSTANCE_TYPE_UNKNOWN != _specInstance )
            {
               instanceBuilder.append( pmdPreferInstInt2String(
                               ( PMD_PREFER_INSTANCE_TYPE )_specInstance ) ) ;
            }
            instanceBuilder.doneFast() ;

            BSONArrayBuilder instBuilder(
                  builder.subarrayStart( FIELD_NAME_PREFERRED_INSTANCE ) ) ;
            for ( RTN_INSTANCE_LIST::const_iterator iter = _instanceList.begin() ;
                  iter != _instanceList.end() ;
                  iter ++ )
            {
               instBuilder.append( ( INT32 )( *iter ) ) ;
            }
            if ( PMD_PREFER_INSTANCE_TYPE_UNKNOWN != _specInstance )
            {
               instBuilder.append( pmdPreferInstInt2String(
                               ( PMD_PREFER_INSTANCE_TYPE )_specInstance ) ) ;
            }
            instBuilder.doneFast() ;
         }
         switch ( _mode )
         {
            case PMD_PREFER_INSTANCE_MODE_RANDOM :
               modeStr = PREFER_INSTANCE_RANDOM_STR ;
               break ;
            case PMD_PREFER_INSTANCE_MODE_ORDERED :
               modeStr = PREFER_INSTANCE_ORDERED_STR ;
               break ;
            default :
               modeStr = PREFER_INSTANCE_RANDOM_STR ;
               break ;
         }
         builder.append( FIELD_NAME_PREFERRED_INSTANCE_MODE_LEGACY, modeStr ) ;
         builder.append( FIELD_NAME_PREFERRED_INSTANCE_MODE, modeStr ) ;
         builder.appendBool( FIELD_NAME_PREFERRED_STRICT_LEGACY, _strict ) ;
         builder.appendBool( FIELD_NAME_PREFERRED_STRICT, _strict ) ;
         builder.append( FIELD_NAME_PREFERRED_PERIOD_LEGACY, _period ) ;
         builder.append( FIELD_NAME_PREFERRED_PERIOD, _period ) ;
         switch ( _constraint )
         {
            case PMD_PREFER_CONSTRAINT_PRY_ONLY :
               constraintStr = PREFER_CONSTRAINT_PRY_ONLY_STR ;
               break ;
            case PMD_PREFER_CONSTRAINT_SND_ONLY :
               constraintStr = PREFER_CONSTRAINT_SND_ONLY_STR ;
               break ;
            default :
               constraintStr = "" ;
         }
         builder.append( FIELD_NAME_PREFERRED_CONSTRAINT, constraintStr ) ;
      }
      else
      {
         // Invalid options, use the default one
         builder.append( FIELD_NAME_PREFERRED_INSTANCE_LEGACY,
                         PREFER_INSTANCE_MASTER_STR ) ;
         builder.append( FIELD_NAME_PREFERRED_INSTANCE,
                         PREFER_INSTANCE_MASTER_STR ) ;
         builder.append( FIELD_NAME_PREFERRED_INSTANCE_MODE_LEGACY,
                         PREFER_INSTANCE_RANDOM_STR ) ;
         builder.append( FIELD_NAME_PREFERRED_INSTANCE_MODE,
                         PREFER_INSTANCE_RANDOM_STR ) ;
      }
   }

   void _rtnInstanceOption::_clearInstance ()
   {
      _specInstance = ( INT8 )PMD_PREFER_INSTANCE_TYPE_UNKNOWN ;
      _instanceList.clear() ;
   }

   /*
      _rtnSessionProperty implement
    */
   _rtnSessionProperty::_rtnSessionProperty ()
   : _instanceOption(),
     _operationTimeout( RTN_SESSION_OPERATION_TIMEOUT_MAX ),
     _needCheckVer( FALSE ),
     _version( 1 )
   {
   }

   _rtnSessionProperty::_rtnSessionProperty ( const rtnSessionProperty & property )
   : _instanceOption( property._instanceOption ),
     _operationTimeout( property._operationTimeout ),
     _needCheckVer( FALSE ),
     _version( 1 )
   {
   }

   _rtnSessionProperty::~_rtnSessionProperty ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSESSPROP_SETINSTOPT, "_rtnSessionProperty::setInstanceOption" )
   void _rtnSessionProperty::setInstanceOption ( const CHAR * instanceStr,
                                                 const CHAR * instanceModeStr,
                                                 BOOLEAN preferredStrict,
                                                 INT32 preferredPeriod,
                                                 const CHAR * preferredConstraint,
                                                 PMD_PREFER_INSTANCE_TYPE defaultInstance )
   {
      PD_TRACE_ENTRY( SDB__RTNSESSPROP_SETINSTOPT ) ;

      _instanceOption.parsePreferredInstance( instanceStr ) ;
      _instanceOption.parsePreferredInstanceMode( instanceModeStr ) ;
      _instanceOption.setPreferredStrict( preferredStrict ) ;
      _instanceOption.setPreferedPeriod( preferredPeriod ) ;
      _instanceOption.parsePreferredConstraint( preferredConstraint ) ;

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

         if ( property.hasField( FIELD_NAME_PREFERRED_INSTANCE_LEGACY ) &&
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
         ++_version ;
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

         if ( 0 == ossStrcmp( field.fieldName(),
                              FIELD_NAME_PREFERRED_INSTANCE_LEGACY ) )
         {
            /// PreferedInstance
            INT32 replType = PREFER_REPL_ANYONE ;

            PD_CHECK( field.type() == NumberInt, SDB_INVALIDARG, error,
                      PDERROR, "Field[%s] is not integer",
                      FIELD_NAME_PREFERRED_INSTANCE_LEGACY ) ;

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
      BOOLEAN needCheckCatVer = FALSE;

      BOOLEAN gotInstance = FALSE ;
      BOOLEAN gotNeedCheckCatVer = FALSE ;
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
                                       FIELD_NAME_PREFERRED_INSTANCE_MODE ) ||
                   0 == ossStrcasecmp( field.fieldName(),
                                       FIELD_NAME_PREFERRED_INSTANCE_MODE_LEGACY ) )
         {
            /// PreferedInstanceMode
            PD_LOG_MSG_CHECK( String == field.type(), SDB_INVALIDARG, error,
                              PDERROR, "Field [%s] should be a string",
                              field.fieldName() ) ;

            rc = instanceOption.parsePreferredInstanceMode(
               field.valuestrsafe() ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse preferred instance "
                         "mode, rc: %d", rc ) ;

            gotInstance = TRUE ;
         }
         else if ( 0 == ossStrcasecmp( field.fieldName(),
                                       FIELD_NAME_PREFERRED_STRICT ) ||
                   0 == ossStrcasecmp( field.fieldName(),
                                       FIELD_NAME_PREFERRED_STRICT_LEGACY ) )
         {
            /// PreferedStrict
            PD_LOG_MSG_CHECK( Bool == field.type(), SDB_INVALIDARG, error,
                              PDERROR, "Field [%s] should be a boolean",
                              field.fieldName() ) ;

            instanceOption.setPreferredStrict( field.boolean() ) ;
            gotInstance = TRUE ;
         }
         else if ( 0 == ossStrcasecmp( field.fieldName(),
                                       FIELD_NAME_PREFERRED_PERIOD ) ||
                   0 == ossStrcasecmp( field.fieldName(),
                                       FIELD_NAME_PREFERRED_PERIOD_LEGACY ) )
         {
            /// PreferedPeriod
            PD_LOG_MSG_CHECK( field.isNumber(), SDB_INVALIDARG, error, PDERROR,
                              "Field [%s] should be a number",
                              field.fieldName() ) ;
            instanceOption.setPreferedPeriod( field.numberLong() ) ;
            gotInstance = TRUE ;
         }
         else if ( 0 == ossStrcasecmp( field.fieldName(),
                                       FIELD_NAME_PREFERRED_CONSTRAINT ) )
         {
            /// PreferredConstraint
            PD_CHECK( String == field.type(), SDB_INVALIDARG, error,
                      PDERROR, "Field [%s] should be a string",
                      FIELD_NAME_PREFERRED_CONSTRAINT ) ;

            rc = instanceOption.parsePreferredConstraint(
                  field.valuestrsafe() ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to parse preferred constraint, rc: %d", rc ) ;

            gotInstance = TRUE ;
         }
         else if ( 0 == ossStrcasecmp( field.fieldName(), FIELD_NAME_TIMEOUT ) )
         {
            /// Timeout
            PD_LOG_MSG_CHECK( field.isNumber(), SDB_INVALIDARG, error,
                              PDERROR, "Field [%s] should be a number",
                              field.fieldName() ) ;

            operationTimeout = (INT64)field.numberLong() ;
            gotOperationTimeout = TRUE ;
         }
         else if ( 0 == ossStrcasecmp( field.fieldName(),
                                       FIELD_NAME_PREFERRED_INSTANCE ) ||
                   0 == ossStrcasecmp( field.fieldName(),
                                       FIELD_NAME_PREFERRED_INSTANCE_LEGACY ) )
         {
            /// do nothing
         }
         else if ( 0 == ossStrcasecmp( field.fieldName(),
                                       FIELD_NAME_TRANSISOLATION ) )
         {
            PD_LOG_MSG_CHECK( field.isNumber(), SDB_INVALIDARG, error,
                              PDERROR, "Field [%s] should be a number",
                              field.fieldName() ) ;
            INT32 transIsolation = field.numberInt() ;
            if ( transIsolation < TRANS_ISOLATION_RU ||
                 transIsolation >= TRANS_ISOLATION_MAX )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Field [%s]'s value is invalid, rc: %d",
                       field.toString().c_str(), rc ) ;
               goto error ;
            }
            transConf.setTransIsolation( transIsolation, TRUE ) ;
         }
         else if ( 0 == ossStrcasecmp( field.fieldName(),
                                       FIELD_NAME_TRANS_TIMEOUT ) )
         {
            PD_LOG_MSG_CHECK( field.isNumber(), SDB_INVALIDARG, error,
                              PDERROR, "Field [%s] should be a number",
                              field.fieldName() ) ;
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
            PD_LOG_MSG_CHECK( field.isBoolean(), SDB_INVALIDARG, error,
                              PDERROR, "Field [%s] should be a boolean",
                              field.fieldName() ) ;
            transConf.setTransWaitLock( field.boolean() ? TRUE : FALSE,
                                        TRUE ) ;
         }
         else if ( 0 == ossStrcasecmp( field.fieldName(),
                                       FIELD_NAME_TRANS_USE_RBS ) )
         {
            PD_LOG_MSG_CHECK( field.isBoolean(), SDB_INVALIDARG, error,
                              PDERROR, "Field [%s] should be a boolean",
                              field.fieldName() ) ;
            transConf.setUseRollbackSemgent( field.boolean() ? TRUE : FALSE,
                                             TRUE ) ;
         }
         else if ( 0 == ossStrcasecmp( field.fieldName(),
                                       FIELD_NAME_TRANS_AUTOCOMMIT ) )
         {
            PD_LOG_MSG_CHECK( field.isBoolean(), SDB_INVALIDARG, error,
                              PDERROR, "Field [%s] should be a boolean",
                              field.fieldName() ) ;
            transConf.setTransAutoCommit( field.boolean() ? TRUE : FALSE,
                                          TRUE ) ;
         }
         else if ( 0 == ossStrcasecmp( field.fieldName(),
                                       FIELD_NAME_TRANS_AUTOROLLBACK ) )
         {
            PD_LOG_MSG_CHECK( field.isBoolean(), SDB_INVALIDARG, error,
                              PDERROR, "Field [%s] should be a boolean",
                              field.fieldName() ) ;
            transConf.setTransAutoRollback( field.boolean() ? TRUE : FALSE,
                                            TRUE ) ;
         }
         else if ( 0 == ossStrcasecmp( field.fieldName(),
                                       FIELD_NAME_TRANS_RCCOUNT ) )
         {
            PD_LOG_MSG_CHECK( field.isBoolean(), SDB_INVALIDARG, error,
                              PDERROR, "Field [%s] should be a boolean",
                              field.fieldName() ) ;
            transConf.setTransRCCount( field.boolean() ? TRUE : FALSE,
                                       TRUE ) ;

         }
         else if ( 0 == ossStrcasecmp( field.fieldName(),
                                       FIELD_NAME_TRANS_ALLOWLOCKESCALATION ) )
         {
            PD_LOG_MSG_CHECK( field.isBoolean(), SDB_INVALIDARG, error, PDERROR,
                              "Field [%s] should be a boolean",
                              field.fieldName() ) ;
            transConf.setTransAllowLockEscalation(
                  field.boolean() ? TRUE : FALSE, TRUE ) ;
         }
         else if ( 0 == ossStrcasecmp( field.fieldName(),
                                       FIELD_NAME_TRANS_MAXLOCKNUM ) )
         {
            INT64 temp = 0 ;

            PD_LOG_MSG_CHECK( field.isNumber(), SDB_INVALIDARG, error, PDERROR,
                              "Field [%s] should be a boolean",
                              field.fieldName() ) ;
            temp = field.numberLong() ;

            // auto adjust
            if ( temp < DPS_TRANS_MAXLOCKNUM_MIN )
            {
               temp = DPS_TRANS_MAXLOCKNUM_MIN ;
            }
            else if ( temp > DPS_TRANS_MAXLOCKNUM_MAX )
            {
               temp = DPS_TRANS_MAXLOCKNUM_MAX ;
            }

            transConf.setTransMaxLockNum( (INT32)temp, TRUE ) ;
         }
         else if ( 0 == ossStrcasecmp( field.fieldName(),
                                       FIELD_NAME_TRANS_MAXLOGSPACERATIO ) )
         {
            INT64 temp = 0 ;

            PD_LOG_MSG_CHECK( field.isNumber(), SDB_INVALIDARG, error, PDERROR,
                              "Field [%s] should be a number",
                              field.fieldName() ) ;
            temp = field.numberLong() ;

            // auto adjust
            if ( temp < DPS_TRANS_MAXLOGSPACERATIO_MIN )
            {
               temp = DPS_TRANS_MAXLOGSPACERATIO_MIN ;
            }
            else if ( temp > DPS_TRANS_MAXLOGSPACERATIO_MAX )
            {
               temp = DPS_TRANS_MAXLOGSPACERATIO_MAX ;
            }

            transConf.setTransMaxLogSpaceRatio( (INT32)temp, TRUE ) ;
         }
         else if ( 0 == ossStrcasecmp( field.fieldName(), FIELD_NAME_SOURCE ) )
         {
            PD_LOG_MSG_CHECK( String == field.type(), SDB_INVALIDARG, error,
                              PDERROR, "Field [%s] should be a string",
                              field.fieldName() ) ;
            pSource = field.valuestr() ;
         }
         else if ( 0 == ossStrcasecmp( field.fieldName(),
                                       FIELD_NAME_CHECK_CLIENT_CATA_VERSION ) )
         {
            PD_LOG_MSG_CHECK( field.isBoolean(), SDB_INVALIDARG, error,
                              PDERROR, "Field [%s] should be a boolean",
                              field.fieldName() ) ;
            gotNeedCheckCatVer = TRUE;
            needCheckCatVer    = field.boolean() ? TRUE : FALSE;
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
      if ( gotNeedCheckCatVer )
      {
         setNeedCheckCatVer( needCheckCatVer );
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
