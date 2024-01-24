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

   Source File Name = coordCommandSequence.cpp

   Descriptive Name = Coordinator Sequence Command

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/06/2018  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "coordCommandSequence.hpp"
#include "coordSequenceAgent.hpp"
#include "coordCB.hpp"
#include "clsResourceContainer.hpp"
#include "coordResource.hpp"
#include "msgMessage.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"

using namespace bson ;

namespace engine
{
   static BOOLEAN coordIsSysSequence( const CHAR *sequenceName )
   {
      if ( utilStrStartsWithIgnoreCase( sequenceName, "SYS" ) ||
           utilStrStartsWithIgnoreCase( sequenceName, "$" ) )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   static INT32 coordExtractSeqID( rtnContextCoord *pContext,
                                   pmdEDUCB *cb,
                                   utilSequenceID &seqID )
   {
      INT32 rc = SDB_OK ;
      rtnContextBuf buffObj ;

      if ( NULL == pContext || NULL == cb )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = pContext->getMore( 1, buffObj, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get more from context [%lld], rc: %d",
                   pContext->contextID(), rc ) ;

      try
      {
         BSONObj obj( buffObj.data() ) ;
         INT64 num = 0 ;
         rc = rtnGetNumberLongElement( obj, FIELD_NAME_SEQUENCE_ID, num ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get sequence id, rc=%d", rc ) ;
         seqID = (utilSequenceID) num ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurs: %s", e.what() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   static INT32 coordValidateSeqOptions( const BSONObj &options )
   {
      INT32 rc = SDB_OK ;

      try
      {
         static const CHAR *SEQ_FIELD_ARRAY[] = {
            FIELD_NAME_NAME,
            FIELD_NAME_CURRENT_VALUE,
            FIELD_NAME_INCREMENT,
            FIELD_NAME_START_VALUE,
            FIELD_NAME_MIN_VALUE,
            FIELD_NAME_MAX_VALUE,
            FIELD_NAME_CACHE_SIZE,
            FIELD_NAME_ACQUIRE_SIZE,
            FIELD_NAME_CYCLED
         } ;
         static const UINT32 SEQ_FIELD_COUNT =
               sizeof( SEQ_FIELD_ARRAY ) / sizeof( SEQ_FIELD_ARRAY[0] ) ;

         BSONObjIterator iter( options ) ;

         while ( iter.more() )
         {
            BSONElement ele = iter.next() ;
            const CHAR *fieldName = ele.fieldName() ;
            BOOLEAN found = FALSE ;

            for ( UINT32 i = 0; i < SEQ_FIELD_COUNT; ++i )
            {
               if ( 0 == ossStrcmp( fieldName, SEQ_FIELD_ARRAY[i] ) )
               {
                  found = TRUE ;
                  break ;
               }
            }

            if ( !found )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "Unknown sequence option '%s'", fieldName ) ;
               break ;
            }
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurs: %s", e.what() ) ;
      }

      return rc ;
   }


   /*
      _coordCMDInvalidateSequenceCache implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDInvalidateSequenceCache,
                                      CMD_NAME_INVALIDATE_SEQUENCE_CACHE,
                                      TRUE ) ;
   _coordCMDInvalidateSequenceCache::_coordCMDInvalidateSequenceCache()
   {
   }

   _coordCMDInvalidateSequenceCache::~_coordCMDInvalidateSequenceCache()
   {
   }

   void _coordCMDInvalidateSequenceCache::_preSet( pmdEDUCB * cb,
                                                   coordCtrlParam & ctrlParam )
   {
      ctrlParam._isGlobal = TRUE ;
      ctrlParam._filterID = FILTER_ID_MATCHER ;
      ctrlParam._emptyFilterSel = NODE_SEL_ALL ;
      ctrlParam.resetRole() ;
      ctrlParam._role[ SDB_ROLE_COORD ] = 1 ;
   }

   UINT32 _coordCMDInvalidateSequenceCache::_getControlMask() const
   {
      return COORD_CTRL_MASK_GLOBAL ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_coordInvalidateSequenceCache)
   _coordInvalidateSequenceCache::_coordInvalidateSequenceCache()
   : _collection( NULL ),
     _fieldName( NULL ),
     _sequenceName( NULL ),
     _sequenceID( UTIL_SEQUENCEID_NULL ),
     _explicitCurrValue( FALSE ),
     _currentValue( 0 )
   {
   }

   _coordInvalidateSequenceCache::~_coordInvalidateSequenceCache()
   {
   }

   INT32 _coordInvalidateSequenceCache::spaceNode()
   {
      return CMD_SPACE_NODE_COORD ;
   }

   INT32 _coordInvalidateSequenceCache::init ( INT32 flags,
                                               INT64 numToSkip,
                                               INT64 numToReturn,
                                               const CHAR *pMatcherBuff,
                                               const CHAR *pSelectBuff,
                                               const CHAR *pOrderByBuff,
                                               const CHAR *pHintBuff )
   {
      INT32 rc = SDB_OK ;

      try
      {
         _object = BSONObj( pMatcherBuff ).getOwned() ;

         BSONElement e ;

         // check collection name
         if ( _object.hasField( FIELD_NAME_COLLECTION ) )
         {
            e = _object.getField( FIELD_NAME_COLLECTION ) ;
            PD_CHECK( String == e.type(), SDB_INVALIDARG, error, PDERROR,
                      "Field [%s] is invalid in obj [%s]",
                      FIELD_NAME_COLLECTION, _object.toString().c_str() ) ;
            _collection = e.valuestr() ;
         }

         // check field name
         if ( _object.hasField( FIELD_NAME_AUTOINC_FIELD ) )
         {
            e = _object.getField( FIELD_NAME_AUTOINC_FIELD ) ;
            PD_CHECK( String == e.type(), SDB_INVALIDARG, error, PDERROR,
                      "Field [%s] is invalid in obj [%s]",
                      FIELD_NAME_AUTOINC_FIELD, _object.toString().c_str() ) ;
            _fieldName = e.valuestr() ;
         }

         // check sequence name
         if ( _object.hasField( FIELD_NAME_SEQUENCE_NAME ) )
         {
            e = _object.getField( FIELD_NAME_SEQUENCE_NAME ) ;
            PD_CHECK( String == e.type(), SDB_INVALIDARG, error, PDERROR,
                      "Field [%s] is invalid in obj [%s]",
                      FIELD_NAME_SEQUENCE_NAME, _object.toString().c_str() ) ;
            _sequenceName = e.valuestr() ;
         }

         // check sequence ID
         if( _object.hasField( FIELD_NAME_SEQUENCE_ID ) )
         {
            e = _object.getField( FIELD_NAME_SEQUENCE_ID ) ;
            PD_CHECK( e.isNumber(), SDB_INVALIDARG, error, PDERROR,
                      "Field [%s] is invalid in obj [%s]",
                     FIELD_NAME_SEQUENCE_ID, _object.toString().c_str() ) ;
            _sequenceID = e.Long() ;
         }

         // check current value
         if( _object.hasField( FIELD_NAME_CURRENT_VALUE ) )
         {
            e = _object.getField( FIELD_NAME_CURRENT_VALUE ) ;
            PD_CHECK( e.isNumber(), SDB_INVALIDARG, error, PDERROR,
                      "Field [%s] is invalid in obj [%s]",
                     FIELD_NAME_CURRENT_VALUE, _object.toString().c_str() ) ;
            _explicitCurrValue = TRUE ;
            _currentValue = e.Long() ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      PD_CHECK( NULL != _sequenceName ||
                ( NULL != _collection && NULL != _fieldName ),
                SDB_INVALIDARG, error, PDERROR, "No sequence is given" ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordInvalidateSequenceCache::doit ( _pmdEDUCB *cb,
                                               SDB_DMSCB *dmsCB,
                                               _SDB_RTNCB *rtnCB,
                                               _dpsLogWrapper *dpsCB,
                                               INT16 w,
                                               INT64 *pContextID )
   {
      coordResource * resource = sdbGetResourceContainer()->getResource() ;
      SDB_ASSERT( NULL != resource, "coord resource is invalid" ) ;

      coordSequenceAgent * sequenceAgent = resource->getSequenceAgent() ;
      SDB_ASSERT( NULL != sequenceAgent, "coord sequence agent is invalid" ) ;

      if ( NULL != _sequenceName )
      {
         INT64 *pCurrentValue = _explicitCurrValue ? &_currentValue : NULL ;
         sequenceAgent->removeCache( _sequenceName, _sequenceID, pCurrentValue ) ;
         PD_LOG( PDDEBUG, "Removed sequence cache [%s]", _sequenceName ) ;
      }
      else if ( NULL != _collection && NULL != _fieldName )
      {
         CoordCataInfoPtr cataPtr ;

         // no need to get the latest ( in this case, the sender coord might
         // get problem with catalog )
         resource->getCataInfo( _collection, cataPtr ) ;
         if ( NULL != cataPtr.get() )
         {
            const clsAutoIncSet & autoIncSet = cataPtr->getAutoIncSet() ;
            const clsAutoIncItem * autoIncItem = autoIncSet.find( _fieldName ) ;
            if ( NULL != autoIncItem )
            {
               sequenceAgent->removeCache( autoIncItem->sequenceName(),
                                           autoIncItem->sequenceID() ) ;
               PD_LOG( PDDEBUG, "Removed sequence cache [%s]",
                       autoIncItem->sequenceName() ) ;
            }
            else
            {
               // auto increment field is not found, clear collection
               // catalog cache which might be too old
               PD_LOG( PDDEBUG, "Failed to find field [%s] in collection [%s], "
                       "clear catalog cache", _fieldName, _collection ) ;
               resource->removeCataInfo( _collection ) ;
            }
         }
         else
         {
            PD_LOG( PDDEBUG, "Failed to find collection [%s] in catalog cache",
                    _collection ) ;
         }
      }

      // ignore errors
      return SDB_OK ;
   }

   /*
      _coordCMDCreateSequence implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDCreateSequence,
                                      CMD_NAME_CREATE_SEQUENCE,
                                      FALSE ) ;
   _coordCMDCreateSequence::_coordCMDCreateSequence()
   {
   }

   _coordCMDCreateSequence::~_coordCMDCreateSequence()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_CREATE_SEQUENCE_EXE, "_coordCMDCreateSequence::execute" )
   INT32 _coordCMDCreateSequence::execute( MsgHeader *pMsg,
                                           pmdEDUCB *cb,
                                           INT64 &contextID,
                                           rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_CREATE_SEQUENCE_EXE ) ;

      contextID = -1 ;

      _printDebug ( (const CHAR*)pMsg, getName() ) ;

      try
      {
         const CHAR *pQuery = NULL ;
         BSONObj boQuery ;
         const CHAR *pSeqName = NULL ;
         MsgOpQuery *forward  = (MsgOpQuery *)pMsg;
         forward->header.opCode = MSG_GTS_SEQUENCE_CREATE_REQ ;

         rc = msgExtractQuery( (const CHAR*)pMsg, NULL, NULL,
                               NULL, NULL, &pQuery, NULL, NULL, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to extract message, rc: %d", rc ) ;

         boQuery = BSONObj( pQuery ) ;
         rc = rtnGetStringElement( boQuery, FIELD_NAME_NAME, &pSeqName ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get sequence name" ) ;

         rc = coordValidateSeqOptions( boQuery ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to validate sequence options, rc: %d",
                      rc ) ;

         if ( boQuery.hasField( FIELD_NAME_CURRENT_VALUE ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "CurrentValue is not a creation option" ) ;
            goto error ;
         }

         rc = executeOnCataGroup ( pMsg, cb, TRUE, NULL, NULL, buf ) ;
         PD_AUDIT_COMMAND( AUDIT_DDL, getName(), AUDIT_OBJ_SEQ, pSeqName, rc,
                           "Options:%s", boQuery.toString().c_str() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Execute on catalog failed in command[%s], rc: %d",
                     getName(), rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done :
      PD_TRACE_EXITRC ( COORD_CREATE_SEQUENCE_EXE, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   /*
      _coordCMDDropSequence implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDDropSequence,
                                      CMD_NAME_DROP_SEQUENCE,
                                      FALSE ) ;
   _coordCMDDropSequence::_coordCMDDropSequence()
   {
   }

   _coordCMDDropSequence::~_coordCMDDropSequence()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_DROP_SEQUENCE_EXE, "_coordCMDDropSequence::execute" )
   INT32 _coordCMDDropSequence::execute( MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         INT64 &contextID,
                                         rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_DROP_SEQUENCE_EXE ) ;

      const CHAR *pQuery = NULL ;
      BSONObj boQuery ;
      const CHAR *pSeqName = NULL ;
      rtnContextCoord::sharePtr pContext ;
      utilSequenceID seqID = UTIL_SEQUENCEID_NULL ;

      contextID = -1 ;

      _printDebug ( (const CHAR*)pMsg, getName() ) ;

      try
      {
         MsgOpQuery *forward  = (MsgOpQuery *)pMsg;
         forward->header.opCode = MSG_GTS_SEQUENCE_DROP_REQ ;

         rc = msgExtractQuery( (const CHAR*)pMsg, NULL, NULL, NULL, NULL,
                               &pQuery, NULL, NULL, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to extract message, rc: %d", rc ) ;

         boQuery = BSONObj( pQuery ) ;
         rc = rtnGetStringElement( boQuery, FIELD_NAME_NAME, &pSeqName ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get sequence name" ) ;

         if ( coordIsSysSequence( pSeqName ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "System sequences are not allowed to be dropped" ) ;
            goto error ;
         }

         rc = executeOnCataGroup ( pMsg, cb, TRUE, NULL, &pContext, buf ) ;
         PD_RC_CHECK( rc, PDERROR, "Execute on catalog failed in command[%s], "
                      "rc: %d", getName(), rc ) ;

         rc = coordExtractSeqID( pContext, cb, seqID ) ;
         if ( rc != SDB_OK )
         {
            PD_LOG( PDWARNING, "Failed to get sequence id from reply, rc: %d",
                    rc ) ;
            rc = SDB_OK ;
         }

         rc = coordSequenceInvalidateCache( pSeqName, seqID, cb ) ;
         if ( rc != SDB_OK )
         {
            PD_LOG( PDWARNING, "Failed to invalidate sequence[%s] cache, "
                    "rc: %d", pSeqName, rc ) ;
            rc = SDB_OK ;
         }
      }
      catch( std::exception& e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurs: %s", e.what() ) ;
         goto error ;
      }

   done:
      if ( pContext )
      {
         SDB_RTNCB *pRtnCB = pmdGetKRCB()->getRTNCB() ;
         pRtnCB->contextDelete( pContext->contextID(), cb ) ;
         pContext.release() ;
      }
      PD_AUDIT_COMMAND( AUDIT_DDL, getName(), AUDIT_OBJ_SEQ, pSeqName, rc, "" ) ;
      PD_TRACE_EXITRC ( COORD_DROP_SEQUENCE_EXE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordCMDAlterSequence implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDAlterSequence,
                                      CMD_NAME_ALTER_SEQUENCE,
                                      FALSE ) ;
   _coordCMDAlterSequence::_coordCMDAlterSequence()
   {
   }

   _coordCMDAlterSequence::~_coordCMDAlterSequence()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_ALTER_SEQUENCE_EXE, "_coordCMDAlterSequence::execute" )
   INT32 _coordCMDAlterSequence::execute( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          INT64 &contextID,
                                          rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_ALTER_SEQUENCE_EXE ) ;

      const CHAR *pQuery = NULL ;
      BSONObj boQuery ;
      BSONObj options ;
      const CHAR *pSeqName = "" ;
      const CHAR *pAction = "" ;
      utilSequenceID seqID = UTIL_SEQUENCEID_NULL ;
      rtnContextCoord::sharePtr pContext ;

      MsgOpQuery *pAttachMsg           = (MsgOpQuery *)pMsg ;
      pAttachMsg->header.opCode        = MSG_GTS_SEQUENCE_ALTER_REQ ;
      contextID                        = -1 ;

      rc = msgExtractQuery( (const CHAR*)pMsg, NULL, NULL, NULL, NULL,
                            &pQuery, NULL, NULL, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Extract command[%s] msg failed, rc: %d",
                   getName(), rc ) ;

      try
      {
         boQuery = BSONObj( pQuery ) ;
         rc = _parseArguments( boQuery, &pAction, options, &pSeqName ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse arguments, rc: %d", rc ) ;

         if ( coordIsSysSequence( pSeqName ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "System sequences are not allowed to be modified" ) ;
            goto error ;
         }

         // For 'set current value', firstly try setting on coord. If not in
         // coord, then try setting on catalog.
         if ( 0 == ossStrcmp( pAction, CMD_VALUE_NAME_SET_CURR_VALUE ) )
         {
            INT64 expectValue = 0 ;
            BOOLEAN isSet = FALSE ;
            rc = rtnGetNumberLongElement( options, FIELD_NAME_EXPECT_VALUE,
                                          expectValue ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse expect value, rc: %d", rc ) ;

            rc = _setCurrValueOnCoord( pSeqName, expectValue, cb, isSet ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to set current value on coord, rc=%d", rc ) ;
            if ( isSet )
            {
               goto done ;
            }
         }
         else if ( 0 == ossStrcmp( pAction, CMD_VALUE_NAME_SETATTR ) )
         {
            rc = coordValidateSeqOptions( options ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to validate sequence options, "
                         "rc: %d", rc ) ;
         }

         rc = executeOnCataGroup ( pMsg, cb, TRUE, NULL, &pContext, buf ) ;
         PD_RC_CHECK( rc, PDERROR, "Execute on catalog failed in command[%s], "
                      "rc: %d", getName(), rc ) ;

         rc = coordExtractSeqID( pContext, cb, seqID ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDWARNING, "Failed to get sequence id from reply, rc=%d", rc ) ;
            rc = SDB_OK ;
         }

         rc = _pResource->getSequenceAgent()->removeCache( pSeqName, seqID ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDWARNING, "Failed to remove local sequence cache, "
                    "name[%s], rc: %d", pSeqName, rc ) ;
            rc = SDB_OK ;
         }

         rc = coordSequenceInvalidateCache( pSeqName, seqID, cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDWARNING, "Failed to invalidate sequence cache, name[%s], "
                    "rc: %d", pSeqName, rc ) ;
            rc = SDB_OK ;
         }

      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "exception occurs during process command[%s]: %s",
                 getName(), e.what() ) ;
         goto error ;
      }

   done:
      if ( pContext )
      {
         SDB_RTNCB *pRtnCB = pmdGetKRCB()->getRTNCB() ;
         pRtnCB->contextDelete( pContext->contextID(), cb ) ;
         pContext.release() ;
      }
      PD_AUDIT_COMMAND( AUDIT_DDL, getName(), AUDIT_OBJ_SEQ, pSeqName,
                        rc, "Options:%s", boQuery.toString().c_str() ) ;

      PD_TRACE_EXITRC( COORD_ALTER_SEQUENCE_EXE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordCMDAlterSequence::_parseArguments( const BSONObj &boQuery,
                                                  const CHAR **ppAction,
                                                  BSONObj &options,
                                                  const CHAR **ppSeqName )
   {
      INT32 rc = SDB_OK ;
      BSONElement beOptions ;

      rc = rtnGetStringElement( boQuery, FIELD_NAME_ACTION, ppAction ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse the action, rc: %d", rc ) ;

      beOptions = boQuery.getField( FIELD_NAME_OPTIONS ) ;
      PD_CHECK( Object == beOptions.type(), SDB_INVALIDARG, error, PDERROR,
                "Field[%s] should be object, but found %s",
                FIELD_NAME_OPTIONS, beOptions.toString().c_str() ) ;
      options = beOptions.embeddedObject() ;

      rc = rtnGetStringElement( options, FIELD_NAME_NAME, ppSeqName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse the action, rc: %d", rc ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordCMDAlterSequence::_setCurrValueOnCoord( const CHAR *pSeqName,
                                                       const INT64 expectValue,
                                                       pmdEDUCB *cb,
                                                       BOOLEAN &isSet )
   {
      INT32 rc = SDB_OK ;
      coordSequenceAgent *pSeqAgent = _pResource->getSequenceAgent() ;
      utilSequenceID cachedSeqID = UTIL_SEQUENCEID_NULL ;

      rc = pSeqAgent->setCurrentValue( pSeqName, UTIL_SEQUENCEID_NULL,
                                       expectValue, cb, &cachedSeqID ) ;
      if ( SDB_OK == rc )
      {
         isSet = TRUE ;
         rc = coordSequenceInvalidateCache( pSeqName, cachedSeqID, cb,
                                            &expectValue ) ;
         if ( rc )
         {
            PD_LOG( PDWARNING, "Failed to invalidate sequence cache, name[%s], "
                    "rc: %d", pSeqName, rc ) ;
            rc = SDB_OK ;
         }
      }
      else if ( SDB_SEQUENCE_OUT_OF_CACHE == rc )
      {
         isSet = FALSE ;
         rc = SDB_OK ;
      }
      else
      {
         isSet = FALSE ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordCMDGetSeqCurrentValue implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDGetSeqCurrentValue,
                                      CMD_NAME_GET_SEQ_CURR_VAL,
                                      FALSE ) ;
   _coordCMDGetSeqCurrentValue::_coordCMDGetSeqCurrentValue()
   {
   }

   _coordCMDGetSeqCurrentValue::~_coordCMDGetSeqCurrentValue()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_GET_SEQ_CURR_VAL_EXE, "_coordCMDGetSeqCurrentValue::execute" )
   INT32 _coordCMDGetSeqCurrentValue::execute( MsgHeader *pMsg,
                                               pmdEDUCB *cb,
                                               INT64 &contextID,
                                               rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_GET_SEQ_CURR_VAL_EXE ) ;

      const CHAR *pQuery = NULL ;
      const CHAR *pSeqName = NULL ;
      coordSequenceAgent *pSeqAgent = _pResource->getSequenceAgent() ;
      INT64 currentValue = 0 ;
      BSONObjBuilder bob( 64 ) ;
      BSONObj result ;

      _printDebug ( (const CHAR*)pMsg, getName() ) ;

      rc = msgExtractQuery( (const CHAR*)pMsg, NULL, NULL, NULL, NULL,
                            &pQuery, NULL, NULL, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to extract message, rc: %d", rc ) ;

      try
      {
         BSONObj boQuery( pQuery ) ;
         pSeqName = boQuery.getField( FIELD_NAME_NAME ).valuestrsafe() ;

         rc = pSeqAgent->getCurrentValue( pSeqName, UTIL_SEQUENCEID_NULL,
                                          currentValue, cb ) ;
         if ( SDB_SEQUENCE_OUT_OF_CACHE == rc )
         {
            rc = _getCurrValueFromCatalog( pSeqName, pMsg, cb, currentValue ) ;
         }
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get current value of sequence[%s], rc: %d",
                      pSeqName, rc ) ;

         bob.append( FIELD_NAME_CURRENT_VALUE, currentValue ) ;
         result = bob.obj() ;

         *buf = rtnContextBuf( result ) ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         goto error ;
      }

   done :
      contextID = -1 ;
      PD_TRACE_EXITRC ( COORD_GET_SEQ_CURR_VAL_EXE, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_GET_SEQ_CURR_VAL__GET_CURR_VAL_FROM_CATA, "_coordCMDGetSeqCurrentValue::_getCurrValueFromCatalog" )
   INT32 _coordCMDGetSeqCurrentValue::_getCurrValueFromCatalog(
         const CHAR *pSeqName,
         MsgHeader *pMsg,
         pmdEDUCB *cb,
         INT64 &currentValue )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_GET_SEQ_CURR_VAL__GET_CURR_VAL_FROM_CATA ) ;
      SDB_RTNCB *pRtncb = pmdGetKRCB()->getRTNCB() ;
      coordCommandFactory *pFactory = coordGetFactory() ;
      coordOperator *pOperator = NULL ;
      rtnContextBuf buf ;
      rtnContextBuf buffObj ;
      INT64 contextID = 0 ;
      BSONObj obj ;
      BSONElement ele ;

      rc = pFactory->create( CMD_NAME_SNAPSHOT_SEQUENCES, pOperator ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Create operator by name[%s] failed, rc: %d",
                 CMD_NAME_SNAPSHOT_SEQUENCES, rc ) ;
         goto error ;
      }

      rc = pOperator->init( _pResource, cb, getTimeout() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init operator failed[%s], rc: %d",
                 pOperator->getName(), rc ) ;
         goto error ;
      }

      rc = pOperator->execute( pMsg, cb, contextID, &buf ) ;
      if ( rc != SDB_OK )
      {
         PD_LOG ( PDERROR, "Execute operator[%s] failed, rc: %d",
                  pOperator->getName(), rc ) ;
         goto error ;
      }

      rc = rtnGetMore( contextID, -1, buffObj, cb, pRtncb ) ;
      if ( rc )
      {
         contextID = -1 ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_SEQUENCE_NOT_EXIST ;
            PD_LOG ( PDWARNING, "Sequence[%s] doesn't exist", pSeqName ) ;
         }
         else
         {
            PD_LOG ( PDERROR, "Failed to get more, rc: %d", rc ) ;
         }
      }
      else
      {
         obj = BSONObj( buffObj.data() ) ;
         ele = obj.getField( FIELD_NAME_INITIAL ) ;
         if ( ele.booleanSafe() )
         {
            rc = SDB_SEQUENCE_NEVER_USED ;
            PD_LOG ( PDDEBUG, "Sequence[%s] has never been used", pSeqName ) ;
            goto done ;
         }

         ele = obj.getField( FIELD_NAME_CURRENT_VALUE ) ;
         currentValue = ele.numberLong() ;
      }

   done:
      if ( contextID >= 0 )
      {
         pRtncb->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      if ( pOperator )
      {
         pFactory->release( pOperator ) ;
      }
      PD_TRACE_EXITRC ( COORD_GET_SEQ_CURR_VAL__GET_CURR_VAL_FROM_CATA, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

